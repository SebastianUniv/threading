/* 
 * Operating Systems <2INCO> Practical Assignment
 * Threaded Application
 *
 * Oscar Robben (1248278)
 * Bas Gerritsen (1333038)
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
#include <string.h>

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
static pthread_mutex_t      mutex_flip[(NROF_PIECES/128) + 1]; //= PTHREAD_MUTEX_INITIALIZER;

// Semaphore used for controlling number of active threads
sem_t sem;

// free Index
int freeIndex[NROF_THREADS] = { 0 };

// Struct for passing parameters to thread
struct thread_data
{
  int thread_id;
  int multiple;
};

// Define function that is used by threads
void *flip(void *multiple);

int main (void)
{   
    // Thread id
    pthread_t tid[NROF_THREADS];
    // Thread attribute
    pthread_attr_t tattr;
    // Thread parameter
    struct thread_data thread_data_array[NROF_THREADS];
    // Iterator value (just declared once and then reused in multiple for loops, doesn't have any further meaning)
    int i;
    // Multiple value to flip bits by (skip 1)
    int multiple;

    // Initialize buffer (Set all bits (elements) to 1) and mutex locks
    for (i = 0; i < ((NROF_PIECES/128) + 1); i++) {
        buffer[i] = ~0;
    }

    // Initialize mutexes 
    for (i = 0; i < ((NROF_PIECES/128) + 1); i++) {
        pthread_mutex_init(&(mutex_flip[i]), NULL);
    }
        
    // Initialize semaphore, set max NROF_THREADS
    sem_init(&sem, 0, NROF_THREADS);
    // Set thread attribute to detached (pthread_join not needed)
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    // Start creating threads for every multiple
    for (multiple = 2; multiple < NROF_PIECES; multiple++) {
        // Wait for resource to become free (if necessary)
        sem_wait(&sem);
        // Index of thread_data_array to be used.
        int curIndex;
        // Loop over all indexes to see which one is freed
        for (curIndex = 0; curIndex < NROF_THREADS; curIndex++) {
            // If index is free (= 0), then set the parameters of the array at this index
            if (freeIndex[curIndex] == 0) {
                // Set index to in use.
                freeIndex[curIndex] = 1;
                // Set parameters
                thread_data_array[curIndex].multiple = multiple;
                thread_data_array[curIndex].thread_id = curIndex;
                // Start thread
                pthread_create(&tid[curIndex], &tattr, flip, (void *) &thread_data_array[curIndex]);
                // A free index has been intialized to busy for this multiple, so we can continue to the next multiple
                break;
            }
        }
    }

    // Wait for all threads to finish [Probably not needed]
    for (i = 0; i < NROF_THREADS; i++) {
        sem_wait(&sem);
    }

    // Remove semaphore
    sem_destroy(&sem);
    // Remove mutex locks
    pthread_mutex_destroy(mutex_flip);

    // Print all elements in buffer which are 1 -> convert elements to decimal notation.
    // Iterate over all buffer indexes
    for (i = 0; i < ((NROF_PIECES/128) + 1); i++) {
        // Iterate over all elements in buffer
        int bit;
        for (bit = 0; bit < 128; bit++) {
            // Check if bit is set, if so convert to decimal.
            if (BIT_IS_SET(buffer[i], bit)) {
                int value = 128 * i + bit;
                // Check if value is over max number of pieces, if so stop printing.
                if (value >= NROF_PIECES) {
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
void *flip(void *args)
{
    // Retrieve pointer value and store it as integer.
    int thread_id;
    int multiple;

    struct thread_data *parameters;
    parameters = (struct thread_data *) args;
    thread_id = parameters->thread_id;
    multiple = parameters->multiple;

    // Index is no longer needed, so set it to free. 
    // (Lock is not needed because no two threads will ever acces the same index at the same time)
    freeIndex[thread_id] = 0;

    int i;
    for (i = 0; i < ((NROF_PIECES/128) + 1); i++) {
        int bit;
        for (bit = 0; bit < 128; bit++) {
            // Convert current element to decimal value and check if it can be divided by the current multiple.
            int value = 128 * i + bit;
            if (value % multiple == 0) {
                // Flip bit
                // Request lock so there will be no race conditions
                pthread_mutex_lock (&mutex_flip[i]);
                if (BIT_IS_SET(buffer[i], bit)) {
                    BIT_CLEAR(buffer[i], bit);
                } else {
                    BIT_SET(buffer[i], bit);
                }
                pthread_mutex_unlock (&mutex_flip[i]);
            }
        }
    }
    
    // Increase semaphore since this thread has finished
    sem_post(&sem);
    // Exit thread
    pthread_exit(0);
}

