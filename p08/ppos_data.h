// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto

// status de tarefa
#define PPOS_STATUS_NEW 0
#define PPOS_STATUS_READY 1
#define PPOS_STATUS_RUNNING 2
#define PPOS_STATUS_SUSPENDED 3
#define PPOS_STATUS_TERMINATED 4

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

// escalonamento
#define PPOS_SCHED_AGING (-1)

// tipos de task
#define PPOS_USER_TASK 0
#define PPOS_SYS_TASK 1

// quantum para cada tarefa
#define PPOS_QUANTUM 20

#define STACKSIZE 64*1024

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
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
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

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
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif

