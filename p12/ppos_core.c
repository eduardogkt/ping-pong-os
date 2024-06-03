#define _XOPEN_SOURCE 700  // evitar problemas com struct sigaction

#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

int task_id_count = 0;      // contador para id das tarefas
int user_tasks_count = 0;   // contador das tarefas restantes

task_t *curr_task;          // ponteiro para tarefa corrente
task_t main_task;           // tarefa main 
task_t dispatcher_task;     // tafera de dispatcher
task_t *ready_queue;        // fila de tarefas prontas
task_t *sleep_queue;        // fila de tarefas dormindo (suspensas)

struct sigaction action;    // estrutura para definir tratador de interrupções
struct itimerval timer;     // estrutura para timer de interrrupções

unsigned int clock = 0;     // timer interno (em milisegundos)

// =============================================================================

// imprime elemento com id inteiro
void print_elem (void *ptr) {
    task_t *elem = ptr;

    if (elem == NULL)
        return;

    elem->prev ? printf ("%d", elem->prev->id) : printf ("*") ;
    printf ("<%d>", elem->id) ;
    elem->next ? printf ("%d", elem->next->id) : printf ("*") ;
}

// inicializa os campos da tarefa
void set_task(task_t *task, short type, short prio, short status) {
    task->id = task_id_count++;
    task->next = NULL;
    task->prev = NULL;
    task->status = status;
    task_setprio(task, prio);
    task->type = type;
    task->quantum = PPOS_QUANTUM;
    task->activations = 0;
    task->init_time = systime();
    task->processor_time = 0;
    task->execution_time = 0;
    task->exit_code = 0;
    task->wait_queue = NULL;
}

// retorna a tarefa com maior prioridade da fila de prontos
task_t *get_next_task() {
    if (ready_queue == NULL) {
        return NULL;
    }

    task_t *aux = ready_queue;
    task_t *chosen_task = ready_queue;
    do {
        // pega a tarefa com maior prioridade
        if (aux->prio_d <= chosen_task->prio_d) {

            if (aux->prio_d < chosen_task->prio_d) {
                chosen_task = aux;
            }
            else { // aux->prio_d == chosen_task->prio_d

                if (aux->prio_e < chosen_task->prio_e) {
                    chosen_task = aux;
                }
                // se aux.prio_e == chosen_task.prio_e escolhe tarefa mais 
                // velha, ou seja, a tarefa em chosen_task 
            } 
            
        }
        aux = aux->next;
    } while (aux != ready_queue);

    // #if DEBUG
    // printf("PPOS: sheduler - highest prio: %d\n", chosen_task->prio_d);
    // printf("PPOS: sheduler - chosen task: %d\n", chosen_task->id);
    // #endif
    
    return chosen_task;
}

// implementação da tarefa scheduler
task_t *scheduler() {
    // procura pela tarefa com maior prioridade na fila
    task_t *chosen_task = get_next_task();
    if (chosen_task == NULL) {
        return NULL;
    }

    // resetando a prioridade dinamica da tarefa escolhida
    chosen_task->prio_d = chosen_task->prio_e;

    // retirando a tarefa com maior prioridade da fila
    // queue_remove((queue_t **) &ready_queue, (queue_t *) chosen_task);

    // atualizando a prioridade das tarefas nao escolhidas
    task_t *aux = ready_queue;
    if (ready_queue != NULL) {
        do {
            aux->prio_d += PPOS_SCHED_AGING;

            // ajustando caso prioridade passe do limite de -20
            aux->prio_d = (aux->prio_d < -20) ? -20 : aux->prio_d;

            aux = aux->next;
        } while (aux != ready_queue);
    }

    return chosen_task;
}

