// Main File - rwlock.c
// Ishika Pol - CSE130
// Implementation of a reader-writer lock with priority levels, N-Way priority, and thread synchronization using pthreads.

// Citation: Used pseudocode from Mitchell's section

#include "rwlock.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct rwlock {
    int priority; // Priority level (READERS, WRITERS, N_WAY)
    int n; // Parameter for N_WAY priority
    int active_readers; // Number of currently active readers
    int active_writers; // Number of currently active writers
    int waiting_readers; // Number of readers waiting to acquire the lock
    int waiting_writers; // Number of writers waiting to acquire the lock
    int nway_count; // Counter for N_WAY priority
    bool flag; // Flag for N_WAY priority
    pthread_mutex_t mutex; // Mutex for controlling access to the lock
    pthread_cond_t readers_cond; // Condition variable for readers
    pthread_cond_t writers_cond; // Condition variable for writers
} rwlock_t;

// Create a new read-write lock with the specified priority type
rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {
    rwlock_t *rw = (rwlock_t *) malloc(sizeof(rwlock_t));
    if (rw == NULL) {
        fprintf(stderr, "Failed to allocate memory for rwlock.\n");
        exit(EXIT_FAILURE);
    }

    rw->priority = p;
    rw->n = n;
    rw->active_readers = 0;
    rw->active_writers = 0;
    rw->waiting_readers = 0;
    rw->waiting_writers = 0;
    rw->nway_count = 0;
    rw->flag = false;

    // Initialize the mutex for synchronization
    pthread_mutex_init(&(rw->mutex), NULL);

    // Initialize condition variables for readers and writers
    pthread_cond_init(&(rw->readers_cond), NULL);
    pthread_cond_init(&(rw->writers_cond), NULL);

    return rw;
}

// Delete the reader-writer lock and release associated resources
void rwlock_delete(rwlock_t **rw) {
    if (rw == NULL || *rw == NULL) {
        return;
    }

    // Retrieve the reader-writer lock from the pointer
    rwlock_t *lock = *rw;

    // Destroy the associated mutex, signaling condition variables, and free any dynamically allocated memory
    pthread_mutex_destroy(&(lock->mutex));
    pthread_cond_destroy(&(lock->readers_cond));
    pthread_cond_destroy(&(lock->writers_cond));

    // Free the memory occupied by the reader-writer lock
    free(lock);
    *rw = NULL;
}

// Helper function that returns the count of threads waiting for the lock based on the lock's priority and current state
int reader_wait(rwlock_t *rw) {
    // If priority is readers, it returns the count of active writers
    if (rw->priority == READERS) {
        return rw->active_writers;
    }
    // If priority is writers, it returns the count of waiting writers
    else if (rw->priority == WRITERS) {
        return rw->waiting_writers;
    }
    // If priority is N_WAY, it considers the number of active readers, waiting writers, and a condition based on nway_count and flag
    else if (rw->priority == N_WAY) {
        if (rw->active_readers > 0) {
            if (rw->waiting_writers > 0) {
                if (rw->nway_count < rw->n) {
                    return 0;
                } else {
                    if (rw->flag == true) {
                        return rw->waiting_writers;
                    }
                }
            } else {
                return 0;
            }
        } else if (rw->active_writers > 0) {
            return rw->active_writers;
        } else {
            if (rw->flag) {
                return rw->waiting_writers;
            } else {
                return 0;
            }
        }
    } else {
        fprintf(stderr, "Reader wait failed. Unknown priority.\n");
        return -1;
    }
    return -1;
}

// Helper function that determines the number of active readers, active writers, or waiting readers based on the priority and the current state of the read-write lock
int writer_wait(rwlock_t *rw) {
    // If the priority is readers, returns the number of active readers
    if (rw->priority == READERS) {
        return rw->active_readers;
    }
    // If the priority is writers, returns the number of active writers
    else if (rw->priority == WRITERS) {
        return rw->active_writers;
    }
    // If priority is N_WAY, it considers the number of active readers, active writers, and a condition based on the flag
    else if (rw->priority == N_WAY) {
        if (rw->active_readers > 0) {
            return rw->active_readers;
        } else if (rw->active_writers) {
            return rw->active_writers;
        } else {
            if (rw->flag == false) {
                return rw->waiting_readers;
            } else {
                return 0;
            }
        }
    } else {
        fprintf(stderr, "Writer wait failed. Unknown priority.\n");
        return -1;
    }
}

