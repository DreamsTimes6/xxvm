#include <Windows.h>
#include "st_tree.h"
#include "xx_comm32.h"
#include "xx_opmz.h"
#include "xx_def_api.h"
#include "zlog.h"

/////////////////////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////////////////////

extern zlog_category_t *zc;
extern int func_cb_cmp_data(void *pdata1, void *pdata2);
/////////////////////////////////////////////////////////////////////////////////////////////
/*
单条指令生成树
*/
struct ST_TREE *xx_opmz_tree_gen(struct ST_VM_INST *pinst,int in_offset,int *pout_offset)
{
	int n = 0;
	int iret = 0;
	int idef = 0;
	struct ST_VM_INST sub_inst;
	int sub_idef = 0;
	int inst_seq = 0;
	unsigned char tmp_var[0x10];

	int tmp_offset = 0;

	/*树节点数据结构*/
	int var_num = 0;
	struct ST_OP_DATA var[4];
	struct ST_OP_DATA result;
	struct ST_OP_DATA *pvar[4];
	int pvar_size[4];

	/*树结构*/
	struct ST_TREE *ret_tree = 0;
	struct ST_TREE *sub_tree = 0;
	
	/*检查参数*/
	if (pinst == 0)
	{
		zlog_error(zc, "xx_opmz_tree_gen error. pinst:[%x]\n", pinst);
		return 0;
	}

	//zlog_info(zc, "=========xx_opmz_tree_gen start. seq:[%d] idef:[%x]=========\n", pinst->seq, pinst->idef);

	/*通过指令定义获取指令序号*/
	iret = def_api_get_iseq(pinst->idef, &inst_seq);
	if (iret == 0)
	{
		/*如果遇到此等级没有的指令，返回2，外部继续分析*/
		zlog_error(zc, "def_api_get_iseq error. pinst->idef:[%x]\n", pinst->idef);
		return 0;
	}

	/*赋值返回的out_offset*/
	if (pout_offset)
	{
		/*获取执行后的sp变化*/
		iret = def_api_get_sp_offset1(inst_seq, pout_offset);
		if (iret == 0)
		{
			zlog_error(zc, "def_api_get_sp_offset2 error. pinst->idef:[%x]\n", pinst->idef);
			return 0;
		}
	}

	/*指令中的值*/
	memset(tmp_var, 0, sizeof(tmp_var));
	memcpy(tmp_var, pinst->var, sizeof(pinst->var));

	memset(&result, 0, sizeof(result));
	//结果定义
	iret = def_api_rlst_get_idef(inst_seq, &result.data.idef);
	if (iret == 0)
	{
		//zlog_info(zc, "def_api_rlst_get_idef error. pinst->idef:[%x]\n", pinst->idef);
		return 0;
	}

	//如果是[sp]，则有sp的偏移
	tmp_offset = 0;
	iret = def_api_rlst_get_offset1(inst_seq, &tmp_offset);
	if (iret == 0)
	{
		result.data.offset = in_offset;
	}
	else
	{
		result.data.offset = in_offset + tmp_offset;
	}

	//zlog_info(zc, "def_api_rlst_get_offset2 . pinst->idef:[%x] tmp_offset:[%x]\n", pinst->idef, tmp_offset);

	//结果的数据大小
	iret = def_api_rlst_get_size(inst_seq, &result.data.op_size);
	if (iret == 0)
	{
		zlog_error(zc, "def_api_rlst_get_size error. pinst->idef:[%x]\n", pinst->idef);
		return 0;
	}

