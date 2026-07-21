/**
 * compat/string.h — string.h 替代（映射到兼容层）
 */
#ifndef COMPAT_STRING_H
#define COMPAT_STRING_H

#include "compat.h"

#define memset(p,c,n)  jam_memset(p,c,n)
#define memcpy(d,s,n)  jam_memcpy(d,s,n)
#define memcmp(a,b,n)  jam_memcmp(a,b,n)
#define memmove(d,s,n) jam_memmove(d,s,n)
#define strlen(s)      jam_strlen(s)
#define strcpy(d,s)    jam_strcpy(d,s)
#define strncpy(d,s,n) jam_strncpy(d,s,n)
#define strcmp(a,b)    jam_strcmp(a,b)
#define strncmp(a,b,n) jam_strncmp(a,b,n)
#define strcat(d,s)    jam_strcat(d,s)
#define strchr(s,c)    jam_strchr(s,c)
#define strrchr(s,c)   jam_strrchr(s,c)
#define strdup(s)      jam_strdup(s)
#define strstr(h,n)    jam_strstr(h,n)
#define strtok_r(s,d,p) jam_strtok_r(s,d,p)

/* bzero 映射到 memset */
#define bzero(p,n)     jam_memset(p, 0, n)

#endif
