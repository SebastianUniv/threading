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
#include <time.h> // for timing application
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

// declare a mutex, and it is initialized as well
static pthread_mutex_t      mutex_flip          = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t      thread_init          = PTHREAD_MUTEX_INITIALIZER;
// Semaphore used for controlling number of active threads
sem_t sem;

// Define function that is used by threads
void *flip(void *multiple);

int main (void)
{   
    // Count time consumed by program, set starting time
    clock_t begin = clock();
    // Thread id
    pthread_t tid;
    // Thread attribute
    pthread_attr_t tattr;
    // Thread parameter
    int parameter;
    // Iterator value
    int i;
    // Multiple value (skip 1)
    int multiple;

    // Initialize buffer: Set all bits (elements) to 1
    for (i = 0; i < ((NROF_PIECES/128) + 1); i++) {
        buffer[i] = ~0;
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
        // Lock so the thread parameter is secured
        pthread_mutex_lock (&thread_init);
        parameter = multiple;
        // Create thread
        pthread_create(&tid, &tattr, flip, &parameter);
    }

    // Wait for all threads to finish [Probably not needed]
    for (i = 0; i < NROF_THREADS; i++) {
        sem_wait(&sem);
        sem_destroy(&sem);
    }

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
                if (value > NROF_PIECES) {
                    goto finish;
                }
                // Print value
                printf("%d\n", value);
            }
        }
    }
    finish: ;
    // Set end time
    clock_t end = clock();
    // Calculate time spent and print this to console
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent: %f\n", time_spent);
    // End of program
    return (0);
}

// Flips elements of buffer according to multiple
void *flip(void *arg)
{
    // Retrieve pointer value and store it as integer.
    int multiple = *(int *) arg;
    // Parameter is no longer crucial and can be freed
    pthread_mutex_unlock (&thread_init);

    int i;
    for (i = 0; i < ((NROF_PIECES/128) + 1); i++) {
        int bit;
        for (bit = 0; bit < 128; bit++) {
            // Convert current element to decimal value and check if it can be divided by the current multiple.
            int value = 128 * i + bit;
            if (value % multiple == 0) {
                // Flip bit
                pthread_mutex_lock (&mutex_flip);
                if (BIT_IS_SET(buffer[i], bit)) {
                    BIT_CLEAR(buffer[i], bit);
                } else {
                    BIT_SET(buffer[i], bit);
                }
                pthread_mutex_unlock (&mutex_flip);
            }
        }
    }

    // Increase semaphore since this thread has finished
    sem_post(&sem);
    // Exit thread
    pthread_exit(0);
}

