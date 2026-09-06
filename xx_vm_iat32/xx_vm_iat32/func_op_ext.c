#include <stdio.h>
#include "xxdisasm32.h"





/*
符号扩展
参数1：变量1大小
参数2：变量2大小
参数3：变量2
*/
__declspec(noinline) void item_sign_ext(unsigned long size1, unsigned long size2, unsigned char *pvar2)
{
	int ext_size = 0;

	/*一样则返回*/
	if (size1 <= size2)
	{
		return;
	}

	ext_size = size1 - size2 - 1;

	/*扩展*/
	if (pvar2[size2 - 1] < 0x80)
	{
		while (ext_size >= 0)
		{
			pvar2[size2 + ext_size] = 0x00;
			ext_size = ext_size - 1;
		}
	}
	else
	{
		while (ext_size >= 0)
		{
			pvar2[size2 + ext_size] = 0xff;
			ext_size = ext_size - 1;
		}
	}
	return;
}



__declspec(noinline) int func_inst_prefix(struct XX_INST *pinst, unsigned char pc)
{
	int n = 0;

	for (n = 0; n < pinst->xx_inst_code.nprefix; n++)
	{
		if (pinst->xx_inst_code.prefix[n] == pc)
		{
			return 1;
		}
	}

	return 0;
}







