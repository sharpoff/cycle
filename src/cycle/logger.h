#pragma once

#include <stdio.h> // IWYU pragma: keep

#ifndef NDEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif

#define LOGI(fmt, ...) if (DEBUG) printf("[%s][INFO ] " fmt "\n", __TIME__, __VA_ARGS__)
#define LOGW(fmt, ...) if (DEBUG) printf("\033[33m" "[%s][WARN ] " fmt "\033[0m\n", __TIME__, __VA_ARGS__)
#define LOGE(fmt, ...) if (DEBUG) printf("\033[31m" "[%s][ERROR] " fmt "\033[0m\n", __TIME__, __VA_ARGS__)