	/*对结果项文本赋值*/
	if (result.data.idef == 1)
	{
		//reg
		memcpy(result.data.var, tmp_var, sizeof(tmp_var));
		if (result.data.op_size == 1)
		{
			sprintf(result.data.sdef, "<breg[%lx]>", *(unsigned long*)tmp_var);
		}
		else if (result.data.op_size == 2)
		{
			sprintf(result.data.sdef, "<wreg[%lx]>", *(unsigned long*)tmp_var);
		}
		else if (result.data.op_size == 4)
		{
			sprintf(result.data.sdef, "<dreg[%lx]>", *(unsigned long*)tmp_var);
		}
		else if (result.data.op_size == 8)
		{
			sprintf(result.data.sdef, "<qreg[%lx]>", *(unsigned long*)tmp_var);
		}
	}
	else if (result.data.idef == 2)
	{
		//const
		if (result.data.op_size == 8)
		{
			memcpy(result.data.var, tmp_var, sizeof(tmp_var));
			sprintf(result.data.sdef, "<0x%llx>", *(unsigned long long*)tmp_var);
		}
		else
		{
			memcpy(result.data.var, tmp_var, sizeof(tmp_var));
			sprintf(result.data.sdef, "<0x%lx>", *(unsigned long*)tmp_var);
		}
	}
	else if (result.data.idef == 7)
	{
		//const
		if (result.data.op_size == 8)
		{
			memcpy(result.data.var, tmp_var, sizeof(tmp_var));
			sprintf(result.data.sdef, "[<0x%llx>]", *(unsigned long long*)tmp_var);
		}
		else
		{
			memcpy(result.data.var, tmp_var, sizeof(tmp_var));
			sprintf(result.data.sdef, "[<0x%lx>]", *(unsigned long*)tmp_var);
		}
	}
	else
	{
		iret = def_api_rlst_get_sdef(inst_seq, result.data.sdef, sizeof(result.data.sdef));
		if (iret == 0)
		{
			zlog_error(zc, "def_api_rlst_get_sdef error. pinst->idef:[%x]\n", pinst->idef);
			return 0;
		}
	}

	/*结果的操作项文本赋值*/
	iret = def_api_get_op_sdef(inst_seq, result.op.sdef, sizeof(result.op.sdef));
	if (iret == 0)
	{
		zlog_error(zc, "def_api_get_op_sdef error. pinst->idef:[%x]\n", pinst->idef);
		return 0;
	}
	/*结果的操作项定义赋值*/
	iret = def_api_get_op_idef(inst_seq, &result.op.idef);
	if (iret == 0)
	{
		zlog_error(zc, "def_api_get_op_idef error. pinst->idef:[%x]\n", pinst->idef);
		return 0;
	}

	//result.op.sub_flag = sub_flag;

	//zlog_info(zc, "result  op.idef:[%x] op.sdef:[%s] data.idef:[%x] data.sdef:[%s]\n", \
	//	result.op.idef, result.op.sdef, result.data.idef, result.data.sdef);



	//结果的运算变量数
	iret = def_api_get_item_num(inst_seq, &result.op.ivar);
	if (iret == 0)
	{
		zlog_error(zc, "def_api_get_item_num error. pinst->idef:[%x]\n", pinst->idef);
		return 0;
	}

	//原指令序号
	//iret = set_op_data_seq(&result, pinst->seq);
	//if (iret == 0)
	//{
	//	return 0;
	//}
	//result.op.src_idef = 0x0400;

	//原指令定义，标记该节点由哪个指令产生
	result.op.src_seq = pinst->seq;

	result.op.src_idef = pinst->idef;

	result.op.src_item_flag = 1;


