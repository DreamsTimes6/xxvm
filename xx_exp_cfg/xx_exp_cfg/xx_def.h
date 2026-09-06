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

#ifndef _export
#define _export   \
	 __declspec(dllexport)
#endif

#ifndef DLL_PUBLIC
#define DLL_PUBLIC   \
	 __declspec(dllexport)
#endif

#ifndef DBG_LOG
#define DBG_LOG
#endif


#define LOG_SIZE 1000