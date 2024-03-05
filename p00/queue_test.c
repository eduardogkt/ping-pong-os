#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct filaint_t
{
   struct filaint_t *prev ;  // ptr para usar cast com queue_t
   struct filaint_t *next ;  // ptr para usar cast com queue_t
   int id ;
   // outros campos podem ser acrescidos aqui
} filaint_t ;

void print_elem (void *ptr) {
   filaint_t *elem = ptr ;

   if (!elem)
      return ;

   elem->prev ? printf ("%d", elem->prev->id) : printf ("*") ;
   printf ("<%d>", elem->id) ;
   elem->next ? printf ("%d", elem->next->id) : printf ("*") ;
}

#define N 5

int main () {
   filaint_t *fila = NULL;
   // filaint_t *fila2 = NULL;

   filaint_t item[N];
   for (int i = 0; i < N; i++) {
      item[i].id = i;
      item[i].next = NULL;
      item[i].prev = NULL;
   }

   for (int i = 0; i < N; i++) {
      queue_append((queue_t **) &fila, (queue_t *) &item[i]);
   }
   printf("tamanho: %d\n", queue_size((queue_t *)fila));
   queue_print("fila", (queue_t *) fila, print_elem);

   // queue_append((queue_t **) &fila2, (queue_t *) item);
   
   // removendo do inicio
   // for (int i = 0; i < N; i++) {
   //    queue_remove((queue_t **) &fila, (queue_t *) &item[i]);
   //    printf("tamanho: %d\n", queue_size((queue_t *)fila));
   //    queue_print("fila", (queue_t *) fila, print_elem);
   // }

   // removendo do final
   // for (int i = N-1; i >= 0; i--) {
   //    queue_remove((queue_t **) &fila, (queue_t *) &item[i]);
   //    printf("tamanho: %d\n", queue_size((queue_t *)fila));
   //    queue_print("fila", (queue_t *) fila, print_elem);
   // }

   // for (int i = 1; i < N; i++) {
   //    queue_remove((queue_t **) &fila, (queue_t *) &item[i]);
   //    printf("tamanho: %d\n", queue_size((queue_t *)fila));
   //    queue_print("fila", (queue_t *) fila, print_elem);
   // }

   return 0;
}