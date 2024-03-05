#include "queue.h"
#include <stdio.h>

#define ERROR_APPEND_QUEUE_DOESNT_EXIST -1
#define ERROR_APPEND_ELEM_DOESNT_EXIST -2
#define ERROR_APPEND_ANOTHER_QUEUE -3


//------------------------------------------------------------------------------
// Funções auxiliares
int queue_is_empty (queue_t *queue) {
    return (queue == NULL);
}

void queue_remove_elem(queue_t *elem) {
    queue_t *prev = elem->prev;
    queue_t *next = elem->next;
    prev->next = elem->next;
    next->prev = elem->prev;
    elem->next = NULL;
    elem->prev = NULL;
}

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue) {
    if (queue_is_empty(queue)) {
        return 0;
    }
    
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
    if (queue_is_empty(queue)) {
        printf("%s: []\n", name);
        return;
    }

    printf("%s: [", name);

    queue_t *aux = queue;
    while (aux->next != queue) {
        print_elem((void *) aux);
        printf(" ");
        aux = aux->next;
    }
    // printing last element
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

int queue_append (queue_t **queue, queue_t *elem) {
    if (queue == NULL) {
        fprintf(stderr, "Error: queue append - the queue doesn't exist.\n");
        return -1;
    }
    if (elem == NULL) {
        fprintf(stderr, "Error: queue append - the element doesn't exist.\n");
        return -2;
    }

    // percorre a fila verificando se o elemento ja faz parte dela
    queue_t *aux = (*queue);
    while (!queue_is_empty(*queue) && (aux->next != (*queue))) {
        if (aux == elem) {
            fprintf(stderr, "Error: queue append - the element is already in the queue.\n");
            return -3;
        }
        aux = aux->next;
    }
    // verificando ultimo elemento
    if (aux == elem) {
        fprintf(stderr, "Error: queue append - the element is already in the queue.\n");
        return -3;
    }

    // verificar se está em algum outra fila
    if (elem->prev || elem->next) {
        fprintf(stderr, "Error: queue append - the element is in another queue.\n");
        return -4;
    }

    // inserindo no inicio da fila vazia
    if (queue_is_empty(*queue)) {
        elem->next = elem;
        elem->prev = elem;
        (*queue) = elem;
        return 0;
    }

    // inserindo no final da fila 
    queue_t *prev = (*queue)->prev;
    queue_t *next = (*queue);
    prev->next = elem;
    next->prev = elem;
    elem->next = next;
    elem->prev = prev;
    
    return 0;
}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_remove (queue_t **queue, queue_t *elem) {
    if (queue == NULL) {
        fprintf(stderr, "Error: queue remove - the queue doesn't exist.\n");
        return -1;
    }
    if ((*queue) == NULL) {
        fprintf(stderr, "Error: queue remove - the queue is empty.\n");
        return -2;
    }
    if (elem == NULL) {
        fprintf(stderr, "Error: queue remove - the element doesn't exist.\n");
        return -3;
    }

    // removendo do inicio da fila
    if ((*queue) == elem) {
        // fila com 1 elemento
        if ((elem->next == (*queue)) && (elem->prev == (*queue))) {
            elem->next = NULL;
            elem->prev = NULL;
            (*queue) = NULL;
        }
        // fila com mais de 1 elemento
        else {
            (*queue) = (*queue)->next;
            queue_remove_elem(elem);
        }
        return 0;
    }

    // removendo do meio da fila
    // precorre a fila procurando pelo elemento a remover
    queue_t *aux = (*queue);
    while (aux->next != (*queue)) {
        if (aux == elem) {
            queue_remove_elem(elem);
            return 0;
        }
        aux = aux->next;
    }
    // verificando o ultimo elemento
    if (aux == elem) {
        queue_remove_elem(elem);
        return 0;
    }
    // se não encontrou o elemento, não esta na fila
    fprintf(stderr, "Error: queue remove - the element is not in the queue.\n");
    return -4;
}
