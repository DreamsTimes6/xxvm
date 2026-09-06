#pragma once

#include <stdio.h>







#ifndef uchar
#define uchar \
	unsigned char
#endif


#ifndef ulong
#define ulong \
	unsigned long
#endif

#ifndef ulong64
#define ulong64 \
	unsigned long long
#endif


/*需要重新定义，定义为地址，兼容64，32*/
#ifdef _WIN64

#ifndef XLADDR
#define XLADDR \
	unsigned long long
#endif

#else

#ifndef XLADDR
#define XLADDR \
	unsigned long 
#endif

#endif // _WIN64





#ifndef _export
#define _export   \
	 __declspec(dllexport)
#endif

#ifndef DLL_PUBLIC
#define DLL_PUBLIC   \
	 __declspec(dllexport)
#endif



#define LOG_SIZE 1000




#define OPMZ_LV1   10
#define OPMZ_LV2   20
#define OPMZ_LV3   30
#define OPMZ_LV4   40
#define OPMZ_LV_FIX 10
#define OPMZ_LV_TMP 1


//#define DBG_LOG
