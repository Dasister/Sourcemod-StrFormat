#if defined _str_format_included
  #endinput
#endif
#define _str_format_included

native StrFormatEx(String:buffer[], maxlength, const String:format[], any:...);

public Extension:__ext_str_format = 
{
	name = "String Format Tool",
	file = "str_format.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};