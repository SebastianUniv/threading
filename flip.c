/* 
 * Operating Systems <2INCO> Practical Assignment
 * Threaded Application
 *
 * STUDENT_NAME_1 (STUDENT_NR_1)
 * STUDENT_NAME_2 (STUDENT_NR_2)
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * Extra steps can lead to higher marks because we want students to take the initiative. 
 * 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>          // for perror()
#include <pthread.h>
#include <semaphore.h>

#include "uint128.h"
#include "flip.h"

// create a bitmask where bit at position n is set
#define BITMASK(n)          (((uint128_t) 1) << (n))

// check if bit n in v is set
#define BIT_IS_SET(v,n)     (((v) & BITMASK(n)) == BITMASK(n))

// set bit n in v
#define BIT_SET(v,n)        ((v) =  (v) |  BITMASK(n))

// clear bit n in v
#define BIT_CLEAR(v,n)      ((v) =  (v) & ~BITMASK(n))

// Declare mutex used to secure bit flips
pthread_mutex_t      mutex_flip[NROF_PIECES];

// Declare mutex used to secure parameter transmission from main thread to child thread
static pthread_mutex_t      thread_init[NROF_THREADS]          = PTHREAD_MUTEX_INITIALIZER;

// Semaphore used for controlling number of active threads
sem_t sem;

// Struct for passing parameters to thread
struct thread_data
{
  int lockIndex;
  int multiple;
};

// Define function that is used by threads
void *flip(void *multiple);

int main (void)
{   
    // Thread id
    pthread_t tid;
    // Thread attribute
    pthread_attr_t tattr;
    // Thread parameter
    struct thread_data para[NROF_THREADS];
    // Iterator value (just declared once and then reused in multiple for loops, doesn't have any further meaning)
    int i;
    // Multiple value to flip bits by (skip 1)
    int multiple;
    // Index of thread_init mutex lock to be used, this is used so other threads don't have to wait for a single parameter pointer to be freed.
    int curIndex = 0;

    // Initialize buffer (Set all bits (elements) to 1) and mutex locks
    for (i = 0; i < ((NROF_PIECES/128)); i++) {
        buffer[i] = ~0;
    }

    // Initialize mutex locks
    for (i = 0; i < NROF_PIECES; i++) {
        pthread_mutex_init(&mutex_flip[i], NULL);
    }

    // Initialize semaphore, set max NROF_THREADS
    sem_init(&sem, 0, NROF_THREADS);
    // Set thread attribute to detached (pthread_join not needed)
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);

    // Start creating threads for every multiple
    for (multiple = 2; multiple < NROF_PIECES; multiple++) {
        // Wait for resource to become available
        sem_wait(&sem);
        // Lock so the thread parameter is secured, use lock with curIndex
        pthread_mutex_lock (&thread_init[curIndex]);
        // Load values in parameter struct, to be sent to thread
        para[curIndex].lockIndex = curIndex;
        para[curIndex].multiple = multiple;
        // Create thread
        pthread_create(&tid, &tattr, flip, &para[curIndex]);
        // Increase curIndex so next thread can immediatly be created (does not have to wait for unlock of curIndex mutex lock)
        curIndex++;
        // Modulo since the array is finite, this way it loops over the array
        curIndex = curIndex % NROF_THREADS;
    }

    // Wait for all threads to finish [Probably not needed]
    for (i = 0; i < NROF_THREADS; i++) {
        sem_wait(&sem);
    }

    // Remove semaphore
    sem_destroy(&sem);

    // Print all elements in buffer which are 1 -> convert elements to decimal notation.
    // Iterate over all buffer indexes
    for (i = 0; i < ((NROF_PIECES/128)); i++) {
        // Iterate over all elements in buffer
        int bit;
        for (bit = 0; bit < 127; bit++) {
            // Check if bit is set, if so convert to decimal.
            if (BIT_IS_SET(buffer[i], bit)) {
                int value = 128 * i + bit;
                // Check if value is over max number of pieces, if so stop printing.
                if (value > NROF_PIECES) {
                    goto finish;
                }
                // Skip zero
                if (value == 0) {
                    continue;
                }
                // Print value
                printf("%d\n", value);
            }
        }
    }
    finish: ;
    // End of program
    return (0);
}

// Flips elements of buffer according to multiple
void *flip(void *arg)
{
    // Retrieve pointer value and store it as integer.
    struct thread_data *parameters;
    parameters  = (struct thread_data *) arg;
    // Parameter is no longer crucial and can be freed
    pthread_mutex_unlock (&thread_init[parameters->lockIndex]);

    int i;
    for (i = 0; i < ((NROF_PIECES/128)); i++) {
        int bit;
        for (bit = 0; bit < 127; bit++) {
            // Convert current element to decimal value and check if it can be divided by the current multiple.
            int value = 128 * i + bit;
            if (value % parameters->multiple == 0) {
                // Flip bit
                pthread_mutex_lock (&mutex_flip[value]);
                if (BIT_IS_SET(buffer[i], bit)) {
                    BIT_CLEAR(buffer[i], bit);
                } else {
                    BIT_SET(buffer[i], bit);
                }
                pthread_mutex_unlock (&mutex_flip[value]);
            }
        }
    }
    // Increase semaphore since this thread has finished
    sem_post(&sem);
    // Exit thread
    pthread_exit(0);
}