// Acquires a reader lock in a reader-writer lock
void reader_lock(rwlock_t *rw) {
    // Acquire the mutex to ensure exclusive access to the shared resources in the rwlock
    pthread_mutex_lock(&(rw->mutex));

    // Increment the count of waiting readers
    rw->waiting_readers++;

    // Wait until it is safe for the reader to proceed
    while (reader_wait(rw)) {
        pthread_cond_wait(&(rw->readers_cond), &(rw->mutex));
    }

    // Decrease the count of waiting readers and increase the count of active readers
    rw->waiting_readers--;
    rw->active_readers++;

    // Check for priority and N_WAY flag for advanced reader-writer lock behavior
    if (rw->priority == N_WAY) {
        // If the flag is true and nway_count is 0, reset the flag to false
        if (rw->flag == true && rw->nway_count == 0) {
            rw->flag = false;
        }

        // If the flag is false and nway_count is less than the total number of readers, increment nway_count
        if (rw->flag == false && rw->nway_count < rw->n) {
            rw->nway_count++;
        }
    }

    // Release the mutex, allowing other threads to acquire the lock
    pthread_mutex_unlock(&(rw->mutex));
}

// Releases a reader lock in a reader-writer lock
void reader_unlock(rwlock_t *rw) {
    // Acquire the mutex to ensure exclusive access to the shared resources in the rwlock
    pthread_mutex_lock(&(rw->mutex));

    // Decrease the count of active readers
    rw->active_readers--;

    // Check for priority and N_WAY flag for advanced reader-writer lock behavior
    if (rw->priority == N_WAY) {
        // If nway_count equals the total number of readers, set the flag to true
        if (rw->nway_count == rw->n) {
            rw->flag = true;
        }
        // If the flag is true and nway_count is greater than 0, reset nway_count to 0
        if (rw->flag == true && rw->nway_count > 0) {
            rw->nway_count = 0;
        }
    }

    // Wake up all waiting readers
    pthread_cond_broadcast(&(rw->readers_cond));
    // Wake up all waiting writers
    pthread_cond_broadcast(&(rw->writers_cond));

    // Release the mutex, allowing other threads to acquire the lock
    pthread_mutex_unlock(&(rw->mutex));
}

// Acquires a writer lock in a reader-writer lock
void writer_lock(rwlock_t *rw) {
    // Acquire the mutex to ensure exclusive access to the shared resources in the rwlock
    pthread_mutex_lock(&(rw->mutex));

    // Increment the count of waiting writers
    rw->waiting_writers++;

    // Wait until it is safe for the writer to proceed
    while (writer_wait(rw)) {
        pthread_cond_wait(&(rw->writers_cond), &(rw->mutex));
    }

    // Decrease the count of waiting writers and increase the count of active writers
    rw->waiting_writers--;
    rw->active_writers++;

    // Release the mutex, allowing other threads to acquire the lock
    pthread_mutex_unlock(&(rw->mutex));
}

// Releases a writer lock in a reader-writer lock
void writer_unlock(rwlock_t *rw) {
    // Acquire the mutex to ensure exclusive access to the shared resources in the rwlock
    pthread_mutex_lock(&(rw->mutex));

    // Decrease the count of active writers
    rw->active_writers--;

    // Check for priority and N_WAY flag for advanced reader-writer lock behavior
    if (rw->priority == N_WAY) {
        // If the flag is true, reset it to false
        if (rw->flag == true) {
            rw->flag = false;
        }
    }

    // Wake up all waiting readers
    pthread_cond_broadcast(&(rw->readers_cond));
    // Wake up all waiting writers
    pthread_cond_broadcast(&(rw->writers_cond));

    // Release the mutex, allowing other threads to acquire the lock
    pthread_mutex_unlock(&(rw->mutex));
}
