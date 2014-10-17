#include "extension.h"
#include "string_utils.h"

#define LADJUST			0x00000004		/* left adjustment */
#define ZEROPAD			0x00000080		/* zero (as opposed to blank) pad */
#define UPPERDIGITS		0x00000200		/* make alpha digits uppercase */
#define to_digit(c)		((c) - '0')
#define is_digit(c)		((unsigned)to_digit(c) <= 9)

#define CHECK_ARGS(x) \
	if ((arg+x) > args) { \
		pContext->ThrowNativeErrorEx(SP_ERROR_PARAM, "String formatted incorrectly - parameter %d (total %d)", arg, args); \
		return 0; \
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
	int arg_to_use = -1;

	buf_p = buffer;
	fmt = format;
	arg = *param;

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
			arg_to_use = width;
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
			} while (is_digit(ch));
			width = n;
			goto reswitch;
		}
		case 'c':
		{
			CHECK_ARGS((arg_to_use - 1));
			if (!llen)
			{
				goto done;
			}
			char *c;
			pContext->LocalToString(params[arg + arg_to_use - 1], &c);
			*buf_p++ = *c;
			llen--;
			if (arg_to_use == -1)
				arg++;
			break;
		}
		case 'b':
		{
			CHECK_ARGS(0);
			cell_t *value;
			pContext->LocalToPhysAddr(params[arg], &value);
			// AddBinary(&buf_p, llen, *value, width, flags);
			arg++;
			break;
		}
		}


	}

done:
	*buf_p = '\0';
	*param = arg;
	return (maxlen - llen - 1);
}