// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto

#define PPOS_ERROR_TASK_INIT_NULL_TASK -1
#define PPOS_ERROR_TASK_INIT_NULL_FUNC -2
#define PPOS_ERROR_TASK_INIT_GET_CONTEXT -3
#define PPOS_ERROR_TASK_INIT_NULL_STACK -4

#define PPOS_ERROR_TASK_SWITCH_NULL_TASK -1
#define PPOS_ERROR_TASK_SWITCH_SWAP_CONTEXT -2

#define STACKSIZE 64*1024

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;		// ponteiros para usar em filas
  int id ;			                	// identificador da tarefa
  ucontext_t context ;		       	// contexto armazenado da tarefa
  short status ;		            	// pronta, rodando, suspensa, ...
  // ... (outros campos serão adicionados mais tarde)
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

