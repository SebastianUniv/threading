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
#include <mqueue.h> 

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
// Queue for finished threads
static char q_finishedThreads[80];
// Message structure for queue
typedef struct {
    int                     arrayIndex;
} Q_THREAD;
// Struct for passing arguments to thread
struct thread_data
{
  int  multiple;
  int arrayIndex;
};

// Define function that is used by threads
void *flip(void *multiple);

int main (void)
{   
    // Count time consumed by program, set starting time
    clock_t begin = clock();
    // Variables used by queue
    mqd_t q_fd_finishedThreads;
    Q_THREAD  threadSignal;
    struct mq_attr q_attr;
    // Thread count
    int threadcount = 0;
    // Thread ids
    pthread_t tid[NROF_THREADS];
    // Thread attributes
    pthread_attr_t attr[NROF_THREADS];
    // Thread parameter
    struct thread_data para[NROF_THREADS];
    // Iterator value
    int i;
    // Multiple value (skip 1)
    int multiple;
    
    // Initialize queue
    snprintf(q_finishedThreads, 80,"/q_finishedthreads_%d", getpid());
    q_attr.mq_maxmsg = NROF_THREADS;
    q_attr.mq_msgsize = sizeof (Q_THREAD);
    q_fd_finishedThreads = mq_open (q_finishedThreads, O_RDWR | O_CREAT | O_EXCL, 0600, &q_attr);

    // Initialize buffer: Set all bits (elements) to 1
    for (i = 0; i < ((NROF_PIECES/128) + 1); i++) {
        buffer[i] = ~0;
    }

    for (multiple = 2; multiple < NROF_PIECES; multiple++) {
        // Check if the maximum number of threads is already in use
        if (threadcount < NROF_THREADS) {
            // If not, make a new thread
            para[threadcount].multiple = multiple;
            para[threadcount].arrayIndex = threadcount;
            pthread_attr_init(&attr[threadcount]);
            pthread_create(&tid[threadcount], &attr[threadcount], flip, &para[threadcount]);
            threadcount++;
        } else {
            // If maximum reached, start refreshing finished threads.
            // Get number of current messages (finished threads) in queue
            mq_getattr(q_fd_finishedThreads, &q_attr);
            // Loop over all messages in queue (= finished threads)
            for (i = 0; i < q_attr.mq_msgsize; i++) {
                // Get id of thread that is finished
                mq_receive (q_finishedThreads, (char *) &threadSignal, sizeof (Q_THREAD), NULL);
                // Join thread
                pthread_join(tid[threadSignal.arrayIndex], NULL);
                // Set the parameters of the index to refresh to new values
                para[threadSignal.arrayIndex].multiple = multiple;
                para[threadSignal.arrayIndex].arrayIndex = threadSignal.arrayIndex;
                // Make new thread
                pthread_attr_init(&attr[threadSignal.arrayIndex]);
                pthread_create(&tid[threadSignal.arrayIndex], &attr[threadSignal.arrayIndex], flip, &para[threadSignal.arrayIndex]);

                // Check if there are more messages in the queue to iterate over
                if (q_attr.mq_msgsize > (i + 1)) {
                    // If so, increase multiple and continue refreshing threads
                    multiple++;
                    // If multiple is now larger than NROF_PIECES we are done
                    if (multiple > NROF_PIECES) {
                        break;
                    }
                }
            }
        }
    }

    // Wait for all threads to finish.
    for (i = 0; i < threadcount; i++) {
        pthread_join(tid[i], NULL);
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
    struct thread_data *parameters;
    parameters  = (struct thread_data *) arg;
    Q_THREAD  threadSignal;
    //printf("%d\n", multiple);
    int i;
    for (i = 0; i < ((NROF_PIECES/128) + 1); i++) {
        int bit;
        for (bit = 0; bit < 128; bit++) {
            // Convert current element to decimal value and check if it can be divided by the current multiple.
            int value = 128 * i + bit;
            if (value % (int) parameters->multiple == 0) {
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

    // Get the index of this thread, send it to the queue to indicate it is close to finishing
    threadSignal.arrayIndex = parameters->arrayIndex;
    mq_send(q_finishedThreads, (const char *) &threadSignal, sizeof(Q_THREAD), 0);
    // Exit thread
    pthread_exit(0);
}

