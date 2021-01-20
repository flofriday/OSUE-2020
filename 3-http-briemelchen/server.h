/**
 * @author briemelchen
 * @date 03.01.2020
 * @brief programs of that module represent a HTTP 1.1 server. header for @see server.c
 * @details defines macros and include-dependencies
 **/ 

#ifndef server_h
#define server_h

#include "util.h"
#include "gziputil.h" 
#include <signal.h>

#define DEFAULT_FILE "index.html" // default index file, if no other is specified
#define DEFAULT_PORT "8080" // default port, if no other is specified

#endif