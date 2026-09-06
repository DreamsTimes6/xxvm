#include <Windows.h>
#include "xx_comm64.h"
#include "xx_exp_tbl.h"



/*
先写入静态结构，直接生成
*/

char exp_version[] = "1.3.2";
char *exp_file = "xx_exp.x";


int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	int iret = 0;
	struct EXP_DATA_HEAD  exp_head;

	/*获取数据表*/
	memset(&exp_head, 0, sizeof(exp_head));

	/*生成文件头*/
	memcpy(exp_head.version, exp_version, sizeof(exp_version));

	exp_head.num = EXP_NUM;

	/*判断一下文件头大小*/
	if (sizeof(exp_head) != 0x14)
	{
		return 0;
	}

	/*写入文件头*/
	iret = xx_cover_file(exp_file, (char*)&exp_head, sizeof(exp_head));
	if (iret == 0)
	{
		return 0;
	}

	/*写入数据*/
	iret = xx_append_file(exp_file, (char*)exp_tbl, sizeof(exp_tbl));
	if (iret == 0)
	{
		return 0;
	}

	return 1;
}


















