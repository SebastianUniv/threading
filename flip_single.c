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

// Define function that is used by threads
void *flip(void *multiple);

int main (void)
{
    // Thread id
    pthread_t tid;
    pthread_attr_t attr;
    // Iterator value
    int i;
    // Multiple value (skip 1)
    int multiple;

    // Initialize buffer: Set all bits (elements) to 1
    for (i = 0; i < ((NROF_PIECES/128)); i++) {
        buffer[i] = ~0;
    }

    // Get default attributes for thread
    pthread_attr_init(&attr);

    // Flip elements of the buffer according to the current multiple
    for (multiple = 2; multiple < NROF_PIECES; multiple++) {
        // Start thread for the current multiple
        pthread_create(&tid, &attr, flip, multiple);
        pthread_join(tid, NULL);
    }

    // Print all elements in buffer which are 1 -> convert elements to decimal notation.
    // Iterate over all buffer indexes
    int count = 0;
    for (i = 0; i < ((NROF_PIECES/128)); i++) {
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
                count++;
            }
        }
    }
    finish: ;
    printf("%d\n", count);
    // End of program
    return (0);
}

// Flips elements of buffer according to multiple
void *flip(void *multiple)
{
    int i;
    for (i = 0; i < ((NROF_PIECES/128)); i++) {
        int bit;
        for (bit = 0; bit < 128; bit++) {
            // Convert current element to decimal value and check if it can be divided by the current multiple.
            int value = 128 * i + bit;
            if (value % (int) multiple == 0) {
                // Flip bit
                if (BIT_IS_SET(buffer[i], bit)) {
                    BIT_CLEAR(buffer[i], bit);
                } else {
                    BIT_SET(buffer[i], bit);
                }
            }
        }
    }

    pthread_exit(0);
}