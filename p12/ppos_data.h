// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto

// macros para contagem de tempo de processador
// obtem o tempo atual e armazena em timer
#define GET_CURR_TIME(timer) \
    timer = systime();

// obtem o tempo inicial de uso
#define START_PROCESSOR_TIME_COUNT  \
    GET_CURR_TIME(time_start)

// obtem o tempo final de uso de processador e calcula o tempo de processador
#define END_PROCESSOR_TIME_COUNT(task) \
    GET_CURR_TIME(time_end) \
    task->processor_time += (time_end - time_start);

// status de tarefa
#define PPOS_STATUS_NEW 0
#define PPOS_STATUS_READY 1
#define PPOS_STATUS_RUNNING 2
#define PPOS_STATUS_SUSPENDED 3
#define PPOS_STATUS_TERMINATED 4

// escalonamento
#define PPOS_SCHED_AGING (-1)

// tipos de task
#define PPOS_USER_TASK 0
#define PPOS_SYS_TASK 1

// quantum para cada tarefa
#define PPOS_QUANTUM 20

#define STACKSIZE 64*1024

// erros task_init
#define PPOS_ERROR_INIT_NULL_TASK -1
#define PPOS_ERROR_INIT_NULL_FUNC -2
#define PPOS_ERROR_INIT_GET_CONTEXT -3
#define PPOS_ERROR_INIT_NULL_STACK -4

// erros task_switch
#define PPOS_ERROR_SWITCH_NULL_TASK -1
#define PPOS_ERROR_SWITCH_SWAP_CONTEXT -2

// erros task_init
#define PPOS_ERROR_TIMER_INIT -1
#define PPOS_ERROR_HANDLER_INIT -2

// erros task_wait
#define PPOS_ERROR_WAIT_INVALID_TASK -1

// erros semaforo
#define PPOS_ERROR_SEMAPHORE -1

// erros filas de mensagem
#define PPOS_ERROR_MQUEUE -1

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t {
  struct task_t *prev, *next;   // ponteiros para usar em filas
  int id;                       // identificador da tarefa
  ucontext_t context;           // contexto armazenado da tarefa
  short status;                 // pronta, rodando, suspensa, ...
  
  short prio_e, prio_d;         // prioridade estatica
  short type;                   // user task, system task
  unsigned short quantum;       // contador para quantum

  unsigned int init_time;       // horario de criação da tarefa
  unsigned int execution_time;  // tempo de excução da tarefa
  unsigned int processor_time;  // tempo de processsamento da tarefa
  unsigned int activations;     // numero de ativações da tarefa

  struct task_t *wait_queue;    // tarefas suspensas esperando a tarefa terminar
  int exit_code;                // código de saida da tarefa

  unsigned int awake_time;      // horario para acordar a terafa quando suspensa
} task_t;

// estrutura que define um semáforo
typedef struct semaphore_t {
  int lock;
  int count;
  int exists;
  task_t *queue;
} semaphore_t;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct mqueue_t {
  int max_items;   // capacidade maxima do buffer
  int num_items;   // quantidada de itens no buffer
  int item_size;   // tamanho dos itens do buffer
  struct mqueue_item_t *buffer;    // buffer de itens genericos
  struct semaphore_t s_slot, s_item, s_buff;   // semaforos de acesso ao buffer
} mqueue_t;

typedef struct mqueue_item_t {
  struct mqueue_item_t *prev, *next;
  void *item;
} mqueue_item_t;

#endif

