#include "string.h"
#include "types.h"

#if !defined(HOSTED)

// Copy len bytes from src to dest.
void memcpy(void *dest_, const void *src_, uint32_t len)
{
  uint8_t *dest = dest_;
  const uint8_t *src = src_;
  for(; len != 0; len--) *dest++ = *src++;
}

void memmove(void *dest_, const void *src_, uint32_t len) {
  uint8_t *dest = dest_;
  const uint8_t *src = src_;

  if (dest < src) {
    memcpy(dest, src, len);
  } else {
    dest += len;
    src += len;
    for(; len != 0; len--) *--dest = *--src;
  }
}

int
memcmp(const void *v1, const void *v2, uint32_t n)
{
  const uint8_t *s1, *s2;

  s1 = v1;
  s2 = v2;
  while(n-- > 0){
    if(*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }

  return 0;
}


void *memset(void *dest_, int val, uint32_t len) {
    uint8_t *dest = (uint8_t*)dest_;
    while (len--) {
        *dest++ = (uint8_t)val;
    }
    return dest_;
}

/*// Write len copies of val into dest.
void memset(void *dest_, uint8_t val, uint32_t len)
{
  uint8_t *dest = dest_;

  for ( ; len != 0; len--) *dest++ = val;
}*/

void memsetw(void *dest_, uint16_t val, uint32_t len)
{
  uint16_t *dest = dest_;

  for ( ; len != 0; len--) *dest++ = val;
}

// Compare two strings. Should return -1 if
// str1 < str2, 0 if they are equal or 1 otherwise.
int strcmp(const char *str1, const char *str2)
{
  while (*str1 && *str2 && (*str1 == *str2)) {
    ++str1;
    ++str2;
  }

  if (*str1 == '\0' && *str2 == '\0')
    return 0;

  if (*str1 == '\0')
    return -1;
  else return 1;
}

int strncmp(const char *str1, const char *str2, unsigned int len)
{
  unsigned int n = 0;
  while (*str1 && *str2 && (*str1 == *str2) && n < len-1) {
    ++str1;
    ++str2;
    ++n;
  }

  if (*str1 == *str2)
    return 0;

  if (*str1 == '\0')
    return -1;
  else return 1;
}

// Copy the NULL-terminated string src into dest, and
// return dest.
char *strcpy(char *dest, const char *src)
{
  char *_dest = dest;
  while (*src)
    *dest++ = *src++;
  *dest = '\0';
  return _dest;
}

// Copy at most n characters from src into dest, and
// return dest. If src is shorter than n, dest is padded with null bytes.
char *strncpy(char *dest, const char *src, unsigned int n)
{
  char *_dest = dest;
  while (n > 0 && *src) {
    *dest++ = *src++;
    n--;
  }
  // Pad with null bytes if needed
  while (n > 0) {
    *dest++ = '\0';
    n--;
  }
  return _dest;
}

// Concatenate the NULL-terminated string src onto
// the end of dest, and return dest.
char *strcat(char *dest, const char *src)
{
  char *_dest = dest;
  while (*dest)
    ++dest;

  while (*src)
    *dest++ = *src++;
  *dest = '\0';
  return _dest;
}

// Tokenize a string (simple, non-reentrant implementation)
char *strtok(char *str, const char *delim)
{
  static char *last = NULL;
  char *token;

  if (str != NULL)
    last = str;

  if (last == NULL || *last == '\0')
    return NULL;

  // Skip leading delimiters
  while (*last && strchr(delim, *last))
    last++;

  if (*last == '\0')
    return NULL;

  token = last;

  // Find end of token
  while (*last && !strchr(delim, *last))
    last++;

  if (*last)
    *last++ = '\0';

  return token;
}

unsigned strlen(const char *src)
{
  int i = 0;
  while (*src++)
    i++;
  return i;
}

char *strchr(const char *s, int c) {
  while (*s) {
    if (*s == (char)c)
      return (char*)s;
    ++s;
  }
  return NULL;
}

// 安全的字符串复制函数，避免缓冲区溢出
char *safestrcpy(char *dest, const char *src, int n) {
  char *p = dest;
  while (*src && n > 1) {
    *p++ = *src++;
    n--;
  }
  *p = '\0';
  return dest;
}

#endif /* !defined(HOSTED) */
