/**
 * @file config.h
 * @author George Tokmaji <e11908523@student.tuwien.ac.at>
 * @date 22.11.2020
 *
 * @brief Configuration values
 *
 **/

#pragma once

#define MAX_FEEDBACK_SETS 500 /**< The maximum number of feedback sets that can be stored inside the circular buffer **/
#define MAX_NUM_EDGES 16 /**< The maximum number of edges in a feedback set **/

#define MATRICULAR_NUMBER "11908523_" /**< The matricular number used as a prefix for shared memory and semaphores named **/
#define SHM_NAME "fb_arc_set" /**< The name used for the shared memory in conjunction with MATRICULAR_NUMBER **/