	/*对操作项，变量赋值*/
	memset(var, 0, sizeof(var));
	var_num = 0;
	iret = def_api_get_item_num(inst_seq, &var_num);
	if (iret == 0)
	{
		zlog_error(zc, "def_api_get_item_num error. pinst->idef:[%x]\n", pinst->idef);
		return 0;
	}
	for (n = 0; n < var_num; n++)
	{

		//结果定义
		iret = def_api_item_get_idef(inst_seq, n, &var[n].data.idef);
		if (iret == 0)
		{
			zlog_error(zc, "def_api_item_get_idef error. var[%d].data.idef\n",n);
			return 0;
		}

		//如果是[sp]，则有sp的偏移
		tmp_offset = 0;
		iret = def_api_item_get_offset1(inst_seq, n, &tmp_offset);
		if (iret == 0)
		{
			var[n].data.offset = in_offset;
		}
		else
		{
			var[n].data.offset = in_offset + tmp_offset;
		}

		//结果的数据大小
		iret = def_api_item_get_size(inst_seq, n, &var[n].data.op_size);
		if (iret == 0)
		{
			zlog_error(zc, "def_api_item_get_size error. var[%d].data.op_size\n", n);
			return 0;
		}

		if (var[n].data.idef == 1)
		{
			//reg
			memcpy(var[n].data.var, tmp_var, sizeof(tmp_var));
			if (var[n].data.op_size == 1)
			{
				sprintf(var[n].data.sdef, "<reg[%lx]>", *(unsigned long*)tmp_var);
			}
			else if (var[n].data.op_size == 2)
			{
				sprintf(var[n].data.sdef, "<wreg[%lx]>", *(unsigned long*)tmp_var);
			}
			else if (var[n].data.op_size == 4)
			{
				sprintf(var[n].data.sdef, "<dreg[%lx]>", *(unsigned long*)tmp_var);
			}
			else if (var[n].data.op_size == 8)
			{
				sprintf(var[n].data.sdef, "<qreg[%lx]>", *(unsigned long*)tmp_var);
			}
		}
		else if (var[n].data.idef == 2)
		{
			//const
			if (var[n].data.op_size == 8)
			{
				memcpy(var[n].data.var, tmp_var, sizeof(tmp_var));
				sprintf(var[n].data.sdef, "<0x%llx>", *(unsigned long long*)tmp_var);
			}
			else
			{
				memcpy(var[n].data.var, tmp_var, sizeof(tmp_var));
				sprintf(var[n].data.sdef, "<0x%lx>", *(unsigned long*)tmp_var);
			}
		}
		else if (var[n].data.idef == 7)
		{
			//const
			if (var[n].data.op_size == 8)
			{
				memcpy(var[n].data.var, tmp_var, sizeof(tmp_var));
				sprintf(var[n].data.sdef, "[<0x%llx>]", *(unsigned long long*)tmp_var);
			}
			else
			{
				memcpy(var[n].data.var, tmp_var, sizeof(tmp_var));
				sprintf(var[n].data.sdef, "[<0x%lx>]", *(unsigned long*)tmp_var);
			}
		}
		else
		{
			iret = def_api_item_get_sdef(inst_seq, n, var[n].data.sdef, sizeof(var[n].data.sdef));
			if (iret == 0)
			{
				zlog_error(zc, "def_api_item_get_sdef error. var[%d].data.sdef\n", n);
				return 0;
			}
		}

		//var[n].op.sub_flag = sub_flag;

		//设置原指令序号
		//iret2 = set_op_data_seq(&var[n], pinst->seq);
		//if (iret2 == 0)
		//{
		//	return 0;
		//}

		//原指令定义，标记该节点由哪个指令产生
		var[n].op.src_seq = pinst->seq;

		var[n].op.src_idef = pinst->idef;

		var[n].op.src_item_flag = 2;
		var[n].op.src_item_seq = n;

		var[n].data.sub_seq = n + 1;

		//zlog_info(zc, "var[%d].data.sdef:[%s] data.idef:[%x] \n", n, var[n].data.sdef, var[n].data.idef);
	}


	/*生成分析树*/
	memset(pvar, 0, sizeof(pvar));
	memset(pvar_size, 0, sizeof(pvar_size));

	for (n = 0; n < var_num; n++)
	{
		pvar[n] = &var[n];
		pvar_size[n] = sizeof(var[n]);
	}

	ret_tree = xx_tree_new(&result, sizeof(result), var_num, pvar, pvar_size);
	if (ret_tree == 0)
	{
		zlog_error(zc, "xx_tree_new error\n", n);
		return 0;
	}


	/*判断子指令，有则调用指令生成*/
	for (n = 0; n < var_num; n++)
	{
		/*获取子指令*/
		iret = def_api_item_get_inst(inst_seq, n, &sub_idef);
		if (iret == 0)
		{
			continue;
		}

		/*生成子项树*/
		memset(&sub_inst, 0, sizeof(sub_inst));
		memcpy(&sub_inst, pinst, sizeof(sub_inst));
		sub_inst.idef = sub_idef;
		sub_tree = xx_opmz_tree_gen(&sub_inst, var[n].data.offset, 0);
		if (sub_tree == 0)
		{
			zlog_error(zc, "xx_opmz_tree_gen n:[%d] sub_idef:[%x]\n", n, sub_idef);
			continue;
		}

		/*合并子项树*/
		iret = xx_tree_merge(ret_tree, sub_tree, func_cb_cmp_data);
		if (iret == 0)
		{
			/*单个指令的子指令合并，只有1个合并项*/
			zlog_error(zc, "xx_tree_merge n:[%d] sub_idef:[%x]\n", n, sub_idef);
			continue;
		}
	}

	//zlog_info(zc, "=========xx_opmz_tree_gen finish. seq:[%d] idef:[%x]=========\n", pinst->seq, pinst->idef);



	return ret_tree;
}












/*
指令树合并
参数1：树
参数2：单条指令树
合并需要考虑，拆分项和扩大项
*/









