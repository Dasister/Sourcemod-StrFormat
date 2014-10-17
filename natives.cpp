#include "extension.h"
#include "natives.h"
#include "string_utils.h"

cell_t Native_StrFormat(IPluginContext *pContext, const cell_t *params)
{
	char *buf, *fmt;
	size_t res;
	int arg = 4;

	pContext->LocalToString(params[1], &buf);
	pContext->LocalToString(params[3], &fmt);
	res = strformat(buf, static_cast<size_t>(params[2]), fmt, pContext, params, &arg);

	return static_cast<cell_t>(res);
}