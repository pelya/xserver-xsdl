#include "Xos.h"
#include "os.h"
#include <stdarg.h>
#include <stdio.h>
    
char *
Xvprintf(const char *format, va_list va)
{
    char *ret;
    int size;
    va_list va2;

    va_copy(va2, va);
    size = vsnprintf(NULL, 0, format, va2);
    va_end(va2);

    ret = (char *)Xalloc(size + 1);
    if (ret == NULL)
        return NULL;

    vsnprintf(ret, size + 1, format, va);
    ret[size] = 0;
    return ret;
}

char *Xprintf(const char *format, ...)
{
    char *ret;
    va_list va;
    va_start(va, format);
    ret = Xvprintf(format, va);
    va_end(va);
    return ret;
}

char *
XNFvprintf(const char *format, va_list va)
{
    char *ret;
    int size;
    va_list va2;

    va_copy(va2, va);
    size = vsnprintf(NULL, 0, format, va2);
    va_end(va2);

    ret = (char *)XNFalloc(size + 1);
    if (ret == NULL)
        return NULL;

    vsnprintf(ret, size + 1, format, va);
    ret[size] = 0;
    return ret;
}

char *XNFprintf(const char *format, ...)
{
    char *ret;
    va_list va;
    va_start(va, format);
    ret = XNFvprintf(format, va);
    va_end(va);
    return ret;
}
