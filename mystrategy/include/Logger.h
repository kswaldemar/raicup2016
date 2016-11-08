//
// Created by valdemar on 08.11.16.
//

#pragma once

#ifdef RUNNING_LOCAL

#include <cstdio>

#define LOG_DEBUG(...) printf("[ %s:%d ] DEBUG ", __FUNCTION__, __LINE__);  printf( __VA_ARGS__ );

#else

#define LOG_DEBUG(...)

#endif
