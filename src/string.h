#ifndef STRING_H
#define STRING_H

#include "types.h"

uint64_t strlen(const char *str);
int      strcmp(const char *a, const char *b);
void    *memcpy(void *dest, const void *src, uint64_t n);
void    *memset(void *s, int c, uint64_t n);
void    *memmove(void *dest, const void *src, uint64_t n);
int      memcmp(const void *a, const void *b, uint64_t n);
char    *itoa(int64_t value, char *str, int base);
void     reverse(char *str, int len);
int      isdigit(int c);

#endif