#include <Windows.h>
#include "xx_comm32.h"
#include "xx_opmz.h"


int opmz_data_flag=0;


///////////////////////////////////////////////////////////////////////////////////////////

extern int xx_cfg_get_int(int ikey, int *out_var);
/////////////////////////////////////////////////////////////////////////////////////////////
int xx_opmz_data(char *pfile, int src_seq, int idef, char *var);
/*
生成伪指令数据文件
参数1：数据文件名
参数2：指令地址文本
参数3：指令定义
参数4：指令常数，寄存器定义
参数5：指令数据
返回值：成功-1；失败-0
*/

int xx_opmz_data(char *pfile, int src_seq, int idef, char *var)
{
	int iret = 0;
	struct ST_VM_INST vm_inst;

	/*参数检查*/
	if (pfile == 0  || var == 0 )
	{
		return 0;
	}

	/*获取优化数据标志*/
	if(opmz_data_flag==0)
	{
		/*从配置文件读取参数*/
		iret=xx_cfg_get_int(3,&opmz_data_flag);
		if(iret==0)
		{
			return 0;
		}
	}

	/*标志为1，则产生优化数据*/
	if(opmz_data_flag!=1)
	{
		return 1;
	}


	/*结构赋值*/
	memset(&vm_inst, 0, sizeof(vm_inst));
	vm_inst.idef = idef;
	vm_inst.src_seq = src_seq;
	memcpy(vm_inst.var, var, sizeof(vm_inst.var));
	

	//memset(&vm_inst_data, 0, sizeof(vm_inst_data));
	//memcpy(vm_inst_data.data, pdata, strlen(pdata));

	/*写入文件*/
	iret=xx_append_file(pfile, (char*)&vm_inst, sizeof(vm_inst));
	if (iret == 0)
	{
		return 0;
	}
	//iret = xx_append_file(pdfile, (char*)&vm_inst_data, sizeof(vm_inst_data));
	//if (iret == 0)
	//{
	//	return 0;
	//}

	return 1;
}


/*
cmd命令时设置标志
*/
int xx_opmz_set_flag()
{
	int iret=0;

	opmz_data_flag=0;
	/*从配置文件读取参数*/
	iret=xx_cfg_get_int(3,&opmz_data_flag);
	if(iret==0)
	{
		return 0;
	}

	return 1;
}




/*
写入指令数据项
*/
int xx_opmz_item_write(char *pdfile,struct ST_VM_ITEM_DATA *pitem_data)
{
	int iret = 0;


	/*参数检查*/
	if (pdfile == 0 )
	{
		return 0;
	}

	/*获取优化数据标志*/
	if (opmz_data_flag == 0)
	{
		/*从配置文件读取参数*/
		iret = xx_cfg_get_int(3, &opmz_data_flag);
		if (iret == 0)
		{
			return 0;
		}
	}

	/*标志为1，则产生优化数据*/
	if (opmz_data_flag != 1)
	{
		return 1;
	}

	/*写入文件*/
	iret = xx_append_file(pdfile, (char*)pitem_data, sizeof(struct ST_VM_ITEM_DATA));
	if (iret == 0)
	{
		return 0;
	}

	return 1;
}




void xx_opmz_write_invalid(char *pfile, char *pdfile, int seq)
{
	int iret = 0;
	char out_var[0x10];
	struct ST_VM_ITEM_DATA item_data;

	/*获取优化数据标志*/
	if (opmz_data_flag == 0)
	{
		/*从配置文件读取参数*/
		iret = xx_cfg_get_int(3, &opmz_data_flag);
		if (iret == 0)
		{
			return ;
		}
	}

	/*标志为1，则产生优化数据*/
	if (opmz_data_flag != 1)
	{
		return ;
	}

	memset(&item_data, 0, sizeof(item_data));
	memset(out_var, 0, sizeof(out_var));

	xx_opmz_data(pfile, seq, 1, out_var);

	xx_opmz_item_write(pdfile, &item_data);

	return;
}







