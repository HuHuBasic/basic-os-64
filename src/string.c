#include "string.h"

uint64_t strlen(const char *str)
{
    uint64_t len = 0;
    while (str[len])
        len++;
    return len;
}

int strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(const unsigned char *)a - *(const unsigned char *)b;
}

void *memcpy(void *dest, const void *src, uint64_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (uint64_t i = 0; i < n; i++)
        d[i] = s[i];
    return dest;
}

void *memset(void *s, int c, uint64_t n)
{
    unsigned char *p = (unsigned char *)s;
    for (uint64_t i = 0; i < n; i++)
        p[i] = (unsigned char)c;
    return s;
}

void *memmove(void *dest, const void *src, uint64_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        for (uint64_t i = 0; i < n; i++)
            d[i] = s[i];
    } else {
        for (uint64_t i = n; i > 0; i--)
            d[i - 1] = s[i - 1];
    }
    return dest;
}

int memcmp(const void *a, const void *b, uint64_t n)
{
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    for (uint64_t i = 0; i < n; i++) {
        if (pa[i] != pb[i])
            return pa[i] - pb[i];
    }
    return 0;
}

void reverse(char *str, int len)
{
    int start = 0;
    int end = len - 1;
    while (start < end) {
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++;
        end--;
    }
}

char *itoa(int64_t value, char *str, int base)
{
    int i = 0;
    int is_negative = 0;

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    uint64_t uval;
    if (value < 0 && base == 10) {
        is_negative = 1;
        uval = (uint64_t)(-(value + 1)) + 1;
    } else {
        uval = (uint64_t)value;
    }

    while (uval != 0) {
        int rem = uval % base;
        str[i++] = (rem > 9) ? (rem - 10 + 'A') : (rem + '0');
        uval = uval / base;
    }

    if (is_negative)
        str[i++] = '-';

    str[i] = '\0';
    reverse(str, i);
    return str;
}

int isdigit(int c)
{
    return (c >= '0' && c <= '9');
}