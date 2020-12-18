/**
 * @file named_semaphore.h
 * @author George Tokmaji <e11908523@student.tuwien.ac.at>
 * @date 22.11.2020
 *
 * @brief Named semaphore datastrucure.
 *
 * This module deals with named semaphores.
 **/

#pragma once

#include <semaphore.h>
#include <stdbool.h>

/**
 * @brief The named_semaphore struct
 *
 * @details This is an opaque struct which only be used as an argument to
 * named_semaphore_create, named_semaphore_destroy, named_semaphore_wait and named_semaphore_post.
 */

struct named_semaphore;

/**
 * @brief Creates a new named semaphore
 * @param name The name of the semaphore. A leading forward slash is required.
 * @param value The value the semaphore should be initialised with
 * @return A valid pointer to a named semaphore struct if successful
 * @return NULL on error. Check errno for details.
 */
struct named_semaphore *named_semaphore_create(const char *name, int value);

/**
 * @brief Destroys a named semaphore created with named_semaphore_create
 * @param semaphore A pointer to a named semaphore struct created with named_semaphore_create
 */
void named_semaphore_destroy(struct named_semaphore *semaphore);

/**
 * @brief Calls sem_wait on the named semaphore
 * @param semaphore A pointer to a named semaphore struct created with named_semaphore_create
 * @return The return value of sem_wait
 */
int named_semaphore_wait(struct named_semaphore *semaphore);

/**
 * @brief Calls sem_post on the named semaphore
 * @param semaphore A pointer to a named semaphore struct created with named_semaphore_create
 * @return The return value of sem_post
 */
int named_semaphore_post(struct named_semaphore *semaphore);
