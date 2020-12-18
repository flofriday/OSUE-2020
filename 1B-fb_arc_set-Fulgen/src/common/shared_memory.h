/**
 * @file shared_memory.h
 * @author George Tokmaji <e11908523@student.tuwien.ac.at>
 * @date 22.11.2020
 *
 * @brief Shared memory datastrucure
 *
 * This module provides a datastructure representing circular buffer in shared memory as well as the necessary semaphores to ensure synchronicity and
 * data integrity.
 * Generators can write to the circular buffer occurs by calling shared_memory_write_feedback_arc_set. The supervisor can read via
 * shared_memory_read_feedback_arc_set. Generators cannot read from and supervisors cannot write to the buffer.
 * The supervisor can request generators to quit using shared_memory_request_quit; generators can check for that using shared_memory_quit_requested.
 **/

#pragma once

#include "feedback_arc_set.h"

#include <stdbool.h>
#include <stdlib.h>

/**
 * @brief The shared_memory struct
 *
 * @details This is an opaque struct which only be used as an argument to
 * shared_memory_create, shared_memory_destroy, shared_memory_request_quit, shared_memory_quit_requested, shared_memory_write_feedback_arc_set and shared_memory_read_feedback_arc_set.
 **/
struct shared_memory;

/**
 * @brief Creates a new shared memory structure
 * @param prefix The prefix used to prefix the shared memory name and the semaphore names. A leading slash is automatically added by the implementation.
 * @param name The name for the shared memory
 * @param is_supervisor Whether the caller is a supervisor or a generator
 * @param feedback_arc_capacity How many instances of struct feedback_arc_set the circular buffer should have the capacity for
 * @return A valid pointer to a named semaphore struct if successful
 * @return NULL on error. Check errno for details.
 **/
struct shared_memory *shared_memory_create(const char *const prefix, const char *const name, bool is_supervisor, size_t feedback_arc_capacity);

/**
 * @brief Destroys a shared memory created with shared_memory_create
 * @param memory A valid pointer to a shared memory struct
 **/
void shared_memory_destroy(struct shared_memory *memory);

/**
 * @brief Requests generators to quit.
 * @param memory A valid pointer to a shared memory struct
 * @return 0 on success
 * @return -1 if not a supervisor. errno is set to EPERM.
 * @return -1 on error. Check errno for details.
 **/
int shared_memory_request_quit(struct shared_memory *memory);

/**
 * @brief Returns whether quitting has been requested by the supervisor
 * @param memory A valid pointer to a shared memory struct
 * @return Whether the application should quit
 **/
bool shared_memory_quit_requested(struct shared_memory *const memory);

/**
 * @brief Writes a feedback arc set to the circular buffer.
 * @details This function blocks until it has acquired the write semaphore and until enough space is present in the circular buffer.
 * @param memory A valid pointer to a shared memory struct
 * @param arc_set A pointer to the feedback arc set to write. Note that no validation is occuring, the caller is responsible for the set's integrity.
 * @return 0 on success
 * @return -1 if not a supervisor. errno is set to EPERM.
 * @return -1 on error. Check errno for details.
 **/
int shared_memory_write_feedback_arc_set(struct shared_memory *memory, struct feedback_arc_set *const arc_set);

/**
 * @brief Reads a feedback arc set from the circular buffer.
 * @details This function blocks until a feedback arc set is present in the buffer.
 * @param memory A valid pointer to a shared memory struct
 * @param arc_set A pointer to the feedback arc set to copy the read set into. Note that no validation is occuring, the caller is responsible for checking the set's integrity.
 * @return 0 on success
 * @return -1 if a supervisor. errno is set to EPERM.
 * @return -1 on error. Check errno for details.
 */
int shared_memory_read_feedback_arc_set(struct shared_memory *memory, struct feedback_arc_set *arc_set);