// procura por tarefas que precisam ser acordadas na fila de tarefas dormindo
void check_sleep_queue() {
    if (sleep_queue == NULL) {
        return;
    }

    unsigned int curr_time = systime();

    task_t *aux = sleep_queue;

    while (sleep_queue != NULL) {
        task_t *next = aux->next;

        if (aux->awake_time <= curr_time) {
            task_awake(aux, &sleep_queue);
        }
        aux = next;

        // deu a volta na fila
        if (aux == sleep_queue) {
            break;
        }
    }
}

// trata a tarefa de acordo com seu estado
void status_handler(task_t *task) {
    switch (task->status) {
        case PPOS_STATUS_TERMINATED: 
            // remove a tarefa da fila de prontos apenas ao final 
            if (queue_remove((queue_t **) &ready_queue, (queue_t *) task) < 0) {
                fprintf(stderr, "Error: dispatcher - queue remove failed.\n");
            }
            // libera a pilha da tarefa
            free(task->context.uc_stack.ss_sp);
            break;
        case PPOS_STATUS_READY: break;
        case PPOS_STATUS_NEW: break;
        case PPOS_STATUS_RUNNING: break;
        case PPOS_STATUS_SUSPENDED: break;
    }
}

// implementação da tarefa dispatcher
void dispatcher() {
    // retira o dispatcher da fila de prontas, para evitar que ative a si mesmo
    queue_remove((queue_t **) &ready_queue, (queue_t *) &dispatcher_task);
    
    // enquanto houverem tarefas do usuário
    while (user_tasks_count > 0 || queue_size((queue_t *) sleep_queue) > 0) {
        
        // escolhe a próxima tarefa a executar
        task_t *next_task = scheduler();

        if (next_task != NULL) {
            
            // transfere controle para a próxima tarefa
            task_switch(next_task);

            // trata a tarefa de acordo com seu estado
            status_handler(next_task);
        }

        // verifica as tarefas que precisam ser acordadas
        check_sleep_queue();
    }

    // encerra a tarefa dispatcher
    task_exit(0);
}

// tratador de interrupção de tick
void tick_handler() {
    clock++;
    curr_task->processor_time++;

    if (curr_task->type == PPOS_USER_TASK) {
        curr_task->quantum--;
        if (curr_task->quantum == 0) {
            // retorna para o dispatcher e recoloca a tarefa na fila de prontas
            curr_task->quantum = PPOS_QUANTUM;
            curr_task->status = PPOS_STATUS_READY;
            task_switch(&dispatcher_task);
        }
    }
}

// incializa tick handler para lidar com as interrupções
void tick_handler_init() {
    // registra a ação para o sinal de timer SIGALRM (sinal do timer)
    action.sa_handler = tick_handler;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;

    int status = sigaction(SIGALRM, &action, 0); 
    if (status < 0) {
        fprintf(stderr, "Error: task init - tick handler init failed.\n");
        exit (PPOS_ERROR_HANDLER_INIT);
    }
}

// incializa o temporizador para ticks
void timer_init() {
    // inicializando o temporizador
    timer.it_value.tv_usec = 1000;     // primeiro disparo em micro-segundos
    timer.it_value.tv_sec  = 0;        // primeiro disparo em segundos
    timer.it_interval.tv_usec = 1000;  // disparos subsequentes em microsegundos
    timer.it_interval.tv_sec  = 0;     // disparos subsequentes em segundos

    // arma o temporizador ITIMER_REAL
    int status = setitimer(ITIMER_REAL, &timer, 0);
    if (status < 0) {
        fprintf(stderr, "Error: task init - timer init failed.\n");
        exit(PPOS_ERROR_TIMER_INIT);
    }
}

// imprime as informações de tempo final da tarefa finalizada
void display_time_infos() {
    curr_task->execution_time = systime() - curr_task->init_time;
    printf("Task %d exit: ", curr_task->id);
    printf("execution time %5d ms, ", curr_task->execution_time);
    printf("processor time %5d ms, ", curr_task->processor_time);
    printf("%d activations\n", curr_task->activations);

    // printf("PPOS: task_exit - ");
    // printf("task %d. ", curr_task->id);
    // printf("execution time %5d ms. ", curr_task->execution_time); 
    // printf("processor time %5d ms. ", curr_task->processor_time); 
    // printf("activations %d.\n", curr_task->activations);
}

