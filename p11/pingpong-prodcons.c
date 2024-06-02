#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "queue.h"

typedef struct item_t {
    struct item_t *prev, *next;
    int value;
} item_t;

#define NUM_CONSUMERS 2
#define NUM_PRODUCERS 3
#define NUM_SLOTS 5

item_t *buffer;

int num_items = 0;
int num_slots = NUM_SLOTS;

semaphore_t s_buff, s_vaga, s_item;

int rand_range(int min, int max) {
    return (rand() % (max - min)) + min;
}

item_t *produce_item(long id) {
    item_t *item = malloc(sizeof(item_t));
    if (item == NULL) {
        return NULL;
    }
    item->next = NULL;
    item->prev = NULL;
    item->value = rand_range(0, 99);

    // printf("P%ld produziu %d\n", id, item->value);
    return item;
}

void consume_item(long id, item_t *item) {
    // printf("C%ld consumiu %d\n", id, item->value);
    free(item);
}

void producer(void *arg) {
    long id = (long) arg;

    while (1) {
        task_sleep(1000);

        item_t *item = produce_item(id);

        sem_down(&s_vaga);
        sem_down(&s_buff);

        // coloca no buffer
        queue_append((queue_t **) &buffer, (queue_t *) item);
        num_items++;
        num_slots--;

        printf("P%ld inseriu %d (%d itens, %d vagas)\n", 
               id, item->value, num_items, num_slots);

        sem_up(&s_buff);
        sem_up(&s_item);
    }
}

void consumer(void *arg) {
    long id = (long) arg;

    while (1) {
        sem_down(&s_item);
        sem_down(&s_buff);

        // pega do buffer
        item_t *first = buffer;
        queue_remove((queue_t **) &buffer, (queue_t *) first);
        num_items--;
        num_slots++;

        printf("\t\t\t\tC%ld retirou %d (%d itens, %d vagas)\n", 
               id, first->value, num_items, num_slots);

        sem_up(&s_buff);
        sem_up(&s_vaga);

        consume_item(id, first);

        task_sleep(1000);
    }
}

int main() {
    printf("main: inicio\n");

    ppos_init();

    task_t producers[NUM_PRODUCERS];
    task_t consumers[NUM_CONSUMERS];

    // incializando os semaforos
    sem_init(&s_vaga, NUM_SLOTS);
    sem_init(&s_item, 0);
    sem_init(&s_buff, 1);

    // incializando a tarefas
    for (long i = 0; i < NUM_PRODUCERS; i++) {
        task_init(&producers[i], producer, (void *) i);
    }

    for (long i = 0; i < NUM_CONSUMERS; i++) {
        task_init(&consumers[i], consumer, (void *) i);
    }

    // esperando as tarefas terminarem
    for (long i = 0; i < NUM_PRODUCERS; i++) {
        task_wait(&producers[i]);
    }

    for (long i = 0; i < NUM_CONSUMERS; i++) {
        task_wait(&consumers[i]);
    }

    // destruindo as semaforos
    sem_destroy(&s_buff);
    sem_destroy(&s_item);
    sem_destroy(&s_vaga);
    
    printf("main: fim\n");
    task_exit(0);
}