#include "extension.h"
#include "string_utils.h"
#include "sp_typeutil.h"

#include <am-utility.h>

#include <cmath>

#define LADJUST         0x00000004      /* left adjustment */
#define ZEROPAD         0x00000080      /* zero (as opposed to blank) pad */
#define UPPERDIGITS     0x00000200      /* make alpha digits uppercase */
#define to_digit(c)     ((c) - '0')
#define is_digit(c)     ((unsigned)to_digit(c) <= 9)

#define CHECK_ARGS(x) \
    if ((arg+x) > args) { \
        pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "String formatted incorrectly - parameter %d (total %d)", arg + arg_to_use, args); \
        return 0; \
    }

#define CALC_ARG(x)     (arg + x)

void AddInt(char **buf_p, size_t &maxlen, int val, int width, int flags);
void AddUInt(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags);
void AddBinary(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags);
void AddFloat(char **buf_p, size_t &maxlen, double fval, int width, int prec, int flags);
void AddString(char **buf_p, size_t &maxlen, const char *string, int width, int prec);
void AddHex(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags);
size_t Translate(char *buffer, size_t maxlen, IPluginContext *pCtx, const char *key,
	cell_t target, const cell_t *params, int base_arg, int *arg, bool *error);

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	}
	else
	{
		return len;
	}
}


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
            AddFloat(&buf_p, llen, sp_ctof(*value), width, prec, flags);
            arg_to_use++;
            break;
        }
		case 'L':
		{
			CHECK_ARGS(arg_to_use);
			cell_t *value;
			pContext->LocalToPhysAddr(params[CALC_ARG(arg_to_use)], &value);
			char buffer[255];
			if (*value)
			{
				IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(*value);
				if (!pPlayer || !pPlayer->IsConnected())
				{
					return pContext->ThrowNativeError("Client index %d is invalid", *value);
				}
				const char* auth = pPlayer->GetAuthString();
				if (!auth || auth[0] == '\0')
				{
					auth = "STEAM_ID_PENDING";
				}
				int userid = pPlayer->GetUserId();
				UTIL_Format(buffer,
					sizeof(buffer),
					"%s<%d><%s><>",
					pPlayer->GetName(),
					userid,
					auth);
			}
			else
			{
				UTIL_Format(buffer,
					sizeof(buffer),
					"Console<0><Console><Console>");
			}
			AddString(&buf_p, llen, buffer, width, prec);
			arg_to_use++;
			break;
		}
		case 'N':
		{
			CHECK_ARGS(arg_to_use);
			cell_t *value;
			pContext->LocalToPhysAddr(params[CALC_ARG(arg_to_use)], &value);

			const char *name = "Console";
			if (*value)
			{
				IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(*value);
				if (!pPlayer || !pPlayer->IsConnected())
				{
					return pContext->ThrowNativeError("Client index %d is invalid", *value);
				}
				name = pPlayer->GetName();
			}
			AddString(&buf_p, llen, name, width, prec);
			arg_to_use++;
			break;
		}
		case 's':
		{
			CHECK_ARGS(arg_to_use);
			char *str;
			int err;
			if ((err = pContext->LocalToString(params[CALC_ARG(arg_to_use)], &str)) != SP_ERROR_NONE)
			{
				pContext->ThrowNativeErrorEx(err, "Could not deference string");
				return 0;
			}
			AddString(&buf_p, llen, str, width, prec);
			arg_to_use++;
			break;
		}
		case 'T':
		{
			CHECK_ARGS(arg_to_use + 1);
			char *key;
			bool error;
			size_t res;
			cell_t *target;
			pContext->LocalToString(params[CALC_ARG(arg_to_use++)], &key);
			pContext->LocalToPhysAddr(params[CALC_ARG(arg_to_use++)], &target);
			res = Translate(buf_p, llen, pContext, key, *target, params, arg, &arg_to_use, &error);
			if (error)
			{
				return 0;
			}
			buf_p += res;
			llen -= res;
		}
		case 't':
		{
			CHECK_ARGS(arg_to_use);
			char *key;
			bool error;
			size_t res;
			cell_t target = smutils->GetGlobalTarget();
			pContext->LocalToString(params[CALC_ARG(arg_to_use++)], &key);
			res = Translate(buf_p, llen, pContext, key, target, params, arg, &arg_to_use, &error);
			if (error)
			{
				return 0;
			}
			buf_p += res;
			llen -= res;
		}
		case 'X':
		{
			CHECK_ARGS(arg_to_use);
			cell_t *value;
			pContext->LocalToPhysAddr(params[CALC_ARG(arg_to_use)], &value);
			flags |= UPPERDIGITS;
			AddHex(&buf_p, llen, static_cast<unsigned int>(*value), width, flags);
			arg_to_use++;
			break;
		}
		case 'x':
		{
			CHECK_ARGS(arg_to_use);
			cell_t *value;
			pContext->LocalToPhysAddr(params[CALC_ARG(arg_to_use)], &value);
			AddHex(&buf_p, llen, static_cast<unsigned int>(*value), width, flags);
			arg_to_use++;
			break;
		}
		case '%':
		{
			if (!llen)
			{
				goto done;
			}
			*buf_p++ = ch;
			llen--;
			break;
		}
		case '\0':
		{
			if (!llen)
			{
				goto done;
			}
			*buf_p++ = '%';
			llen--;
			goto done;
		}
		default:
		{
			if (!llen)
			{
				goto done;
			}
			*buf_p++ = ch;
			llen--;
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

void AddFloat(char **buf_p, size_t &maxlen, double fval, int width, int prec, int flags)
{
	int digits;					// non-fraction part digits
	double tmp;					// temporary
	char *buf = *buf_p;			// output buffer pointer
	int val;					// temporary
	int sign = 0;				// 0: positive, 1: negative
	int fieldlength;			// for padding
	int significant_digits = 0;	// number of significant digits written
	const int MAX_SIGNIFICANT_DIGITS = 16;

	if (ke::IsNaN(fval))
	{
		AddString(buf_p, maxlen, "NaN", width, prec);
		return;
	}

	// default precision
	if (prec < 0)
	{
		prec = 6;
	}

	// get the sign
	if (fval < 0)
	{
		fval = -fval;
		sign = 1;
	}

	// compute whole-part digits count
	digits = (int)log10(fval) + 1;

	// Only print 0.something if 0 < fval < 1
	if (digits < 1)
	{
		digits = 1;
	}

	// compute the field length
	fieldlength = digits + prec + ((prec > 0) ? 1 : 0) + sign;

	// minus sign BEFORE left padding if padding with zeros
	if (sign && maxlen && (flags & ZEROPAD))
	{
		*buf++ = '-';
		maxlen--;
	}

	// right justify if required
	if ((flags & LADJUST) == 0)
	{
		while ((fieldlength < width) && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			maxlen--;
		}
	}

	// minus sign AFTER left padding if padding with spaces
	if (sign && maxlen && !(flags & ZEROPAD))
	{
		*buf++ = '-';
		maxlen--;
	}

	// write the whole part
	tmp = pow(10.0, digits - 1);
	while ((digits--) && maxlen)
	{
		if (++significant_digits > MAX_SIGNIFICANT_DIGITS)
		{
			*buf++ = '0';
		}
		else
		{
			val = (int)(fval / tmp);
			*buf++ = '0' + val;
			fval -= val * tmp;
			tmp *= 0.1;
		}
		maxlen--;
	}

	// write the fraction part
	if (maxlen && prec)
	{
		*buf++ = '.';
		maxlen--;
	}

	tmp = pow(10.0, prec);

	fval *= tmp;
	while (prec-- && maxlen)
	{
		if (++significant_digits > MAX_SIGNIFICANT_DIGITS)
		{
			*buf++ = '0';
		}
		else
		{
			tmp *= 0.1;
			val = (int)(fval / tmp);
			*buf++ = '0' + val;
			fval -= val * tmp;
		}
		maxlen--;
	}

	// left justify if required
	if (flags & LADJUST)
	{
		while ((fieldlength < width) && maxlen)
		{
			// right-padding only with spaces, ZEROPAD is ignored
			*buf++ = ' ';
			width--;
			maxlen--;
		}
	}

	// update parent's buffer pointer
	*buf_p = buf;
}

void AddString(char **buf_p, size_t &maxlen, const char *string, int width, int prec)
{
	int size = 0;
	char *buf;
	static char nlstr[] = { '(', 'n', 'u', 'l', 'l', ')', '\0' };

	buf = *buf_p;

	if (string == NULL)
	{
		string = nlstr;
		prec = -1;
	}

	if (prec >= 0)
	{
		for (size = 0; size < prec; size++)
		{
			if (string[size] == '\0')
			{
				break;
			}
		}
	}
	else
	{
		while (string[size++]);
		size--;
	}

	if (size > (int)maxlen)
	{
		size = maxlen;
	}

	maxlen -= size;
	width -= size;

	while (size--)
	{
		*buf++ = *string++;
	}

	while ((width-- > 0) && maxlen)
	{
		*buf++ = ' ';
		maxlen--;
	}

	*buf_p = buf;
}

void AddHex(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags)
{
	char text[32];
	int digits;
	char *buf;
	char digit;
	int hexadjust;

	if (flags & UPPERDIGITS)
	{
		hexadjust = 'A' - '9' - 1;
	}
	else
	{
		hexadjust = 'a' - '9' - 1;
	}

	digits = 0;
	do
	{
		digit = ('0' + val % 16);
		if (digit > '9')
		{
			digit += hexadjust;
		}

		text[digits++] = digit;
		val /= 16;
	} while (val);

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

inline void ReorderTranslationParams(const Translation *pTrans, cell_t *params)
{
	cell_t new_params[MAX_TRANSLATE_PARAMS];
	for (unsigned int i = 0; i < pTrans->fmt_count; i++)
	{
		new_params[i] = params[pTrans->fmt_order[i]];
	}
	memcpy(params, new_params, pTrans->fmt_count * sizeof(cell_t));
}

size_t Translate(char *buffer,
	size_t maxlen,
	IPluginContext *pCtx,
	const char *key,
	cell_t target,
	const cell_t *params,
	int base_arg,
	int *arg,
	bool *error)
{
#define T_CALC_ARG(x)	(base_arg + x)

	unsigned int langid;
	*error = false;
	Translation pTrans;
	IPlugin *pl = plsys->FindPluginByContext(pCtx->GetContext());
	unsigned int max_params = 0;
	IPhraseCollection *pPhrases;

	pPhrases = pl->GetPhrases();

try_serverlang:
	if (target == SOURCEMOD_SERVER_LANGUAGE)
	{
		langid = translator->GetServerLanguage();
	}
	else if ((target >= 1) && (target <= playerhelpers->GetMaxClients()))
	{
		langid = translator->GetClientLanguage(target);
	}
	else
	{
		pCtx->ThrowNativeErrorEx(SP_ERROR_PARAM, "Translation failed: invalid client index %d", target);
		goto error_out;
	}

	if (pPhrases->FindTranslation(key, langid, &pTrans) != Trans_Okay)
	{
		if (target != SOURCEMOD_SERVER_LANGUAGE && langid != translator->GetServerLanguage())
		{
			target = SOURCEMOD_SERVER_LANGUAGE;
			goto try_serverlang;
		}
		else if (langid != SOURCEMOD_LANGUAGE_ENGLISH)
		{
			if (pPhrases->FindTranslation(key, SOURCEMOD_LANGUAGE_ENGLISH, &pTrans) != Trans_Okay)
			{
				pCtx->ThrowNativeErrorEx(SP_ERROR_PARAM, "Language phrase \"%s\" not found", key);
				goto error_out;
			}
		}
		else
		{
			pCtx->ThrowNativeErrorEx(SP_ERROR_PARAM, "Language phrase \"%s\" not found", key);
			goto error_out;
		}
	}

	max_params = pTrans.fmt_count;

	if (max_params)
	{
		cell_t new_params[MAX_TRANSLATE_PARAMS];

		/* Check if we're going to over the limit */
		if ((T_CALC_ARG(*arg)) + (max_params - 1) > (size_t)params[0])
		{
			pCtx->ThrowNativeErrorEx(SP_ERROR_PARAMS_MAX,
				"Translation string formatted incorrectly - missing at least %d parameters",
				((T_CALC_ARG(*arg) + (max_params - 1)) - params[0])
				);
			goto error_out;
		}

		/* If we need to re-order the parameters, do so with a temporary array.
		* Otherwise, we could run into trouble with continual formats, a la ShowActivity().
		*/
		memcpy(new_params, params, sizeof(cell_t) * (params[0] + 1));
		ReorderTranslationParams(&pTrans, &new_params[T_CALC_ARG(*arg)]);

		return strformat(buffer, maxlen, pTrans.szPhrase, pCtx, new_params, arg);
	}
	else
	{
		return strformat(buffer, maxlen, pTrans.szPhrase, pCtx, params, arg);
	}

error_out:
	*error = true;
	return 0;
}