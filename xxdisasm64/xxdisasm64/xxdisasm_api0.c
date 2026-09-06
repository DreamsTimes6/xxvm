#include <stdlib.h>
#include "xx_err.h"
#include "xx_inst.h"

extern ulong err_code;


extern int xx_get_reg_mic2(unsigned long ireg_def,char* reg_mic,unsigned long mic_size);



char *xx_err_des[] =
{
	"unknow",
	"success",
	"undefine",
	"unknow",
	"seq error",
	"not init",
	"arg error",
	"arg of item error ",
	"malloc error",
	"buf error",
};




  DLL_PUBLIC int xx_get_last_err()
{
	return err_code;
}


  DLL_PUBLIC char *xx_get_last_err_des()
{
	return xx_err_des[err_code];
}



DLL_PUBLIC int xx_reg_mic(ulong reg_def,char *reg_mic,int mic_size)
{
	int iret=0;

	xx_get_reg_mic2(reg_def,reg_mic,mic_size);
	if(iret==0)
	{
		err_code=INST_ERR_BUFSIZE;
		return 0;
	}

	err_code=INST_ERR_SUCCESS;
	return 1;
}



DLL_PUBLIC int xx_cmp_reg(ulong reg_def1,ulong reg_def2)
{
	if(reg_def1==reg_def2)
	{
		return REG_SAME;
	}

	if((reg_def1&0xffff0000)==(reg_def2&0xffff0000) && \
			(reg_def1&0x0000ff00)<(reg_def2&0x0000ff00) && \
			(reg_def1&0x000000ff)<(reg_def2&0x000000ff))
	{
		return REG_1_SUB;
	}
	
	if((reg_def1&0xffff0000)==(reg_def2&0xffff0000) && \
			(reg_def1&0x0000ff00)>(reg_def2&0x0000ff00) && \
			(reg_def1&0x000000ff)>(reg_def2&0x000000ff))
	{
		return REG_2_SUB;
	}
	
	err_code=INST_ERR_SUCCESS;
	return REG_NSAME;
}