// acorda todas as tarefas da fila
void awake_all_tasks(task_t *queue) {
    task_t *aux = queue;

    while (aux != NULL) {
        task_awake(aux, &queue);
        aux = queue;
    }
}

// entra na seção critica
void enter_cs(int *lock) {
    // atomic OR (Intel macro for GCC)
    while (__sync_fetch_and_or(lock, 1));
}
 
// sai da seção critica
void leave_cs(int *lock) {
    (*lock) = 0;
}

// funções gerais ==============================================================

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {
    // desativa buffer de stdout
    setvbuf (stdout, 0, _IONBF, 0);

    // incializa os campos da tarefa main
    getcontext(&(main_task.context));
    set_task(&main_task, PPOS_USER_TASK, 0, PPOS_STATUS_RUNNING);
    main_task.activations = 1;  // ativação inicial
    queue_append((queue_t **) &ready_queue, (queue_t *) &main_task);

    // colocando main como tarefa corrente
    curr_task = &main_task;

    // inicializando a tarefa de dispatcher
    task_init(&dispatcher_task, (void *) dispatcher, "Dispatcher");
    dispatcher_task.type = PPOS_SYS_TASK;

    tick_handler_init();
    timer_init();

    #if DEBUG
    printf("PPOS: ppos_init\n");
    printf("  main task %d (%p)\n", main_task.id, &main_task);
    printf("  curr task %d (%p)\n", curr_task->id, curr_task);
    printf("  disp task %d (%p)\n", dispatcher_task.id, &dispatcher_task);
    #endif
}

// gerência de tarefas =========================================================

