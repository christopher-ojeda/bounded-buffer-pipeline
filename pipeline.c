/*
 * ===============================================================================
 * Student Implementation Template
 *
 * INSTRUCTIONS:
 * Complete the implementation of all functions marked with TODO below.
 * 💡 Follow the bounded buffer (producer-consumer) algorithm specifications in the lecture slides.
 *
 * REQUIREMENTS:
 * - Implement bounded buffer operations with counting semaphores and mutex
 * - Implement generator thread (producer for buffer1)
 * - Implement processor thread (consumer + producer)
 * - Implement writer thread (consumer for buffer2)
 * - Use semaphores for all coordination (no manual counters)
 * - Ensure results are written in correct order (by ID)
 * - Use proper synchronization to avoid race conditions and deadlocks
 *
 *💡 CRITICAL SYNCHRONIZATION PATTERN (Bounded Buffer Algorithm)
 * PRODUCER OPERATION
 * 1. Wait until a buffer slot becomes available
 * 2. Enter the critical section protecting the buffer
 * 3. Insert the item into the buffer
 * 4. Leave the critical section
 * 5. Signal that a new item is available

 * CONSUMER OPERATION
 *  1. Wait until an item becomes available
 *  2. Enter the critical section protecting the buffer
 *  3. Remove the item from the buffer
 *  4. Leave the critical section
 *  5. Signal that a buffer slot is now free
 *
 * ⚠️ GRADING CRITERIA:
 * - Correct bounded buffer implementation
 * - Proper semaphore usage
 * - Proper mutex usage
 * - No race conditions or deadlocks
 * - Correct work distribution
 * - Writer maintains correct order
 * - Code compiles cleanly (no warnings or errors)
 */

#include "pipeline.h"

// Allow access to input array loaded in driver.c
extern NumberData input_array[MAX_N];

/* ========================================================================================
 ⚠️BUFFER MANAGEMENT FUNCTIONS - YOU MUST IMPLEMENT THESE 4 FUNCTIONS ONLY! 
=================================================================++=======================*/

/**
 * @brief Initialize a bounded buffer with semaphores and mutex
 * @param buf Pointer to BoundedBuffer structure
 * @param capacity Number of slots in buffer
 */
void init_buffer(BoundedBuffer *buf, int capacity) {
   // Initialize indices
    buf->in = 0;
    buf->out = 0;

    // Initialize semaphores
    sem_init(&buf->empty, 0, capacity);
    sem_init(&buf->full, 0, 0);

    // Initialize mutex
    pthread_mutex_init(&buf->mutex, NULL);


}

/**
 * @brief Destroy/cleanup a bounded buffer
 */
void destroy_buffer(BoundedBuffer *buf) {
    sem_destroy(&buf->empty);
    sem_destroy(&buf->full);
    pthread_mutex_destroy(&buf->mutex);

}

/**
 * @brief Add a number to bounded buffer (PRODUCER operation)
 */
void buffer_add(BoundedBuffer *buf, NumberData data) {
 // Wait for empty slot
    sem_wait(&buf->empty);

    // Lock mutex
    pthread_mutex_lock(&buf->mutex);

    // Add data
    buf->buffer[buf->in] = data;
    buf->in = (buf->in + 1) % BUFFER_SIZE;

    // Unlock mutex
    pthread_mutex_unlock(&buf->mutex);

    // Signal full slot
    sem_post(&buf->full);
}

/**
 * @brief Remove a number from bounded buffer (CONSUMER operation)
 */
NumberData buffer_remove(BoundedBuffer *buf) {
NumberData data;

    // Wait for full slot
    sem_wait(&buf->full);

    // Lock mutex
    pthread_mutex_lock(&buf->mutex);

    // Remove data
    data = buf->buffer[buf->out];
    buf->out = (buf->out + 1) % BUFFER_SIZE;

    // Unlock mutex
    pthread_mutex_unlock(&buf->mutex);

    // Signal empty slot
    sem_post(&buf->empty);

    return data;
}


/* ========================================================================================*/
/* ⚠️ THREAD FUNCTIONS - ALREADY PROVODED (YOU NEED NOT IMPLEMENT THESE)
   ⚠️ DO NOT MODIFIY THESE FUNCTIONS 
 ========================================================================================*/

/**
 * @brief Generator thread function (PRODUCER for buffer1)
 */
void* generator_thread(void *arg) {
    // Compute start and end range for this thread
    int thread_id = *(int*)arg;
    int start = (thread_id * N) / NUM_GENERATORS;
    int end   = ((thread_id + 1) * N) / NUM_GENERATORS;

    // Loop through assigned range and add to buffer1
    for (int i = start; i < end; i++) {
        NumberData data = input_array[i];  // ✅ Load from pre-parsed input
        buffer_add(&buffer1, data);
    }

    return NULL;
}

/**
 * @brief Processor thread function (CONSUMER + PRODUCER)
 */
void* processor_thread(void *arg) {
    // Compute start and end indices for this thread
    int thread_id = *(int*)arg;
    int start = (thread_id * N) / NUM_PROCESSORS;
    int end   = ((thread_id + 1) * N) / NUM_PROCESSORS;

    // Consume from buffer1, square numbers, produce to buffer2
    for (int i = start; i < end; i++) {
        NumberData data = buffer_remove(&buffer1);
        data.number = data.number * data.number;
        buffer_add(&buffer2, data);
    }

    // Notify writer that this processor is done
    sem_post(&processor_done);
    
    return NULL;
}

/**
 * @brief Writer thread function (CONSUMER for buffer2)
 *
 * Note:
 * The argument `arg` is unused because the writer thread
 * does not require any parameters.
 * However, the function must match the standard pthread signature:
 *     void* (*)(void*)
 * So, we explicitly cast `(void)arg;` below to mark it as "intentionally unused".
 * This prevents compiler warnings when using flags like `-Wall -Werror`.
 */
void* writer_thread_func(void *arg) {
    (void)arg;  // ✅ Prevents unused-parameter warning; required for clean -Werror builds
    
    // Initialize variables for maintaining order
    int expected_id = 0;
    NumberData holding[MAX_N];
    bool has_data[MAX_N] = {false};
    int received = 0;

    // Process all N numbers
    while (received < N) {
        NumberData data = buffer_remove(&buffer2);

        // If this is the next expected ID, write it immediately
        if (data.id == expected_id) {
            write_result(data);
            expected_id++;

            // Check if future results are already waiting in holding buffer
            while (expected_id < N && has_data[expected_id]) {
                write_result(holding[expected_id]);
                has_data[expected_id] = false;
                expected_id++;
            }
        } else {
            // Out of order - store in holding buffer
            holding[data.id] = data;
            has_data[data.id] = true;
        }

        received++;
    }

    // Wait for all processor threads to finish
    for (int i = 0; i < NUM_PROCESSORS; i++) {
        sem_wait(&processor_done);
    }

    return NULL;
}

/*
 * Common Mistakes to Avoid:
 * -------------------------
 * ❌ Calling pthread_mutex_lock BEFORE sem_wait → DEADLOCK
 * ❌ Forgetting pthread_mutex_unlock → DEADLOCK
 * ❌ Forgetting sem_post → Threads never wake up
 * ❌ Skipping circular indexing → Buffer overflow
 * ❌ Forgetting (void)arg in writer_thread_func → Warnings under -Werror
 *
 * 💡 Remember:
 * - sem_wait BEFORE locking the mutex
 * - Always unlock mutex before leaving critical section
 * - sem_post AFTER finishing producer/consumer action
 * - Use (void)arg for unused parameters (standard POSIX pattern)
 */
