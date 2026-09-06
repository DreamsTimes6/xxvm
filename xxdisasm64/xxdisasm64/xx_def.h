#pragma once

#include <stdio.h>



#pragma pack(1)


#define _export   \
	 __declspec(dllexport)

#define DLL_PUBLIC   \
	 __declspec(dllexport)

#define uchar \
	unsigned char

#define ulong \
	unsigned long