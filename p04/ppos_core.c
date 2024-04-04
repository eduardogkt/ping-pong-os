#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>

int task_id_count = 0;      // contador para id das tarefas
int user_tasks_count = 0;   // contador das tarefas restantes

task_t *curr_task;          // ponteiro para tarefa corrente
task_t main_task;           // tarefa main 
task_t dispatcher_task;     // tafera de dispatcher
task_t *ready_queue;        // fila de tarefas prontas

// =============================================================================

void print_elem (void *ptr) {
    task_t *elem = ptr;

    if (elem == NULL)
        return;

    elem->prev ? printf ("%d", elem->prev->id) : printf ("*") ;
    printf ("<%d>", elem->id) ;
    elem->next ? printf ("%d", elem->next->id) : printf ("*") ;
}

// =============================================================================

// implementação da tarefa scheduler
task_t *scheduler() {

    // elemento a ser removido (primeiro da fila)
    task_t *elem = ready_queue;

    // retirando o elemento escolhido da fila
    queue_remove((queue_t **) &ready_queue, (queue_t *) elem);

    return elem;
}

// implementação da tarefa dispatcher
void dispatcher() {
    // retira o dispatcher da fila de prontas, para evitar que ele ative a si próprio
    queue_remove((queue_t **) &ready_queue, (queue_t *) &dispatcher_task);
    
    // enquanto houverem tarefas do usuário
    while (user_tasks_count > 0) {

        // escolhe a próxima tarefa a executar
        task_t *next_task = scheduler();

        if (next_task != NULL) {
            
            next_task->status = PPOS_STATUS_RUNNING;

            // transfere controle para a próxima tarefa
            task_switch(next_task);

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
        }
    }
    // encerra a tarefa dispatcher
    task_exit(0);
}

// funções gerais ==============================================================

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {
    // desativa buffer de stdout
    setvbuf (stdout, 0, _IONBF, 0);

    // inicializando a tarefa main
    getcontext(&(main_task.context));
    main_task.id = task_id_count++;
    main_task.next = NULL;
    main_task.prev = NULL;
    main_task.status = PPOS_STATUS_RUNNING;

    // colocando main como tarefa corrente
    curr_task = &main_task;

    // inicializando a tarefa de dispatcher
    task_init(&dispatcher_task, (void *) dispatcher, "Dispatcher");

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

    task->id = task_id_count++;
    task->next = NULL;
    task->prev = NULL;
    task->status = PPOS_STATUS_NEW;

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
    #ifdef DEBUG
    printf("PPOS: task_exit - exiting task %d\n", curr_task->id);
    #endif

    if (curr_task == &dispatcher_task) {
        // liberando a pilha da tarefa de dispatcher
        free((&dispatcher_task)->context.uc_stack.ss_sp);
        exit(exit_code);
    }
    // decrementando o contador de tarefas do usuário para o dispatcher
    user_tasks_count--;
    curr_task->status = PPOS_STATUS_TERMINATED;

    // outras tarefas mudam para a tarefa de dispatcher
    task_switch(&dispatcher_task);
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {
    if (task == NULL) {
        fprintf(stderr, "Error: task switck - null task.\n");
        return PPOS_ERROR_SWITCH_NULL_TASK;
    }

    #ifdef DEBUG
    printf("PPOS: task_switch - task %d -> task %d\n", 
            curr_task->id, task->id);
    #endif

    task_t *src = curr_task;
    task_t *dest = task;
    
    // tarefa destino se torna a tarefa corrente
    curr_task = dest;
    
    if (swapcontext(&(src->context), &(dest->context)) < 0) {
        fprintf(stderr, "Error: task switch - contex swap failed.\n");
        return PPOS_ERROR_SWITCH_SWAP_CONTEXT;
    }
    return 0;
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

}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task) {
    return 0;
}

//==============================================================================