#ifndef _INCLUDE_SOURCEMOD_NATIVES_PROPER_H_
#define _INCLUDE_SOURCEMOD_NATIVES_PROPER_H_

cell_t Native_StrFormat(IPluginContext *pContext, const cell_t *params);
cell_t Native_StrFormatEx(IPluginContext *pContext, const cell_t *Params);

const sp_nativeinfo_t g_Natives[] =
{
	{"StrFormat",	 Native_StrFormat},
	{ "StrFormatEx", Native_StrFormatEx },
	{ NULL, NULL },
};

#endif // _INCLUDE_SOURCEMOD_NATIVES_PROPER_H_
