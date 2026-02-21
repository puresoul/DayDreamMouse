#pragma once
#include <string>
#include <windows.h>
#include <math.h>
#include <cstdarg>
#include <iostream>

#define EXPAND( x ) x

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WIDEFILE__ WIDEN(__FILE__)
#define __WIDEFUNCTION__ WIDEN(__FUNCTION__)

// For console, use printf; otherwise use OutputDebugString
#if defined(_CONSOLE) || defined(__CONSOLE__)
#define _L 
#define _trace(...)  printf(__VA_ARGS__)
#else
#define _L L
#define _trace(...)  mytrace(__VA_ARGS__)  
#endif

#define GET_LAST_ARG(_gla1, _gla2, _gla3, _gla4, _gla5, _gla6, _gla7, _gla8, _gla9, _gla10, _gla11, _glaN,...) _glaN
#define PRINT_MACRO_CHOOSER(_pmc1, _pmc2, _pmc3, _pmc4,_pmc5, _pmc6, _pmc7,_pmc8,_pmc9,_pmc10,_pmc11,...)   EXPAND(GET_LAST_ARG(__VA_ARGS__, \
_pmc11, _pmc10, _pmc9, _pmc8,_pmc7, _pmc6, _pmc5,_pmc4,_pmc3,_pmc2,_pmc1) (__VA_ARGS__)) 

#define IfPrintLINE1(x) if (x) _trace("%s : condition is not satisfied in line %d\r\n", #x, __LINE__);
#define IfPrintLINE2(x,...) if (x) {_trace(__VA_ARGS__); _trace(" ; %s : condition is not satisfied in line %d\r\n", #x, __LINE__);}
#define IfPrintLINE(...)   PRINT_MACRO_CHOOSER(IfPrintLINE1,IfPrintLINE2,IfPrintLINE2,IfPrintLINE2, \
 IfPrintLINE2,IfPrintLINE2,IfPrintLINE2,IfPrintLINE2,IfPrintLINE2,IfPrintLINE2,IfPrintLINE2,__VA_ARGS__) 

#define retLINE return __LINE__;
#define IfretLINE(x) if (x) return __LINE__;

#define DBOUT( s )  { \
   std::ostringstream os_; os_ << s; OutputDebugString( os_.str().c_str() ); \
}

// Console-friendly trace function
inline bool mytrace(const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	vprintf(format, argptr);
	va_end(argptr);
	return true;
}

// Overload for wide character strings (converts to narrow)
inline bool mytrace(const wchar_t *format, ...)
{
	// For now, just output to console (simplified)
	std::wcout << format << std::endl;
	return true;
}

