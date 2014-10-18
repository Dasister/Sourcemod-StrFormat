#include "extension.h"
#include "string_utils.h"
#include "sp_typeutil.h"

#include <cmath>

#define LADJUST         0x00000004      /* left adjustment */
#define ZEROPAD         0x00000080      /* zero (as opposed to blank) pad */
#define UPPERDIGITS     0x00000200      /* make alpha digits uppercase */
#define to_digit(c)     ((c) - '0')
#define is_digit(c)     ((unsigned)to_digit(c) <= 9)

#define CHECK_ARGS(x) \
    if ((arg+x) > args) { \
        pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "String formatted incorrectly - parameter %d (total %d)", arg, args); \
        return 0; \
    }

#define CALC_ARG(x)     (arg + x)

void AddInt(char **buf_p, size_t &maxlen, int val, int width, int flags);
void AddUInt(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags);
void AddBinary(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags);

size_t strformat(char *buffer, size_t maxlen, const char *format, IPluginContext *pContext, const cell_t *params, int *param)
{
    if (!buffer || !maxlen)
    {
        return 0;
    }

    int arg = 0;
    int args = params[0];
    char *buf_p;
    char ch;
    int flags;
    int width;
    int prec;
    int n;
    char sign;
    const char *fmt;
    size_t llen = maxlen - 1;
    int arg_to_use;

    buf_p = buffer;
    fmt = format;
    arg = *param;
    arg_to_use = 0;

    while (true)
    {
        for (ch = *fmt; llen && ((ch = *fmt) != '\0') && (ch != '%'); fmt++)
        {
            *buf_p++ = ch;
            llen--;
        }
        if ((ch == '\0') || (llen <= 0))
        {
            goto done;
        }

        fmt++;

        flags = 0;
        width = 0;
        prec = -1;
        sign = '\0';

rflag:
        ch = *fmt++;

reswitch:
        switch (ch)
        {
        case '-':
        {
            flags |= LADJUST;
            goto rflag;
        }
        case '$':
        {
            arg_to_use = width - 1;
            width = 0;
            ch = *fmt++;
            goto reswitch;
        }
        case '.':
        {
            n = 0;
            while (is_digit((ch = *fmt++)))
            {
                n = 10 * n + (ch - '0');
            }
            prec = (n < 0) ? -1 : n;
            goto reswitch;
        }
        case '0':
        {
            flags |= ZEROPAD;
            goto rflag;
        }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            n = 0;
            do
            {
                n = 10 * n + (ch - '0');
                ch = *fmt++;
            }
            while (is_digit(ch));
            width = n;
            goto reswitch;
        }
        case 'c':
        {
            CHECK_ARGS(arg_to_use);
            if (!llen)
            {
                goto done;
            }
            char *c;
            pContext->LocalToString(params[CALC_ARG(arg_to_use)], &c);
            *buf_p++ = *c;
            llen--;
            arg_to_use++;
            break;
        }
        case 'b':
        {
            CHECK_ARGS(arg_to_use);
            cell_t *value;
            pContext->LocalToPhysAddr(params[CALC_ARG(arg_to_use)], &value);
            AddBinary(&buf_p, llen, *value, width, flags);
            arg_to_use++;
            break;
        }

        case 'i':
        case 'd':
        {
            CHECK_ARGS(arg_to_use);
            cell_t *value;
            pContext->LocalToPhysAddr(params[CALC_ARG(arg_to_use)], &value);
            AddInt(&buf_p, llen, static_cast<int>(*value), width, flags);
            arg_to_use++;
            break;
        }
        case 'u':
        {
            CHECK_ARGS(arg_to_use);
            cell_t *value;
            pContext->LocalToPhysAddr(params[CALC_ARG(arg_to_use)], &value);
            AddUInt(&buf_p, llen, static_cast<unsigned int>(*value), width, flags);
            arg_to_use++;
            break;
        }
        case 'f':
        {
            CHECK_ARGS(arg_to_use);
            cell_t *value;
            pContext->LocalToPhysAddr(params[CALC_ARG(arg_to_use)], &value);
            // AddFloat(&buf_p, llen, sp_ctof(*value), width, prec, flags);
            arg_to_use++;
            break;
        }
        }
    }

done:
    *buf_p = '\0';
    *param = arg;
    return (maxlen - llen - 1);
}

void AddInt(char **buf_p, size_t &maxlen, int val, int width, int flags)
{
    char text[32];
    int digits;
    int signedVal;
    char *buf;
    unsigned int unsignedVal;

    digits = 0;
    signedVal = 0;
    if (val < 0)
    {
        unsignedVal = std::abs(val);
    }
    else
    {
        unsignedVal = val;
    }
    do
    {
        text[digits++] = '0' + unsignedVal % 10;
        unsignedVal /= 10;
    }
    while (unsignedVal);
    if (signedVal < 0)
    {
        text[digits++] = '-';
    }

    buf = *buf_p;

    if (!(flags & LADJUST))
    {
        while ((digits < width) && maxlen)
        {
            *buf++ = (flags & ZEROPAD ? '0' : ' ');
            width--;
            maxlen--;
        }
    }
    while (digits-- && maxlen)
    {
        *buf++ = text[digits];
    }
    if (flags & LADJUST)
    {
        while (width-- && maxlen)
        {
            *buf++ = (flags & ZEROPAD ? '0' : ' ');
            maxlen--;
        }
    }
    *buf_p = buf;
}

void AddUInt(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags)
{
    char text[32];
    int digits;
    char *buf;

    digits = 0;
    do
    {
        text[digits++] = '0' + val % 10;
        val /= 10;
    }
    while (val);
    buf = *buf_p;

    if (!(flags & LADJUST))
    {
        while (digits <= width && maxlen)
        {
            *buf++ = (flags & ZEROPAD ? '0' : ' ');
            width--;
            maxlen--;
        }
    }
    while (digits-- && maxlen)
    {
        *buf++ = (flags & ZEROPAD ? '0' : ' ');
        width--;
        maxlen--;
    }
    if (flags & LADJUST)
    {
        while (digits-- && maxlen)
        {
            *buf++ = (flags & ZEROPAD ? '0' : ' ');
            maxlen--;
        }
    }
    *buf_p = buf;
}

void AddBinary(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags)
{
    char text[32];
    int digits;
    char *buf;

    digits = 0;
    do
    {
        if (val & 1)
        {
            text[digits++] = '1';
        }
        else
        {
            text[digits++] = '0';
        }
        val >>= 1;
    }
    while (val);

    buf = *buf_p;

    if (!(flags & LADJUST))
    {
        while (digits < width && maxlen)
        {
            *buf++ = (flags & ZEROPAD) ? '0' : ' ';
            width--;
            maxlen--;
        }
    }

    while (digits-- && maxlen)
    {
        *buf++ = text[digits];
        width--;
        maxlen--;
    }

    if (flags & LADJUST)
    {
        while (width-- && maxlen)
        {
            *buf++ = (flags & ZEROPAD) ? '0' : ' ';
            maxlen--;
        }
    }

    *buf_p = buf;
}