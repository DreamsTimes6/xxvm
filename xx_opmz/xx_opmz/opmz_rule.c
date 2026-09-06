#include <Windows.h>
#include "st_tree.h"
#include "xx_comm32.h"
#include "xx_opmz.h"
#include "xx_def_api.h"
#include "xx_exp_data.h"
#include "zlog.h"



////////////////////////////////////////////////////////////////////////////////////////////////////////////////


extern zlog_category_t *zc;
extern char ginst_dfile[0x500];
extern char gopmz_log[0x500];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int xx_opmz_tree_rule_idef1(struct ST_OP_DATA *pdata, int sub_num, struct ST_OP_DATA **psub_data, int*sub_index);
int xx_opmz_tree_rule_idef2(struct ST_OP_DATA *pdata, int sub_num, struct ST_OP_DATA **psub_data, int*sub_index);
int xx_opmz_tree_rule_idef3(struct ST_OP_DATA *pdata, int sub_num, struct ST_OP_DATA **psub_data, int*sub_index);
int xx_opmz_tree_rule_idef4(struct ST_OP_DATA *pdata, int sub_num, struct ST_OP_DATA **psub_data, int*sub_index);
int xx_opmz_tree_rule_idef5(struct ST_OP_DATA *pdata, int sub_num, struct ST_OP_DATA **psub_data, int*sub_index);

int xx_opmz_tree_rule_exp1(struct ST_OP_DATA *pdata, char *pexp, int sub_num, char **psub_exp, int*sub_index);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
优化规则都是两层间的优化
优化过后分析树变化，无法还原到原始状态
基于定义优化

*/
__declspec(noinline) void xx_opmz_tree_rule_idef(struct ST_TREE *ptree, int lv)
{

	//zlog_info(zc, "xx_opmz_tree_rule_idef lv:[%d]\n", lv);
	/*优化规则*/

	switch (lv)
	{
	case OPMZ_LV2:
		/*常量寄存器项转化为常量项*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef1);

		/*简单的变量传递，子项实值，父项虚值*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef2);

		/*合并常量运算*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef3);
		break;
	case OPMZ_LV3:
		/*常量寄存器项转化为常量项*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef1);

		/*简单的变量传递，子项实值，父项虚值*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef2);

		/*合并常量运算*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef3);

		/*特殊的常量运算*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef4);

		/*sp偏移合并化简*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef5);

		break;
	case OPMZ_LV4:
		/*常量寄存器项转化为常量项*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef1);

		/*简单的变量传递，子项实值，父项虚值*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef2);

		/*合并常量运算*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef3);

		/*特殊的常量运算*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef4);

		/*sp偏移合并化简*/
		xx_tree_opmz_idef(ptree, xx_opmz_tree_rule_idef5);

		break;
	}


	return;
}



