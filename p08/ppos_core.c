#define _XOPEN_SOURCE 700  // evitar problemas com struct sigaction

#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

int task_id_count = 0;      // contador para id das tarefas
int user_tasks_count = 0;   // contador das tarefas restantes

task_t *curr_task;          // ponteiro para tarefa corrente
task_t main_task;           // tarefa main 
task_t dispatcher_task;     // tafera de dispatcher
task_t *ready_queue;        // fila de tarefas prontas

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
}

// retorna a tarefa com maior prioridade da fila de prontos
task_t *get_next_task() {
    // fila vazia
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

    // #ifdef DEBUG
    // printf("PPOS: sheduler - highest prio: %d\n", chosen_task->prio_d);
    // printf("PPOS: sheduler - chosen task: %d\n", chosen_task->id);
    // #endif
    
    return chosen_task;
}

// implementação da tarefa scheduler
task_t *scheduler() {
    // procura pela tarefa com maior prioridade na fila
    task_t *chosen_task = get_next_task();

    // resetando a prioridade dinamica da tarefa escolhida
    chosen_task->prio_d = chosen_task->prio_e;

    // retirando a tarefa com maior prioridade da fila
    queue_remove((queue_t **) &ready_queue, (queue_t *) chosen_task);

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

// implementação da tarefa dispatcher
void dispatcher() {
    // vars para medir o tempo de processador do dispatcher e das tarefas
    int start_cpu_time, end_cpu_time;

    dispatcher_task.activations++;

    // retira o dispatcher da fila de prontas, para evitar que ative a si mesmo
    queue_remove((queue_t **) &ready_queue, (queue_t *) &dispatcher_task);
    
    // enquanto houverem tarefas do usuário
    while (user_tasks_count > 0) {
        start_cpu_time = systime();

        // escolhe a próxima tarefa a executar
        task_t *next_task = scheduler();

        if (next_task == NULL) {
            continue;
        }
            
        next_task->status = PPOS_STATUS_RUNNING;
        next_task->activations++;

        end_cpu_time = systime();
        dispatcher_task.processor_time += (end_cpu_time - start_cpu_time);

        start_cpu_time = systime();
        
        // transfere controle para a próxima tarefa
        task_switch(next_task);

        end_cpu_time = systime();
        next_task->processor_time += (end_cpu_time - start_cpu_time);

        start_cpu_time = systime();
        dispatcher_task.activations++;

        // voltando ao dispatcher, trata a tarefa de acordo com seu estado
        switch (next_task->status) {
            case PPOS_STATUS_NEW: break;
            case PPOS_STATUS_READY: 
                // tarefa não acabou e precisa ser recolocada na fila de prontos
                queue_append((queue_t **) &ready_queue, (queue_t *) next_task);
                break;
                
            case PPOS_STATUS_RUNNING: break;
            case PPOS_STATUS_SUSPENDED: break;
            case PPOS_STATUS_TERMINATED: 
                // libera a pilha da tarefa
                free(next_task->context.uc_stack.ss_sp);
                break;
        }
        end_cpu_time = systime();
        dispatcher_task.processor_time += (end_cpu_time - start_cpu_time);
    }  // end while

    // encerra a tarefa dispatcher
    task_exit(0);
}

// tratador de interrupção de tick
void tick_handler() {
    clock++;
    if (curr_task->type == PPOS_USER_TASK) {
        curr_task->quantum--;
        if (curr_task->quantum == 0) {
            // retorna para o dispatcher e recoloca a tarefa na fila de prontas
            curr_task->quantum = PPOS_QUANTUM;
            curr_task->status = PPOS_STATUS_READY;
            dispatcher_task.status = PPOS_STATUS_RUNNING;
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
    printf("execution time %d ms, ", curr_task->execution_time);
    printf("processor time %d ms, ", curr_task->processor_time);
    printf("%d activations\n", curr_task->activations);

    // printf("PPOS: task_exit - ");
    // printf("task %d. ", curr_task->id);
    // printf("execution time %d ms. ", curr_task->execution_time); 
    // printf("processor time %d ms. ", curr_task->processor_time); 
    // printf("activations %d.\n", curr_task->activations);
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

    #ifdef DEBUG
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
    
    #ifdef DEBUG
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

    // acordando as tarefas que estão esperando
    task_t *waiting_tasks = curr_task->waiting_tasks;
    task_t *aux = waiting_tasks;

    if (waiting_tasks != NULL) {
        do {
            task_awake(aux, &(waiting_tasks));
            aux = aux->next;
        } while (aux != ready_queue);
    }

    task_switch(&dispatcher_task);
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {
    if (task == NULL) {
        fprintf(stderr, "Error: task switck - null task.\n");
        return PPOS_ERROR_SWITCH_NULL_TASK;
    }

    // #ifdef DEBUG
    // printf("PPOS: task_switch - task %d -> task %d\n", 
    //         curr_task->id, task->id);
    // #endif

    task_t *src = curr_task;
    task_t *dest = task;
    
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
    if (queue == NULL) {
        return;
    }
    
    #if DEBUG
    printf("PPOS: task_suspend - suspending task %d.\n", curr_task->id);
    #endif

    // remove tarefa atual da fila de prontas
    queue_remove((queue_t **) &ready_queue, (queue_t *) curr_task);

    curr_task->status = PPOS_STATUS_SUSPENDED;

    // insere tarefa atual na fila informada
    int ret = queue_append((queue_t **) queue, (queue_t *) curr_task);
    if (ret < 0) {
        fprintf(stderr, "Error: task_suspend - queue append.\n");
        return;
    }
    task_switch(&dispatcher_task);
}

// acorda a tarefa indicada,
// trasferindo-a da fila "queue" para a fila de prontas
void task_awake (task_t *task, task_t **queue) {
    if (queue == NULL) {
        return;
    }

    #if DEBUG
    printf("PPOS: task_awake - awaking task %d.\n", task->id);
    #endif

    // remove tarefa da fila informada
    queue_remove((queue_t **) queue, (queue_t *) task);

    task->status = PPOS_STATUS_READY;

    // insere tarefa na fila de prontas
    int ret = queue_append((queue_t **) &ready_queue, (queue_t *) task);
    if (ret < 0) {
        fprintf(stderr, "Error: task_suspend - queue append.\n");
        return;
    }
    task_switch(curr_task);
}

// operações de escalonamento ==================================================

// a tarefa atual libera o processador para outra tarefa
void task_yield () {
    #ifdef DEBUG
    printf("PPOS: task_yeld - task %d yields the CPU.\n", curr_task->id);
    #endif
    curr_task->status = PPOS_STATUS_READY;
    dispatcher_task.status = PPOS_STATUS_RUNNING;
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
    task_suspend(&(task->waiting_tasks));

    // no caso a tarefa a ser retirada da fila de prontas e colocada no fila da task
    // é a tarefa main

    return task->exit_code;
}

//==============================================================================