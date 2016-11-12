//
// Created by valdemar on 08.11.16.
//

#pragma once

#ifdef RUNNING_LOCAL

#include <cstdio>

#define LOG(...) printf(__VA_ARGS__)

#else

#define LOG(...)

#endif
