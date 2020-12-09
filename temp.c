    // New algorithm for more efficient refreshing threads but it does not work right now.
    
    // First try to activate the maximum allowable amount of threads
    for (multiple = 2; multiple <= NROF_PIECES; multiple++) {
        // Start thread for the current multiple //
        parameters[threadcount] = multiple;

        // Get default attributes for thread
        pthread_attr_init(&attr[threadcount]);
        // Create thread
        pthread_create(&tid[threadcount], NULL, flip, &parameters[threadcount]);
        threadcount++;
        // If the maximum number of threads are active, go to the next loop that will refresh finished threads.
        if (threadcount >= NROF_THREADS) { 
            break;
        }
    }

    // Keep refreshing longest running thread
    for (; multiple <= NROF_PIECES; multiple++) {
        // Start try to join first thread, since this one has had the longest to run.
        pthread_join(tid[indexToRefresh], NULL);
        // After joining, remove a thread from the count
        threadcount--;
        // Check if a thread can be added
        if (threadcount < NROF_THREADS) {
            // Start thread for the current multiple //
            parameters[indexToRefresh] = multiple;
            // Create thread
            pthread_create(&tid[indexToRefresh], NULL, flip, &parameters[indexToRefresh]);
            // Increase thread count
            threadcount++;
        }

        // Go to the next longest running thread
        indexToRefresh++;
        // Do modulo to keep looping in the possible indexes of threads
        indexToRefresh = indexToRefresh % (NROF_THREADS - 1);
    }

    // Wait for all threads to finish.
    for (i = 0; i < threadcount; i++) {
        pthread_join(tid[i], NULL);
    }