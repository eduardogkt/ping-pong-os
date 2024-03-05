#include "queue.h"
#include <stdio.h>

//------------------------------------------------------------------------------
// Funções auxiliares


//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue) {
    // if queue is empty or doesn't exist
    if (queue == NULL)
        return 0;

    int size = 1;
    queue_t *aux = queue;
    while (aux->next != queue) {
        size++;
        aux = aux->next;
    }
    return size;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {

    if (queue == NULL) {
        printf("%s: []\n", name);
        return;
    }

    printf("%s: [", name);

    queue_t *aux = queue;
    while (aux->next != queue) {
        print_elem((void *) aux);
        printf(" ");
        aux = aux->next;
    };
    print_elem((void *) aux);

    printf("]\n");
}

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_append (queue_t **queue, queue_t *elem) ;

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_remove (queue_t **queue, queue_t *elem) ;