/*
基于表达式的优化
基于表达式处理处理节点，
处理后重新生成表达式
处理过的节点操作不再是以前的节点操作，转为新的操作
优化过程只处理节点，不处理表达式
*/
__declspec(noinline) void xx_opmz_tree_rule_exp(struct ST_TREE *ptree, int lv)
{
	//zlog_info(zc, "xx_opmz_tree_rule_exp lv:[%d]\n", lv);
	/*优化规则*/

	switch (lv)
	{
	case OPMZ_LV2:
		//xx_tree_opmz_exp(ptree, xx_opmz_tree_rule_exp1);
		break;
	case OPMZ_LV3:
		xx_tree_opmz_exp(ptree, xx_opmz_tree_rule_exp1);
		break;
	case OPMZ_LV4:
		xx_tree_opmz_exp(ptree, xx_opmz_tree_rule_exp1);
		break;
	}

	return;
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/*
定义优化回调函数
常量寄存器转化为常量
*/
__declspec(noinline) int xx_opmz_tree_rule_idef1(struct ST_OP_DATA *pdata, int sub_num, struct ST_OP_DATA **psub_data, int*sub_index)
{
	int n = 0;
	int creg_index = 0;
	unsigned char *pcreg_var = 0;

	/*获取常量寄存器的索引和值*/
	creg_index = def_api_creg_index_get();

	pcreg_var = def_api_creg_var_get();

	/*常量寄存器一般为最低级子项，所以只判断子项*/
	for (n = 0; n < sub_num; n++)
	{
		if (psub_data[n]->data.idef == 1 && memcmp(psub_data[n]->data.var, (char*)&creg_index, sizeof(creg_index)) == 0)
		{

			/*数据定义为寄存器，并且索引相同，将该项转化为常量项*/
			psub_data[n]->data.idef = 2;
			memcpy(psub_data[n]->data.var, pcreg_var, sizeof(psub_data[n]->data.var));

			memset(psub_data[n]->data.sdef, 0, sizeof(psub_data[n]->data.sdef));
			if (psub_data[n]->data.op_size == 8)
			{
				sprintf(psub_data[n]->data.sdef, "<0x%llx>", *(unsigned long long*)psub_data[n]->data.var);
			}
			else
			{
				sprintf(psub_data[n]->data.sdef, "<0x%lx>", *(unsigned long*)psub_data[n]->data.var);
			}

			//zlog_info(zc, "===xx_opmz_tree_rule_idef1===\n");
			//zlog_info(zc, "data.idef:[%x] data.sdef:[%s] data.op_size:[%x] data.var:[%x]  data.offset:[%x]\n", \
			//	psub_data[n]->data.idef, psub_data[n]->data.sdef, psub_data[n]->data.op_size, *(int*)psub_data[n]->data.var, psub_data[n]->data.offset);

			//zlog_info(zc, "op.idef:[%x] op.src_seq:[%d]op.src_idef:[%x] op.sdef:[%s] op.ivar:[%x] \n", \
			//	psub_data[n]->op.idef, psub_data[n]->op.src_seq, psub_data[n]->op.src_idef, psub_data[n]->op.sdef, psub_data[n]->op.ivar);
		}
	}

	/*不处理子项，返回0*/
	return 0;
}





/*
定义优化回调函数
简单的变量传递，子项实值，父项虚值
*/
__declspec(noinline) int xx_opmz_tree_rule_idef2(struct ST_OP_DATA *pdata, int sub_num, struct ST_OP_DATA **psub_data, int*sub_index)
{
	int n = 0;
	int iret = 0;

	/*只有一个子项，如果是root节点，则不传递*/
	if (sub_num != 1 || pdata->op.root_flag == 1)
	{
		return 0;
	}

	/*父项为虚值，操作为mov*/
	if (pdata->data.idef == 4 && pdata->op.idef == 1)
	//if (pdata->data.idef == 4)
	{
		memcpy(&pdata->data, (char*)&psub_data[0]->data, sizeof(psub_data[0]->data));
		pdata->op.idef = 0;
		memset(pdata->op.sdef, 0, sizeof(pdata->op.sdef));

		*sub_index = 0;
		iret = 2;

		//zlog_info(zc, "===xx_opmz_tree_rule_idef2===\n");
		//zlog_info(zc, "data.idef:[%x] data.sdef:[%s] data.op_size:[%x] data.var:[%x]  data.offset:[%x]\n", \
		//	pdata->data.idef, pdata->data.sdef, pdata->data.op_size, *(int*)pdata->data.var, pdata->data.offset);

		//zlog_info(zc, "op.idef:[%x] op.src_seq:[%d]op.src_idef:[%x] op.sdef:[%s] op.ivar:[%x] \n", \
		//	pdata->op.idef, pdata->op.src_seq, pdata->op.src_idef, pdata->op.sdef, pdata->op.ivar);
	}


	return iret;
}








/*
定义优化回调函数
常量合并
*/
__declspec(noinline) int xx_opmz_tree_rule_idef3(struct ST_OP_DATA *pdata, int sub_num, struct ST_OP_DATA **psub_data, int*sub_index)
{
	int n = 0;
	int iret = 0;
	unsigned char var1[0x10];
	unsigned char var2[0x10];
	unsigned char var0[0x10];

	/*判断是否所有的子项都为常量项*/
	for (n = 0; n < sub_num; n++)
	{
		if (psub_data[n]->data.idef != 2)
		{
			/*有一个不是常量就不处理*/
			return 0;
		}
	}

	iret = 0;
	/*运算*/
	switch (pdata->op.idef)
	{
	case 2:
		/*无符号加，两个子项*/
		if (sub_num != 2)
		{
			return 0;
		}

		//var32_1 = *(unsigned long*)psub_data[0]->data.var;
		//var32_2 = *(unsigned long*)psub_data[1]->data.var;
		//var32 = var32_1 + var32_2;

		memset(var1, 0, sizeof(var1));
		memset(var2, 0, sizeof(var2));
		memset(var0, 0, sizeof(var0));

		memcpy(var1, psub_data[0]->data.var, sizeof(psub_data[0]->data.var));
		memcpy(var2, psub_data[1]->data.var, sizeof(psub_data[1]->data.var));
		if (pdata->data.op_size == 8)
		{
			*(unsigned long long*)var0 = *(unsigned long long*)var1 + *(unsigned long long*)var2;
		}
		else
		{
			*(unsigned long*)var0 = *(unsigned long*)var1 + *(unsigned long*)var2;
		}

		if (pdata->op.root_flag == 1)
		{
			/*父项为root时，保留父项节点，合并到一个子项*/
			psub_data[0]->op.idef = 0;
			memset(psub_data[0]->op.sdef, 0, sizeof(psub_data[0]->op.sdef));
			psub_data[0]->data.idef = 2;
			memcpy(psub_data[0]->data.var, var0, sizeof(var0));
			memset(psub_data[0]->data.sdef, 0, sizeof(psub_data[0]->data.sdef));

			if (psub_data[0]->data.op_size == 8)
			{
				sprintf(psub_data[0]->data.sdef, "<0x%llx>", *(unsigned long long*)psub_data[0]->data.var);
			}
			else
			{
				sprintf(psub_data[0]->data.sdef, "<0x%lx>", *(unsigned long*)psub_data[0]->data.var);
			}

			pdata->op.idef = 0;
			memset(pdata->op.sdef, 0, sizeof(pdata->op.sdef));
			pdata->data.idef = 4;
			memset(pdata->data.var, 0, sizeof(pdata->data.var));
			memset(pdata->data.sdef, 0, sizeof(pdata->data.sdef));
			
			if (pdata->data.op_size == 1)
			{
				sprintf(pdata->data.sdef, "b[sp]");
			}
			else if (pdata->data.op_size == 2)
			{
				sprintf(pdata->data.sdef, "w[sp]");
			}
			else if (pdata->data.op_size == 4)
			{
				sprintf(pdata->data.sdef, "d[sp]");
			}
			else if (pdata->data.op_size == 8)
			{
				sprintf(pdata->data.sdef, "q[sp]");
			}

			*sub_index = 1;
			iret = 1;
		}
		else
		{
			pdata->op.idef = 1;
			memset(pdata->op.sdef, 0, sizeof(pdata->op.sdef));
			pdata->data.idef = 2;
			memcpy(pdata->data.var, var0, sizeof(var0));
			memset(pdata->data.sdef, 0, sizeof(pdata->data.sdef));

			if (pdata->data.op_size == 8)
			{
				sprintf(pdata->data.sdef, "<0x%llx>", *(unsigned long long*)pdata->data.var);
			}
			else
			{
				sprintf(pdata->data.sdef, "<0x%lx>", *(unsigned long*)pdata->data.var);
			}

			*sub_index = 0;
			iret = 2;
		}

		//zlog_info(zc, "===xx_opmz_tree_rule_idef3  add ===\n");
		//zlog_info(zc, "data.idef:[%x] data.sdef:[%s] data.op_size:[%x] data.var:[%x]  data.offset:[%x]\n", \
		//	pdata->data.idef, pdata->data.sdef, pdata->data.op_size, *(int*)pdata->data.var, pdata->data.offset);

		//zlog_info(zc, "op.idef:[%x] op.src_seq:[%d]op.src_idef:[%x] op.sdef:[%s] op.ivar:[%x] \n", \
		//	pdata->op.idef, pdata->op.src_seq, pdata->op.src_idef, pdata->op.sdef, pdata->op.ivar);

		break;
	case 3:
		/*取反，一个子项*/
		if (sub_num != 1)
		{
			return 0;
		}

		//var32_1 = *(unsigned long*)psub_data[0]->data.var;
		//var32 = ~var32_1;

		memset(var1, 0, sizeof(var1));
		memset(var0, 0, sizeof(var0));

		memcpy(var1, psub_data[0]->data.var, sizeof(psub_data[0]->data.var));

		if (pdata->data.op_size == 8)
		{
			*(unsigned long long*)var0 = ~*(unsigned long long*)var1 ;
		}
		else
		{
			*(unsigned long*)var0 = ~*(unsigned long*)var1;
		}

		if (pdata->op.root_flag == 1)
		{
			/*父项为root时，保留父项节点，合并到一个子项*/
			psub_data[0]->op.idef = 0;
			memset(psub_data[0]->op.sdef, 0, sizeof(psub_data[0]->op.sdef));
			psub_data[0]->data.idef = 2;
			memcpy(psub_data[0]->data.var, var0, sizeof(var0));
			memset(psub_data[0]->data.sdef, 0, sizeof(psub_data[0]->data.sdef));

			if (psub_data[0]->data.op_size == 8)
			{
				sprintf(psub_data[0]->data.sdef, "<0x%llx>", *(unsigned long long*)psub_data[0]->data.var);
			}
			else
			{
				sprintf(psub_data[0]->data.sdef, "<0x%lx>", *(unsigned long*)psub_data[0]->data.var);
			}

			pdata->op.idef = 1;
			memset(pdata->op.sdef, 0, sizeof(pdata->op.sdef));
			pdata->data.idef = 4;
			memset(pdata->data.var, 0, sizeof(pdata->data.var));
			memset(pdata->data.sdef, 0, sizeof(pdata->data.sdef));
			
			if (pdata->data.op_size == 1)
			{
				sprintf(pdata->data.sdef, "b[sp]");
			}
			else if (pdata->data.op_size == 2)
			{
				sprintf(pdata->data.sdef, "w[sp]");
			}
			else if (pdata->data.op_size == 4)
			{
				sprintf(pdata->data.sdef, "d[sp]");
			}
			else if (pdata->data.op_size == 8)
			{
				sprintf(pdata->data.sdef, "q[sp]");
			}

			*sub_index = 0;
			iret = 0;
		}
		else
		{
			pdata->op.idef = 0;
			memset(pdata->op.sdef, 0, sizeof(pdata->op.sdef));
			pdata->data.idef = 2;
			memcpy(pdata->data.var, var0, sizeof(var0));
			memset(pdata->data.sdef, 0, sizeof(pdata->data.sdef));

			if (pdata->data.op_size == 8)
			{
				sprintf(pdata->data.sdef, "<0x%llx>", *(unsigned long long*)pdata->data.var);
			}
			else
			{
				sprintf(pdata->data.sdef, "<0x%lx>", *(unsigned long*)pdata->data.var);
			}

			*sub_index = 0;
			iret = 2;
		}

		//zlog_info(zc, "===xx_opmz_tree_rule_idef3  not ===\n");
		//zlog_info(zc, "data.idef:[%x] data.sdef:[%s] data.op_size:[%x] data.var:[%x]  data.offset:[%x]\n", \
		//	pdata->data.idef, pdata->data.sdef, pdata->data.op_size, *(int*)pdata->data.var, pdata->data.offset);

		//zlog_info(zc, "op.idef:[%x] op.src_seq:[%d]op.src_idef:[%x] op.sdef:[%s] op.ivar:[%x] \n", \
		//	pdata->op.idef, pdata->op.src_seq, pdata->op.src_idef, pdata->op.sdef, pdata->op.ivar);

		break;
	}



	/*处理子项，返回1*/
	return iret;
}





/*
定义优化回调函数
特殊常量合并，子项都为常量或者常量内存的，不存在多个常量的指令
*/
__declspec(noinline) int xx_opmz_tree_rule_idef4(struct ST_OP_DATA *pdata, int sub_num, struct ST_OP_DATA **psub_data, int*sub_index)
{
	int n = 0;
	int iret = 0;
	int seq = 0;
	struct ST_VM_ITEM_DATA item_data;

	/*判断父项属性，如果为虚值则处理*/
	if (pdata->data.idef != 4)
	{
		return 0;
	}

	/*判断是否所有的子项都为常量项*/
	for (n = 0; n < sub_num; n++)
	{
		if (psub_data[n]->data.idef != 2 && psub_data[n]->data.idef != 7)
		{
			/*有一个不是常量或者不是常量内存就退出*/
			return 0;
		}
	}

	/*这里直接取结果项，不做运算处理*/
	seq = pdata->op.src_seq;

	/*从文件中读取项数据*/
	memset(&item_data, 0, sizeof(struct ST_VM_ITEM_DATA));
	iret = xx_read_file(ginst_dfile, (char*)&item_data, seq * sizeof(struct ST_VM_ITEM_DATA), sizeof(struct ST_VM_ITEM_DATA));
	if (iret == 0)
	{
		zlog_error(zc, "xx_read_file seq:[%d]", seq);
		return 0;
	}

	if (pdata->op.root_flag == 1)
	{
		/*父项为root时，保留父项节点，合并到一个子项*/
		psub_data[0]->op.idef = 0;
		memset(psub_data[0]->op.sdef, 0, sizeof(psub_data[0]->op.sdef));
		psub_data[0]->data.idef = 2;
		memcpy(psub_data[0]->data.var, (char*)item_data.result, psub_data[0]->data.op_size);
		memset(psub_data[0]->data.sdef, 0, sizeof(psub_data[0]->data.sdef));

		if (psub_data[0]->data.op_size == 8)
		{
			sprintf(psub_data[0]->data.sdef, "<0x%llx>", *(unsigned long long*)psub_data[0]->data.var);
		}
		else
		{
			sprintf(psub_data[0]->data.sdef, "<0x%lx>", *(unsigned long*)psub_data[0]->data.var);
		}

		pdata->op.idef = 1;
		memset(pdata->op.sdef, 0, sizeof(pdata->op.sdef));
		pdata->data.idef = 4;
		memset(pdata->data.var, 0, sizeof(pdata->data.var));
		memset(pdata->data.sdef, 0, sizeof(pdata->data.sdef));
		
		if (pdata->data.op_size == 1)
		{
			sprintf(pdata->data.sdef, "b[sp]");
		}
		else if (pdata->data.op_size == 2)
		{
			sprintf(pdata->data.sdef, "w[sp]");
		}
		else if (pdata->data.op_size == 4)
		{
			sprintf(pdata->data.sdef, "d[sp]");
		}
		else if (pdata->data.op_size == 8)
		{
			sprintf(pdata->data.sdef, "q[sp]");
		}

		*sub_index = 1;
		iret = 1;
	}
	else
	{
		pdata->op.idef = 0;
		memset(pdata->op.sdef, 0, sizeof(pdata->op.sdef));
		pdata->data.idef = 2;
		memcpy(pdata->data.var, (char*)item_data.result, pdata->data.op_size);
		memset(pdata->data.sdef, 0, sizeof(pdata->data.sdef));

		if (pdata->data.op_size == 8)
		{
			sprintf(pdata->data.sdef, "<0x%llx>", *(unsigned long long*)pdata->data.var);
		}
		else
		{
			sprintf(pdata->data.sdef, "<0x%lx>", *(unsigned long*)pdata->data.var);
		}
		

		*sub_index = 0;
		iret = 2;
	}

	//zlog_info(zc, "===xx_opmz_tree_rule_idef4   ===\n");
	//zlog_info(zc, "data.idef:[%x] data.sdef:[%s] data.op_size:[%x] data.var:[%x]  data.offset:[%x]\n", \
	//	pdata->data.idef, pdata->data.sdef, pdata->data.op_size, *(int*)pdata->data.var, pdata->data.offset);

	//zlog_info(zc, "op.idef:[%x] op.src_seq:[%d]op.src_idef:[%x] op.sdef:[%s] op.ivar:[%x] \n", \
	//	pdata->op.idef, pdata->op.src_seq, pdata->op.src_idef, pdata->op.sdef, pdata->op.ivar);

	/*处理子项，返回1*/
	return iret;
}







/*
定义优化回调函数
sp偏移合并化简
*/
__declspec(noinline) int xx_opmz_tree_rule_idef5(struct ST_OP_DATA *pdata, int sub_num, struct ST_OP_DATA **psub_data, int*sub_index)
{
	int n = 0;
	int iret = 0;
	int seq = 0;

	if (sub_num != 2)
	{
		return 0;
	}

	if (pdata->op.idef != 2)
	{
		return 0;
	}

	/*判断子项是否是sp和一个常数，并且为加法*/
	if (psub_data[0]->data.idef == 3 && psub_data[1]->data.idef == 2)
	{
		/*判断偏移*/
		if (pdata->data.start_offset != (psub_data[0]->data.offset + *(unsigned long*)psub_data[1]->data.var))
		{
			return 0;
		}
	}
	else if (psub_data[0]->data.idef == 2 && psub_data[1]->data.idef == 3)
	{
		/*判断偏移*/
		if (pdata->data.start_offset != (psub_data[1]->data.offset + *(unsigned long*)psub_data[0]->data.var))
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}


	/*化简*/
	pdata->op.idef = 0;
	memset(pdata->op.sdef, 0, sizeof(pdata->op.sdef));

	pdata->data.idef = 3;
	memset(pdata->data.var, 0, sizeof(pdata->data.var));
	memset(pdata->data.sdef, 0, sizeof(pdata->data.sdef));

	sprintf(pdata->data.sdef, "sp");

	iret = 2;

	//zlog_info(zc, "===xx_opmz_tree_rule_idef5   ===\n");
	//zlog_info(zc, "data.idef:[%x] data.sdef:[%s] data.op_size:[%x] data.var:[%x]  data.offset:[%x]\n", \
	//	pdata->data.idef, pdata->data.sdef, pdata->data.op_size, *(int*)pdata->data.var, pdata->data.offset);

	//zlog_info(zc, "op.idef:[%x] op.src_seq:[%d]op.src_idef:[%x] op.sdef:[%s] op.ivar:[%x] \n", \
	//	pdata->op.idef, pdata->op.src_seq, pdata->op.src_idef, pdata->op.sdef, pdata->op.ivar);

	/*处理子项，返回1*/
	return iret;
}







//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/*
逻辑运算
*/
__declspec(noinline) int xx_opmz_tree_rule_exp1(struct ST_OP_DATA *pdata, char *pexp, int sub_num, char **psub_exp, int*sub_index)
{
	int n = 0;
	int iret = 0;

	switch (pdata->op.idef)
	{
	case 4:
		/*and*/
		if (sub_num != 2)
		{
			return 0;
		}
		if (psub_exp[0] == 0 || psub_exp[1] == 0)
		{
			return 0;
		}

		if (memcmp(psub_exp[0], psub_exp[1], strlen(psub_exp[0])) != 0)
		{
			return 0;
		}
		/*如果子项表达式一样，则化简*/

		/*将父项转化为虚值*/
		pdata->op.idef = 1;
		memset(pdata->op.sdef, 0, sizeof(pdata->op.sdef));
		pdata->data.idef = 4;
		memset(pdata->data.var, 0, sizeof(pdata->data.var));
		memset(pdata->data.sdef, 0, sizeof(pdata->data.sdef));

		iret = 1;
		*sub_index = 1;
		break;
	case 5:
		/*or*/
		if (sub_num != 2)
		{
			return 0;
		}
		if (psub_exp[0] == 0 || psub_exp[1] == 0)
		{
			return 0;
		}

		if (memcmp(psub_exp[0], psub_exp[1], strlen(psub_exp[0])) != 0)
		{
			return 0;
		}
		/*如果子项表达式一样，则化简*/

		/*将父项转化为虚值*/
		pdata->op.idef = 1;
		memset(pdata->op.sdef, 0, sizeof(pdata->op.sdef));
		pdata->data.idef = 4;
		memset(pdata->data.var, 0, sizeof(pdata->data.var));
		memset(pdata->data.sdef, 0, sizeof(pdata->data.sdef));

		iret = 1;
		*sub_index = 1;
		break;

	}

	return iret;
}






