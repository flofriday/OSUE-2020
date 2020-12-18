/**
 * @file feedback_arc_set.h
 * @author George Tokmaji <e11908523@student.tuwien.ac.at>
 * @date 22.11.2020
 *
 * @brief Feedback arc set datastructure
 *
 * This module defines a datastructure to store a feedback arc set.
 **/


#pragma once

#include "config.h"

#include <stdint.h>

/**
 * @brief A directional edge represented by two points
 **/
struct edge
{
	/**
	 * @brief The start vertex
	 **/
	uint32_t u;

	/**
	 * @brief The end vertex
	 **/
	uint32_t v;
};

/**
 * @brief A feedback arc set represented by its edges
 **/
struct feedback_arc_set
{
	/**
	 * @brief The number of edges
	 **/
	uint8_t size;

	/**
	 * @brief The edges
	 * @invariant No more than size edges
	 * @invariant No more than MAX_NUM_EDGES edges
	 **/
	struct edge edges[MAX_NUM_EDGES];
};
