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


// funções gerais ==============================================================

// implementação da tarefa dispatcher
void dispatcher() {

}

// implementação da tarefa scheduler
void scheduler() {

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
    main_task.status = PPOS_TASK_STATUS_RUNNING;

    // inicializando a tarefa de dispatcher
    task_init(&dispatcher_task, (void *) dispatcher, "Dispatcher");

    // colocando main como tarefa corrente
    curr_task = &main_task;
    
    #ifdef DEBUG
    printf("ppos_init:\n");
    printf("  main task %d (%p)\n", main_task.id, &main_task);
    printf("  curr task %d (%p)\n", curr_task->id, curr_task);
    #endif
}

// gerência de tarefas =========================================================

// Inicializa uma nova tarefa. Retorna um ID> 0 ou erro.
int task_init (task_t *task, void (*start_func)(void *), void *arg) {

    if (task == NULL) {
        fprintf(stderr, "Error: task init - null task.\n");
        return PPOS_ERROR_TASK_INIT_NULL_TASK;
    }
    if (start_func == NULL) {
        fprintf(stderr, "Error: task init - null start function.\n");
        return PPOS_ERROR_TASK_INIT_NULL_FUNC;
    }
    
    // salvando o contexto
    if (getcontext (&(task->context)) < 0) {
        fprintf(stderr, "Error: task init - get context failed.\n");
        return PPOS_ERROR_TASK_INIT_GET_CONTEXT;
    }

    char *stack = malloc(STACKSIZE);
    if (stack == NULL) {
        fprintf(stderr, "Error: task init - stack creation failed.\n");
        return PPOS_ERROR_TASK_INIT_NULL_STACK;
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
    task->status = PPOS_TASK_STATUS_NEW;
    
    #ifdef DEBUG
    printf("task_init: task %d initialized\n", task->id);
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
    printf("task_exit: exiting task %d\n", curr_task->id);
    #endif

    // troca de contexto para main
    if (curr_task->id != 0)
        task_switch(&main_task);



    // sempre que terminar uma tarefa, mudar para o dispatcher
    // se o dipatcher chamar o task_exit, então terminar o programa com exit_code
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {
    if (task == NULL) {
        fprintf(stderr, "Error: task switck - null task.\n");
        return PPOS_ERROR_TASK_SWITCH_NULL_TASK;
    }

    #ifdef DEBUG
    printf("task_switch: switching context %d -> %d\n", 
            curr_task->id, task->id);
    #endif

    task_t *src = curr_task;
    task_t *dest = task;
    
    // tarefa destino se torna a tarefa corrente
    curr_task = task;

    if (swapcontext(&(src->context), &(dest->context)) < 0) {
        fprintf(stderr, "Error: task switch - contex swap failed.\n");
        return PPOS_ERROR_TASK_SWITCH_SWAP_CONTEXT;
    }
    return 0;
}

//==============================================================================