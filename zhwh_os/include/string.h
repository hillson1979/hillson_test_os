#ifndef STRING_H
#define STRING_H

#include "types.h"

unsigned strlen(const char *s);

void memcpy(void *dest, const void *src, uint32_t len);
void memmove(void *dest, const void *src, uint32_t len);

void *memset(void *dest_, int val, uint32_t len) ;
//void memset(void *dest, uint8_t val, uint32_t len);
void memsetw(void *dest, uint16_t val, uint32_t len);

int memcmp(const void *v1, const void *v2, uint32_t n);

int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, unsigned int len);

char *strcpy(char *dest, const char *src);

char *strcat(char *dest, const char *src);
char *strchr(const char *s, int c);


#endif
