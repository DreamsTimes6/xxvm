#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xx_comm32.h"
#include "xx_exp_data.h"


#define EXP_VERSION_LEN  0x10

struct XX_EXP  *exp_tbl;
int exp_num;



/*
检查更新exp_data

版本号+exp_num+exp_tbl
版本号   大小0x10
exp_num  大小int

*/



/*
初始化exp_tbl
*/
int xx_exp_data_init(char *exp_file)
{
	int iret = 0;
	int exp_size = 0;

	if (exp_tbl != 0)
	{
		free(exp_tbl);
	}
	exp_tbl = 0;
	exp_num = 0;
	
	/*读取exp_num*/
	iret=xx_read_file(exp_file, (char*)&exp_num, EXP_VERSION_LEN, sizeof(exp_num));
	if (iret == 0)
	{
		return 0;
	}
	if (exp_num == 0)
	{
		return 0;
	}
	
	/*计算exp_tbl大小*/
	exp_size = exp_num * sizeof(struct XX_EXP);
	if (exp_size < sizeof(struct XX_EXP))
	{
		return 0;
	}

	/*申请exp_tbl空间*/
	exp_tbl = malloc(exp_size);
	if (exp_tbl == 0)
	{
		return 0;
	}
	memset(exp_tbl, 0, exp_size);

	/*读取exp_data*/
	iret = xx_read_file(exp_file, (char*)exp_tbl, EXP_VERSION_LEN+ sizeof(exp_num),exp_size );
	if (iret == 0)
	{
		return 0;
	}

	return 1;
}


/*
获取exp_tbl
*/
struct XX_EXP  *xx_exp_data_get()
{
	if (exp_tbl != 0)
	{
		return exp_tbl;
	}
	return 0;
}

/*
获取exp_tbl
*/
int xx_exp_data_num()
{
	return exp_num;
}


/*
释放exp_tbl
*/
void xx_exp_data_free()
{
	if (exp_tbl != 0)
	{
		free(exp_tbl);
		exp_tbl = 0;
		exp_num = 0;
	}
	return;
}



























