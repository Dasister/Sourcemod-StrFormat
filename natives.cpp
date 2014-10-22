#include "extension.h"
#include "natives.h"
#include "string_utils.h"

#include <memory.h>
#include <cstdlib>

class StaticCharBuf
{
	char *buffer;
	size_t max_size;
public:
	StaticCharBuf() : buffer(NULL), max_size(0)
	{
	}
	~StaticCharBuf()
	{
		free(buffer);
	}
	char* GetWithSize(size_t len)
	{
		if (len > max_size)
		{
			buffer = (char *)realloc(buffer, len);
			max_size = len;
		}
		return buffer;
	}
};

static char g_formatbuf[2048];
static StaticCharBuf g_extrabuf;

cell_t Native_StrFormatEx(IPluginContext *pContext, const cell_t *params)
{
	char *buf, *fmt;
	size_t res;
	int arg = 4;

	pContext->LocalToString(params[1], &buf);
	pContext->LocalToString(params[3], &fmt);
	res = strformat(buf, static_cast<size_t>(params[2]), fmt, pContext, params, &arg);

	return static_cast<cell_t>(res);
}

cell_t Native_StrFormat(IPluginContext *pContext, const cell_t *params)
{
	char *buf, *fmt, *destbuf;
	cell_t start_addr, end_addr, maxparam;
	size_t res, maxlen;
	int arg = 4;
	bool copy = false;
	char *__copy_buf;

	pContext->LocalToString(params[1], &destbuf);
	pContext->LocalToString(params[3], &fmt);

	maxlen = static_cast<size_t>(params[2]);
	start_addr = params[1];
	end_addr = params[1] + maxlen;
	maxparam = params[0];

	for (cell_t i = 3; i <= maxparam; i++)
	{
		if ((params[i] >= start_addr) && (params[i] <= end_addr))
		{
			copy = true;
			break;
		}
	}

	if (copy)
	{
		if (maxlen > sizeof(g_formatbuf))
		{
			__copy_buf = g_extrabuf.GetWithSize(maxlen);
		}
		else
		{
			__copy_buf = g_formatbuf;
		}
	}

	buf = (copy) ? __copy_buf : destbuf;
	res = strformat(buf, maxlen, fmt, pContext, params, &arg);

	if (copy)
	{
		memcpy(destbuf, __copy_buf, res + 1);
	}

	return static_cast<cell_t>(res);
}