// Inicializa uma nova tarefa. Retorna um ID> 0 ou erro.
int task_init (task_t *task, void (*start_func)(void *), void *arg) {

    if (task == NULL) {
        fprintf(stderr, "Error: task init - null task.\n");
        return PPOS_ERROR_INIT_NULL_TASK;
    }
    if (start_func == NULL) {
        fprintf(stderr, "Error: task init - null start function.\n");
        return PPOS_ERROR_INIT_NULL_FUNC;
    }
    
    // salvando o contexto
    if (getcontext (&(task->context)) < 0) {
        fprintf(stderr, "Error: task init - get context failed.\n");
        return PPOS_ERROR_INIT_GET_CONTEXT;
    }

    char *stack = malloc(STACKSIZE);
    if (stack == NULL) {
        fprintf(stderr, "Error: task init - stack creation failed.\n");
        return PPOS_ERROR_INIT_NULL_STACK;
    }

    // inicializando variaveis de contexto
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;

    // contexto da tarefa alterada para a função especificada
    makecontext(&(task->context), (void *) start_func, 1, arg);

    // inicialização dos campos da tarefa com prioridade 0
    set_task(task, PPOS_USER_TASK, 0, PPOS_STATUS_NEW);

    // adicionando a tarefa a fila de prontos
    queue_append((queue_t **) &ready_queue, (queue_t *) task);

    // contador de tarefas de usuário para o dispatcher
    user_tasks_count++;
    
    #if DEBUG
    printf("PPOS: task_init - task %d initialized\n", task->id);
    queue_print("READY: ", (queue_t *) ready_queue, print_elem);
    #endif

    return task->id;
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {
    return curr_task->id;
}

// Termina a tarefa corrente com um status de encerramento
void task_exit (int exit_code) {
    // imprimindo informações de tempo da tarefa
    display_time_infos();

    if (curr_task == &dispatcher_task) {
        // liberando a pilha da tarefa de dispatcher
        free((&dispatcher_task)->context.uc_stack.ss_sp);
        exit(exit_code);
    }

    // decrementando o contador de tarefas do usuário para o dispatcher
    user_tasks_count--;
    curr_task->status = PPOS_STATUS_TERMINATED;
    curr_task->exit_code = exit_code;

    // acordando as tarefas que estão esperando na fila da tarefa corrente
    awake_all_tasks(curr_task->wait_queue);

    task_switch(&dispatcher_task);
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {
    if (task == NULL) {
        fprintf(stderr, "Error: task switch - null task.\n");
        return PPOS_ERROR_SWITCH_NULL_TASK;
    }

    // #if DEBUG
    // printf("PPOS: task_switch - task %d -> task %d\n", 
    //         curr_task->id, task->id);
    // #endif

    task_t *src = curr_task;
    task_t *dest = task;

    dest->status = PPOS_STATUS_RUNNING;
    dest->activations++;
    
    // tarefa destino se torna a tarefa corrente
    curr_task = dest;
    
    if (swapcontext(&(src->context), &(dest->context)) < 0) {
        fprintf(stderr, "Error: task switch - context swap failed.\n");
        return PPOS_ERROR_SWITCH_SWAP_CONTEXT;
    }
    return 0;
}

// suspende a tarefa atual,
// transferindo-a da fila de prontas para a fila "queue"
void task_suspend (task_t **queue) {
    #if DEBUG
    printf("PPOS: task_suspend - suspending task %d.\n", curr_task->id);
    #endif

    int status;

    // remove tarefa atual da fila de prontas
    status = queue_remove((queue_t **) &ready_queue, (queue_t *) curr_task);
    if (status < 0) {
        fprintf(stderr, 
                "PPOS: task_suspend - task %d queue remove failed.\n", 
                curr_task->id);
    }

    curr_task->status = PPOS_STATUS_SUSPENDED;

    // insere tarefa atual na fila informada
    status = queue_append((queue_t **) queue, (queue_t *) curr_task);
    if (status < 0) {
        fprintf(stderr, 
                "PPOS: task_suspend - task %d queue append failed.\n", 
                curr_task->id);
    }

    #if DEBUG
    queue_print("READY", (queue_t *) ready_queue, print_elem);
    queue_print("SLEEP", (queue_t *) sleep_queue, print_elem);
    queue_print("QUEUE", (queue_t *) *queue, print_elem);
    #endif

    task_switch(&dispatcher_task);
}

// acorda a tarefa indicada,
// trasferindo-a da fila "queue" para a fila de prontas
void task_awake (task_t *task, task_t **queue) {
    #if DEBUG
    printf("PPOS: task_awake - awaking task %d.\n", task->id);
    #endif

    int status;

    // remove tarefa da fila informada
    status = queue_remove((queue_t **) queue, (queue_t *) task);
    if (status < 0) {
        fprintf(stderr, 
                "PPOS: task_awake - task %d queue remove failed.\n", 
                task->id);
    }

    task->status = PPOS_STATUS_READY;

    // insere tarefa na fila de prontas
    status = queue_append((queue_t **) &ready_queue, (queue_t *) task);
    if (status < 0) {
        fprintf(stderr, 
                "PPOS: task_awake - task %d queue append failed.\n", 
                task->id);
    }

    #if DEBUG
    queue_print("READY", (queue_t *) ready_queue, print_elem);
    queue_print("SLEEP", (queue_t *) sleep_queue, print_elem);
    queue_print("QUEUE", (queue_t *) *queue, print_elem);
    #endif
}

// operações de escalonamento ==================================================

// a tarefa atual libera o processador para outra tarefa
void task_yield () {
    #if DEBUG
    printf("PPOS: task_yeld - task %d yields the CPU.\n", curr_task->id);
    #endif
    
    curr_task->status = PPOS_STATUS_READY;
    task_switch(&dispatcher_task);
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio) {
    
    // ajusta prioridae caso esteja fora de [-20, 20]
    prio = (prio < -20) ? -20 : (prio > 20) ? 20 : prio;

    // se task nao for informada atribui a prioridade a tarefa atual
    if (task == NULL) {
        curr_task->prio_e = prio;
        curr_task->prio_d = prio;
    } 
    else {
        task->prio_e = prio;
        task->prio_d = prio;
    }
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task) {
    return (task == NULL) ? curr_task->prio_e : task->prio_e;
}

// operações de gestão do tempo ================================================

// retorna o relógio atual (em milisegundos)
unsigned int systime () {
    return clock;
}

// suspende a tarefa corrente por t milissegundos
void task_sleep (int t) {
    curr_task->awake_time = systime() + t;

    #if DEBUG
    printf("PPOS: task_sleep - task %d sleeping for %dms (awake at %d).\n",
            curr_task->id, t, curr_task->awake_time);
    #endif

    task_suspend(&sleep_queue);
}

// operações de sincronização ==================================================

// a tarefa corrente aguarda o encerramento de outra task
int task_wait (task_t *task) {
    #if DEBUG
    printf("PPOS: task_wait - task %d waiting for task %d.\n", 
            curr_task->id, task->id);
    #endif
    
    if (task == NULL || task->status == PPOS_STATUS_TERMINATED) {
        return PPOS_ERROR_WAIT_INVALID_TASK;
    }
    // suspender a tarefa atual e coloca na fila de espera da tarefa task
    task_suspend(&(task->wait_queue));

    return task->exit_code;
}

// inicializa um semáforo com valor inicial "value"
int sem_init (semaphore_t *s, int value) {
    if (s == NULL) {
        return PPOS_ERROR_SEMAPHORE;
    }

    if (queue_size((queue_t *) s->queue) != 0) {
        return PPOS_ERROR_SEMAPHORE;
    }

    #if DEBUG
    printf("PPOS: sem_init - semaphore initialized with %d.\n", value);
    #endif

    s->lock = 0;
    s->count = value;
    s->queue = NULL;
    s->exists = 1;
    return 0;
}

// requisita o semáforo
int sem_down (semaphore_t *s) {
    if (s == NULL || !s->exists) {
        return PPOS_ERROR_SEMAPHORE;
    }

    #if DEBUG
    printf("PPOS: sem_down - task %d.\n", curr_task->id);
    #endif

    enter_cs(&(s->lock));
    s->count--;
    if (s->count < 0) {
        leave_cs(&(s->lock));
        task_suspend(&(s->queue));
    }
    else {
        leave_cs(&(s->lock));
    }

    return 0;
}

// libera o semáforo
int sem_up (semaphore_t *s) {
    if (s == NULL || !s->exists) {
        return PPOS_ERROR_SEMAPHORE;
    }

    #if DEBUG
    printf("PPOS: sem_up - task %d.\n", curr_task->id);
    #endif

    enter_cs(&(s->lock));
    s->count++;
    if (s->count <= 0) {
        task_awake(s->queue, &(s->queue));
    }
    leave_cs(&(s->lock));

    return 0;
}

// "destroi" o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s) {
    if (s == NULL || !s->exists) {
        return PPOS_ERROR_SEMAPHORE;
    }

    #if DEBUG
    printf("PPOS: sem_destroy - task %d.\n", curr_task->id);
    #endif

    while (s->queue != NULL) {
        sem_up(s);
    }
    s->exists = 0;

    return 0;
}

// inicializa um mutex (sempre inicialmente livre)
int mutex_init (mutex_t *m) {
    return 0;
}

// requisita o mutex
int mutex_lock (mutex_t *m) {
    return 0;
}

// libera o mutex
int mutex_unlock (mutex_t *m) {
    return 0;
}

// "destroi" o mutex, liberando as tarefas bloqueadas
int mutex_destroy (mutex_t *m) {
    return 0;
}

// inicializa uma barreira para N tarefas
int barrier_init (barrier_t *b, int N) {
    return 0;
}

// espera na barreira
int barrier_wait (barrier_t *b) {
    return 0;
}

// destrói a barreira, liberando as tarefas
int barrier_destroy (barrier_t *b) {
    return 0;
}

// operações de comunicação ====================================================

// inicializa um item para o buffer da fila de mensagens
mqueue_item_t *mqueue_item_create(void *msg, int item_size) {
    mqueue_item_t *mq_item = malloc(sizeof(mqueue_item_t));
    if (mq_item == NULL) {
        return NULL;
    }
    mq_item->item = malloc(item_size);
    if (mq_item == NULL) {
        return NULL;
    }
    memcpy(mq_item->item, msg, item_size);
    mq_item->prev = NULL;
    mq_item->next = NULL;
    
    return mq_item;
}

// inicializa uma fila para até max mensagens de size bytes cada
int mqueue_init (mqueue_t *queue, int max, int size) {
    queue->max_items = max;
    queue->num_items = 0;
    queue->item_size = size;
    queue->buffer = NULL;

    if (sem_init(&(queue->s_slot), max) < 0) {
        fprintf(stderr, "Error: mqueue_init - slot sem init failure.\n");
        return PPOS_ERROR_MQUEUE;
    }
    if (sem_init(&(queue->s_item), 0) < 0) {
        fprintf(stderr, "Error: mqueue_init - item  sem init failure.\n");
        return PPOS_ERROR_MQUEUE;
    }
    if (sem_init(&(queue->s_buff), 1) < 0) {
        fprintf(stderr, "Error: mqueue_init - buff sem init failure.\n");
        return PPOS_ERROR_MQUEUE;
    }

    return 0;
}

// envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg) {
    if (sem_down(&(queue->s_slot))) return PPOS_ERROR_MQUEUE;
    if (sem_down(&(queue->s_buff))) return PPOS_ERROR_MQUEUE;
    
    // coloca no buffer
    mqueue_item_t *mq_item = mqueue_item_create(msg, queue->item_size);
    if (mq_item == NULL) {
        fprintf(stderr, "Error: mqueue_send - item create failure.\n");
        return PPOS_ERROR_MQUEUE;
    }

    int status = queue_append((queue_t **) &(queue->buffer), (queue_t *) mq_item);
    if (status < 0) {
        fprintf(stderr, 
                "Error: msg_send - task %d queue append failed.\n",
                task_id());
        return PPOS_ERROR_MQUEUE;
    }
    queue->num_items++;
    
    if (sem_up(&(queue->s_buff))) return PPOS_ERROR_MQUEUE;
    if (sem_up(&(queue->s_item))) return PPOS_ERROR_MQUEUE;
    
    return 0;
}

// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg) {
    if (sem_down(&(queue->s_item))) return PPOS_ERROR_MQUEUE;
    if (sem_down(&(queue->s_buff))) return PPOS_ERROR_MQUEUE;

    // retira do buffer
    mqueue_item_t *first = queue->buffer;
    if (queue_remove((queue_t **) &(queue->buffer), (queue_t *) first) < 0) {
        fprintf(stderr, 
                "Error: mqueue_recv - task %d queue remove failure.\n",
                task_id());
        return PPOS_ERROR_MQUEUE;
    }
    queue->num_items--;

    memcpy(msg, first->item, queue->item_size);
    free(first->item);

    if (sem_up(&(queue->s_buff))) return PPOS_ERROR_MQUEUE;
    if (sem_up(&(queue->s_slot))) return PPOS_ERROR_MQUEUE;

    return 0;
}

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue) {
    while (queue->buffer != NULL) {
        free(queue->buffer->item);
        queue_remove((queue_t **) &(queue->buffer), (queue_t *) queue->buffer);
    }
    free(queue->buffer);

    sem_destroy(&(queue->s_slot));
    sem_destroy(&(queue->s_item));
    sem_destroy(&(queue->s_buff));
    
    return 0;
}

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue) {
    return queue->num_items;
}

//==============================================================================