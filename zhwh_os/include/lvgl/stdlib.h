/**
 * @file stdlib.h
 * @brief Minimal stdlib.h for LVGL compilation
 */

#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

// Memory allocation functions (should be provided by the system)
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

// Utility functions
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void exit(int status) __attribute__((noreturn));
void abort(void) __attribute__((noreturn));

// Absolute value
int abs(int x);
long labs(long x);

// Numeric conversion
int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);

// Div structure
typedef struct {
    int quot;
    int rem;
} div_t;

typedef struct {
    long quot;
    long rem;
} ldiv_t;

typedef struct {
    long long quot;
    long long rem;
} lldiv_t;

div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);

#endif /* _STDLIB_H */
