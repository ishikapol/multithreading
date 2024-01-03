// Main File - queue.c
// Ishika Pol - CSE130
// Implementation of a thread-safe queue using semaphores for controlling access and tracking empty and full slots.

// Citation: Used pseudocode from lecture slides

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <semaphore.h>
#include "queue.h"

typedef struct queue {
    int size; // Maximum size of the queue
    int tail; // Index pointing to the next available slot to enqueue
    int head; // Index pointing to the next available slot to dequeue
    void **buffer; // The array to hold the elements
    sem_t mutex; // Semaphore for controlling access to the queue
    sem_t full_slots; // Semaphore for tracking available slots for elements
    sem_t empty_slots; // Semaphore for tracking empty slots in the queue
} queue_t;

// Create a new queue with the specified size
queue_t *queue_new(int size) {
    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    if (q == NULL) {
        fprintf(stderr, "Failed to allocate memory for queue.\n");
        exit(EXIT_FAILURE);
    }

    q->size = size;
    q->tail = 0;
    q->head = 0;

    // Allocate memory for the element buffer
    q->buffer = malloc(sizeof(void **) * size);

    // Initialize mutex to control access (only one thread at a time)
    sem_init(&(q->mutex), 0, 1);
    // Initialize full_slots to 'size' indicating no elements initially
    sem_init(&(q->full_slots), 0, size);
    // Initialize empty_slots to '1' indicating one empty slot initially
    sem_init(&(q->empty_slots), 0, 0);

    return q;
}

// Delete the queue and release associated resources
void queue_delete(queue_t **q) {
    if (!q) {
        fprintf(stderr, "Error in queue_delete(). Queue doesn't exist.");
        exit(EXIT_FAILURE);
    }
    if (q && *q) {
        // Free the memory used by the element buffer
        if ((*q)->buffer != NULL) {
            free((*q)->buffer);
        }
        (*q)->tail = 0;
        (*q)->head = 0;

        sem_destroy(&((*q)->full_slots));
        sem_destroy(&((*q)->empty_slots));
        sem_destroy(&((*q)->mutex));

        // Free the memory used by the queue
        free(*q);
        *q = NULL;
    }
}

// Push an element onto the queue
bool queue_push(queue_t *q, void *element) {
    if (q == NULL) {
        return false;
    }

    // Wait for an available slot to push the element
    sem_wait(&(q->full_slots));
    // Acquire the mutex for safe access to the queue
    sem_wait(&(q->mutex));

    // Add the element to the queue
    q->buffer[q->tail] = element;
    q->tail = (q->tail + 1) % q->size;

    // Release the mutex
    sem_post(&(q->mutex));
    // Signal that an empty slot is filled
    sem_post(&(q->empty_slots));

    return true;
}

// Pop an element from the queue
bool queue_pop(queue_t *q, void **element) {
    if (q == NULL) {
        return false;
    }

    // Wait for an element to be available
    sem_wait(&(q->empty_slots));
    // Acquire the mutex for safe access to the queue
    sem_wait(&(q->mutex));

    // Retrieve the element from the queue
    *element = q->buffer[q->head];
    q->head = (q->head + 1) % q->size;

    // Release the mutex
    sem_post(&(q->mutex));
    // Signal that a full slot is emptied
    sem_post(&(q->full_slots));

    return true;
}
