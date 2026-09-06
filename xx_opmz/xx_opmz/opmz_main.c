#include <Windows.h>
#include "st_tree.h"
#include "xx_comm32.h"
#include "xx_opmz.h"
#include "xx_def_api.h"
#include "xx_exp_data.h"
#include "zlog.h"







///////////////////////////////////////////////////////////////////////////////////////////////////////////
int xx_opmz_start(struct ST_VM_INST *pinst, struct ST_TREE **ptree_inst, struct XX_OPMZ_INST **popmz_inst, int num, int lv);
int  xx_opmz_lv(struct ST_VM_INST *pinst, struct ST_TREE **ptree_inst, struct XX_OPMZ_INST **popmz_inst, int num, int lv);
int func_cb_print(void *pdata);
int xx_opmz_tree_inst(struct ST_VM_INST *pvm_inst, struct ST_TREE **ptree_inst, int num);
int  xx_opmz_lv1(struct ST_VM_INST *pvm_inst, struct XX_OPMZ_INST **popmz_inst, int num);
int  xx_opmz_log(struct ST_VM_INST *pvm_inst, struct XX_OPMZ_INST **popmz_inst, int num);
int xx_opmz_tree_merge(struct ST_TREE *ptree1, struct ST_TREE **pptree2, struct ST_VM_INST *pvm_inst2);

void xx_opmz_creg(struct ST_VM_INST *pvm_inst, int num);
///////////////////////////////////////////////////////////////////////////////////////////////////////////
extern zlog_category_t *zc;
extern char ginst_dfile[0x500];
extern char gopmz_log[0x500];

extern struct ST_TREE *xx_opmz_tree_gen(struct ST_VM_INST *pinst, int in_offset, int *pout_offset);
extern int func_cb_check(void *pdata);
extern void func_cb_set_start_offset(void *pdata, int idata);
extern int func_cb_cmp_data(void *pdata1, void *pdata2);
extern int func_cb_node_exp(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
extern int func_cb_exp(struct ST_OP_DATA *pdata, char *proot_exp, char **pret_exp);
extern int func_cb_cmp_sepdata(void *pdata1, void *pdata2);
extern int func_cb_cmp_expddata(void *pdata1, void *pdata2);
extern int xx_exp_to_asm(char *pexp, struct XX_EXP * out_exp, int lv);
extern void func_cb_set_idef(void *pdata, int idef);



extern void xx_opmz_tree_rule_idef(struct ST_TREE *ptree, int lv);
extern void xx_opmz_tree_rule_exp(struct ST_TREE *ptree, int lv);


///////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
还原主函数
参数1.输入指令组
参数2.指令数
*/
__declspec(noinline) int xx_opmz(struct ST_VM_INST *pvm_inst,int num)
{
	int n = 0;
	int iret = 0;
	//还原指令的结构指针数组，有结构则有还原指令
	struct XX_OPMZ_INST **popmz_inst = 0;
	struct ST_TREE **ptree_inst = 0;


	//zlog_info(zc, "=========xx_opmz start [%d]-[%d] num:[%d]=========\n", pvm_inst[0].seq, pvm_inst[num - 1].seq, num);

	/*参数检查*/
	if (pvm_inst == 0 || num == 0)
	{
		zlog_error(zc, "pvm_inst:[%x] num:[%d]\n", pvm_inst, num);
		return 0;
	}

	/*设置常量寄存器*/
	xx_opmz_creg(pvm_inst, num);

	/*申请还原指令空间*/
	popmz_inst = malloc(sizeof(struct XX_OPMZ_INST*)*num);
	if (popmz_inst == 0)
	{
		zlog_error(zc, "malloc error.popmz_inst\n");
		goto l_err;
	}
	memset(popmz_inst, 0, sizeof(struct XX_OPMZ_INST*)*num);

	/*申请指令树空间*/
	ptree_inst = malloc(sizeof(struct ST_TREE*)*num);
	if (ptree_inst == 0)
	{
		zlog_error(zc, "malloc error.ptree_inst\n");
		goto l_err;
	}
	memset(ptree_inst, 0, sizeof(struct ST_TREE*)*num);

	/*生成指令树数组*/
	iret = xx_opmz_tree_inst(pvm_inst, ptree_inst, num);
	if (iret == 0)
	{
		zlog_error(zc, "xx_opmz_tree_inst\n");
		goto l_err;
	}

	/*预处理考虑标志位操作，将标志位操作转化为标志位指令*/
	iret= xx_opmz_lv1(pvm_inst, popmz_inst, num);
	if (iret == 0)
	{
		zlog_error(zc, "xx_opmz_lv 1\n");
		goto l_err;
	}

	/*还原等级2，只还原中间指令*/
	iret = xx_opmz_lv(pvm_inst, ptree_inst, popmz_inst, num, OPMZ_LV2);
	if (iret == 0)
	{
		zlog_error(zc, "xx_opmz_lv 2\n");
		goto l_err;
	}

	/*还原等级3*/
	iret = xx_opmz_lv(pvm_inst, ptree_inst, popmz_inst, num, OPMZ_LV3);
	if (iret == 0)
	{
		zlog_error(zc, "xx_opmz_lv 3\n");
		goto l_err;
	}

	/*还原等级4，push相关的指令*/
	iret = xx_opmz_lv(pvm_inst, ptree_inst, popmz_inst, num, OPMZ_LV4);
	if (iret == 0)
	{
		zlog_error(zc, "xx_opmz_lv 4\n");
		goto l_err;
	}

	/*输出还原信息*/
	iret = xx_opmz_log(pvm_inst, popmz_inst, num);
	if (iret == 0)
	{
		zlog_error(zc, "xx_opmz_log\n");
		goto l_err;
	}

	

	/*释放空间*/
	if (popmz_inst != 0)
	{
		for (n = 0; n < num; n++)
		{
			if (popmz_inst[n] != 0)
			{
				free(popmz_inst[n]);
			}
		}
		free(popmz_inst);
	}

	if (ptree_inst != 0)
	{
		for (n = 0; n < num; n++)
		{
			if (ptree_inst[n] != 0)
			{
				xx_tree_free(ptree_inst[n]);
			}
		}
		free(ptree_inst);
	}

	//zlog_info(zc, "=========xx_opmz finish [%d]-[%d] num:[%d]=========\n", pvm_inst[0].seq, pvm_inst[num - 1].seq, num);

	return 1;

l_err:

	

	/*释放空间*/
	if (popmz_inst != 0)
	{
		for (n = 0; n < num; n++)
		{
			if (popmz_inst[n] != 0)
			{
				free(popmz_inst[n]);
			}
		}
		free(popmz_inst);
	}

	if (ptree_inst != 0)
	{
		for (n = 0; n < num; n++)
		{
			if (ptree_inst[n] != 0)
			{
				xx_tree_free(ptree_inst[n]);
			}
		}
		free(ptree_inst);
	}

	//zlog_info(zc, "=========xx_opmz error [%d]-[%d] num:[%d]=========\n", pvm_inst[0].seq, pvm_inst[num - 1].seq, num);

	return 0;
}




__declspec(noinline) int  xx_opmz_lv(struct ST_VM_INST *pvm_inst,  struct ST_TREE **ptree_inst, struct XX_OPMZ_INST **popmz_inst, int num, int lv)
{
	int n = 0;
	int iret = 0;
	int seq = 0;
	int tree_analyze = 0;

	//zlog_info(zc, "=========xx_opmz_lv start [%d]-[%d] num:[%d] lv:[%d]=========\n", pvm_inst[0].seq, pvm_inst[num - 1].seq, num, lv);


	/*参数检查*/
	if (pvm_inst == 0 || ptree_inst == 0 || popmz_inst == 0 || num == 0 || lv == 0)
	{
		zlog_error(zc, "pvm_inst:[%x] ptree_inst:[%x] popmz_inst:[%x] num:[%x] lv:[%d]\n", \
			pvm_inst, ptree_inst, popmz_inst, num, lv);
		return 0;
	}

	/*初始化分析数据等级*/
	iret = def_api_init(lv);
	if (iret == 0)
	{
		return 0;
	}

	/*遍历指令组，找到分析标志的指令，开始分析*/
	for (n = 0; n < num; n++)
	{
		/*过滤已优化指令,优化等级大于20，则过滤*/
		if (pvm_inst[n].opmz_flag >= OPMZ_LV2)
		{
			continue;
		}

		/*获取分析标志*/
		tree_analyze = def_api_tree_analyze(pvm_inst[n].idef);
		if (tree_analyze == 1)
		{

			/*传入指令组，开始生成树xx_opmz_start，指令组的开始和结束*/
			//memset(&out_opmz, 0, sizeof(out_opmz));
			iret = xx_opmz_start(pvm_inst, ptree_inst, popmz_inst, n + 1, lv);
			if (iret == 0)
			{
				return 0;
			}

		}
	}

	//zlog_info(zc, "=========xx_opmz_lv finish [%d]-[%d] num:[%d] lv:[%d]=========\n", pvm_inst[0].seq, pvm_inst[num - 1].seq, num, lv);

	return 1;
}


/*
对输入指令进行还原
如果能生成完整的指令树，则为一个指令，对还原结构赋值
*/
__declspec(noinline) int xx_opmz_start(struct ST_VM_INST *pvm_inst, struct ST_TREE **ptree_inst, struct XX_OPMZ_INST **popmz_inst, int num, int lv)
{
	int n = 0;
	int m = 0;
	int iret = 0;
	int idef = 0;
	struct ST_TREE *ptree = 0;
	struct ST_TREE *ret_tree = 0;
	struct XX_EXP out_exp;
	int inst_flag = 0;
	struct ST_OP_DATA *proot_data = 0;

	//zlog_info(zc, "=========xx_opmz_start start [%d]-[%d] num:[%d] lv:[%d]=========\n", pvm_inst[0].seq, pvm_inst[num - 1].seq, num, lv);

	/*参数检查*/
	if (pvm_inst == 0 || ptree_inst == 0 || popmz_inst == 0 || num == 0 || lv == 0)
	{
		zlog_error(zc, "pvm_inst:[%x] ptree_inst:[%x] popmz_inst:[%x] num:[%x] lv:[%d]\n", \
			pvm_inst, ptree_inst, popmz_inst, num, lv);
		return 0;
	}

	/*待操作的指令树需要重新生成*/
	//ptree = ptree_inst[num - 1];
	ptree = xx_tree_copy(ptree_inst[num - 1]);
	if (ptree == 0)
	{
		return 0;
	}
	pvm_inst[num - 1].opmz_flag = OPMZ_LV_TMP;

	/*设置root标志*/
	proot_data = xx_tree_root_data(ptree);
	if (proot_data == 0)
	{
		zlog_error(zc, "xx_tree_root_data seq:[%d]\n", pvm_inst[n].seq);
		return 0;
	}
	proot_data->op.root_flag = 1;

	/*生成树*/
	n = num - 2;
	for (n; n >= 0; n--)
	{
		/*如果是无效指令则退出*/
		if (ptree_inst[n] == 0)
		{
			break;
		}

		if (pvm_inst[n].opmz_flag == -1)
		{
			break;
		}
		
		/*过滤已优化指令,优化等级大于20，则过滤*/
		if (pvm_inst[n].opmz_flag >= OPMZ_LV2)
		{
			continue;
		}
		pvm_inst[n].opmz_flag = 0;

		/*
		将单条指令合并到树，合并失败就成功退出，否则会出现错误指令
		由于有中间指令参与，所以失败继续分析
		*/
		iret = xx_opmz_tree_merge(ptree, &ptree_inst[n], &pvm_inst[n]);
		//iret = xx_tree_merge(ptree, ptree_inst[n], func_cb_cmp_data);
		if (iret == 0)
		{
			continue;
			//break;
		}

		/*合并成功，优化标志做临时标记*/
		if (pvm_inst[n].opmz_flag == 0)
		{
			pvm_inst[n].opmz_flag = OPMZ_LV_TMP;
		}

		//zlog_info(zc, "xx_tree_merge success ptree_seq:[%d] seq:[%d]\n", pvm_inst[num - 1].seq, pvm_inst[n].seq);

		/*完整性检查，最低级检查*/
		iret = xx_tree_low_check(ptree, func_cb_check);
		if (iret == 0)
		{
			continue;
		}

		/*获取起始start_offset，并设置*/
		if (n == 0)
		{
			/*序号错误，报错*/
			zlog_error(zc, "n:[%d]\n", n);
		}
		proot_data = xx_tree_root_data(ptree_inst[n - 1]);
		if (proot_data == 0)
		{
			zlog_error(zc, "xx_tree_root_data seq:[%d][%d]\n", pvm_inst[n].seq, pvm_inst[n].src_seq);
			//return 0;
			break;
		}
		/*设置全树的start_offset*/
		xx_tree_set_idata(ptree, func_cb_set_start_offset, proot_data->data.offset);
		//zlog_info(zc, "xx_tree_set_idata offset:[%d]\n", proot_data->data.offset);

		//zlog_info(zc, "xx_tree_check success\n");
		//zlog_info(zc, "=========xx_tree_print start [%d]-[%d]=========\n", pvm_inst[n].seq, pvm_inst[num - 1].seq);
		xx_tree_print(ptree, func_cb_print);
		//zlog_info(zc, "=========xx_tree_print finish [%d]-[%d]=========\n", pvm_inst[n].seq, pvm_inst[num - 1].seq);

		/*进行基于定义的优化，等级由优化内部决定*/
		xx_opmz_tree_rule_idef(ptree, lv);
		

		//zlog_info(zc, "=========xx_tree_print xx_opmz_tree_rule_idef start [%d]-[%d]=========\n", pvm_inst[n].seq, pvm_inst[num - 1].seq);
		xx_tree_print(ptree, func_cb_print);
		//zlog_info(zc, "=========xx_tree_print xx_opmz_tree_rule_idef finish [%d]-[%d]=========\n", pvm_inst[n].seq, pvm_inst[num - 1].seq);

		/*生成节点表达式*/
		iret = xx_tree_node_exp(ptree, func_cb_node_exp);
		if (iret == 0)
		{
			//zlog_info(zc, "xx_tree_node_exp error 1 [%d]-[%d]\n", pvm_inst[n].seq, pvm_inst[num - 1].seq);
			break;
		}

		/*生成完整表达式，，测试*/
		iret = xx_tree_exp(ptree, func_cb_exp);
		if (iret == 0)
		{
			//zlog_info(zc, "xx_tree_exp error [%d]-[%d]\n", pvm_inst[n].seq, pvm_inst[num - 1].seq);
			break;
		}
		//zlog_info(zc, "xx_tree_exp: %s \n", ptree->pexp);

		/*进行基于表达式的优化，等级由优化内部决定*/
		xx_opmz_tree_rule_exp(ptree, lv);

		//zlog_info(zc, "=========xx_tree_print xx_opmz_tree_rule_exp start [%d]-[%d]=========\n", pvm_inst[n].seq, pvm_inst[num - 1].seq);
		xx_tree_print(ptree, func_cb_print);
		//zlog_info(zc, "=========xx_tree_print xx_opmz_tree_rule_exp finish [%d]-[%d]=========\n", pvm_inst[n].seq, pvm_inst[num - 1].seq);

		/*通过表达式优化节点后，清空表达式，重新生成*/
		xx_tree_exp_free(ptree);

		/*表达式优化后，重新生成节点表达式*/
		iret = xx_tree_node_exp(ptree, func_cb_node_exp);
		if (iret == 0)
		{
			//zlog_info(zc, "xx_tree_node_exp error 2 [%d]-[%d]\n", pvm_inst[n].seq, pvm_inst[num - 1].seq);
			break;
		}

		/*生成完整表达式*/
		iret = xx_tree_exp(ptree, func_cb_exp);
		if (iret == 0)
		{
			//zlog_info(zc, "xx_tree_exp error [%d]-[%d]\n", pvm_inst[n].seq, pvm_inst[num - 1].seq);
			break;
		}

		//zlog_info(zc, "xx_tree_exp: %s \n", ptree->pexp);


		memset(&out_exp, 0, sizeof(out_exp));
		iret = xx_exp_to_asm(ptree->pexp, &out_exp, lv);
		if (iret == 0)
		{
			//zlog_info(zc, "xx_exp_to_asm lv:[%d] [%d]-[%d] error [%s]\n", lv, pvm_inst[n].seq, pvm_inst[num - 1].seq, ptree->pexp);
			break;
		}

		/*匹配到正确的指令则还原成功，可能会生成错误的指令*/
		//zlog_info(zc, "out_exp idef:[%x] pasm_inst:[%s]\n", out_exp.idef, out_exp.pasm_inst);

		/*有还原指令的标志*/
		inst_flag = 1;

		/*检查忽略标志，忽略则不产生新指令*/
		if (out_exp.mark_flag == 1)
		{
			break;
		}

		if (out_exp.idef)
		{
			/*如果是中间指令，则重新定义指令，并取消中间指令的标记，参与后面的分析*/
			pvm_inst[num - 1].idef = out_exp.idef;
			pvm_inst[num - 1].opmz_flag = 0;
			/*中间指令的数据只取了第一个*/
			memcpy(pvm_inst[num - 1].var, out_exp.exp_var[0].ivar, g_curt_sys);

			/*中间指令则重新生成该指令的指令树*/

			/*获取原指令树的根节点数据，取到原指令执行前的sp_offset，*/
			if (n == 0)
			{
				/*序号错误，报错*/
				zlog_error(zc, "n:[%d]\n", n);
			}

			/*从前一个树取offset*/
			proot_data = xx_tree_root_data(ptree_inst[n - 1]);
			if (proot_data == 0)
			{
				zlog_error(zc, "xx_tree_root_data seq:[%d]\n", pvm_inst[n-1].seq);
				return 0;
			}

			//zlog_info(zc, "xx_tree_root_data seq:[%d] offset:[%x]\n", pvm_inst[n-1].seq, proot_data->data.offset);

			/*生成新指令树*/
			ret_tree = xx_opmz_tree_gen(&pvm_inst[num - 1], proot_data->data.offset, 0);
			if (ret_tree == 0)
			{
				zlog_error(zc, "xx_opmz_tree_gen seq:[%d]\n", pvm_inst[n].seq);
				return 0;
			}

			//zlog_info(zc, "=========xx_tree_print start middle-inst=========\n");
			xx_tree_print(ret_tree, func_cb_print);
			//zlog_info(zc, "=========xx_tree_print finish middle-inst=========\n");

			/*释放原指令树*/
			xx_tree_free(ptree_inst[num - 1]);

			ptree_inst[num - 1] = ret_tree;

			//zlog_info(zc, "middle-inst out_exp idef:[%x] pasm_inst:[%s]\n", out_exp.idef, out_exp.pasm_inst);

		}
		else
		{
			popmz_inst[num - 1] = malloc(sizeof(struct XX_OPMZ_INST));
			if (popmz_inst[num - 1] == 0)
			{
				zlog_error(zc, "malloc popmz_inst[%d]error\n",n);
				return 0;
			}
			memset(popmz_inst[num - 1], 0, sizeof(struct XX_OPMZ_INST));

			if (strlen(out_exp.pasm_inst) > sizeof(popmz_inst[num - 1]->asm))
			{
				zlog_error(zc, "len out_exp.pasm_inst:[%s]\n", strlen(out_exp.pasm_inst));
				break;
			}

			/*完整则生成还原指令，还原指令的存放序号与start_seq相同，此时为n*/
			memcpy(popmz_inst[num - 1]->asm, out_exp.pasm_inst, strlen(out_exp.pasm_inst));
			popmz_inst[num - 1]->start_seq = pvm_inst[n].seq;
			popmz_inst[num - 1]->end_seq = pvm_inst[num - 1].seq;
			
			//zlog_info(zc, "opmz-inst out_exp idef:[%x] pasm_inst:[%s]\n", out_exp.idef, out_exp.pasm_inst);
			
		}
		break;
	}

	/*生成了还原指令，则将临时标记改为已优化标记*/
	if (inst_flag == 1)
	{
		for (n = 0; n < num; n++)
		{
			if (pvm_inst[n].opmz_flag == OPMZ_LV_TMP)
			{
				pvm_inst[n].opmz_flag = lv;
			}
		}
	}
	else
	{
		/*取消临时标记*/
		for (n = 0; n < num; n++)
		{
			if (pvm_inst[n].opmz_flag == OPMZ_LV_TMP)
			{
				pvm_inst[n].opmz_flag = 0;
			}
		}
	}

	//zlog_info(zc, "=========xx_opmz_start finish [%d]-[%d] num:[%d] lv:[%d]=========\n", pvm_inst[0].seq, pvm_inst[num - 1].seq, num, lv);

	/*释放还原指令树*/
	if (ptree)
	{
		xx_tree_free(ptree);
	}

	return 1;

}


/*
输入指令组，生成指令树数组
指令树数组在优化前生成
*/
__declspec(noinline) int xx_opmz_tree_inst(struct ST_VM_INST *pvm_inst, struct ST_TREE **ptree_inst, int num)
{
	int n = 0;
	int iret = 0;
	int sp_offset = 0;
	int out_offset = 0;
	struct ST_TREE *ret_tree = 0;

	//zlog_info(zc, "=========xx_opmz_tree_inst start =========\n");

	/*参数检查*/
	if (pvm_inst == 0 || ptree_inst == 0 || num == 0)
	{
		zlog_error(zc, "pvm_inst:[%x] ptree_inst:[%x] num:[%d] \n", pvm_inst, ptree_inst, num);
		return 0;
	}

	/*初始化指令表，这个指令表应该为总表*/
	iret = def_api_init(OPMZ_LV4);
	if (iret == 0)
	{
		return 0;
	}

	sp_offset = 0;
	
	for (n = 0; n < num; n++)
	{

		out_offset = 0;

		/*单条指令生成树,因为是正向，所以out_offset获取执行后的sp变化*/
		ret_tree = xx_opmz_tree_gen(&pvm_inst[n], sp_offset, &out_offset);
		if (ret_tree == 0)
		{
			//zlog_info(zc, "xx_opmz_tree_gen n:[%d] idef:[%x] seq:[%d] \n", n,pvm_inst[n].idef, pvm_inst[n].seq);
			continue;
		}

		ptree_inst[n] = ret_tree;

		/*打印树*/
		//zlog_info(zc, "=========xx_tree_print start =========\n");
		xx_tree_print(ret_tree, func_cb_print);
		//zlog_info(zc, "=========xx_tree_print finish =========\n");

		sp_offset = sp_offset + out_offset;
	}

	//zlog_info(zc, "=========xx_opmz_tree_inst finish =========\n");

	return 1;
}


int func_cb_print(void *pdata)
{
	int iret = 0;
	struct ST_OP_DATA *p = 0;

	if (pdata == 0)
	{
		return 0;
	}

	p = pdata;

	//zlog_info(zc, "data.idef:[%x] data.sdef:[%s] data.op_size:[%x] data.var:[%x] data.vsize:[%x] data.offset:[%x]\n", \
	//	p->data.idef, p->data.sdef, p->data.op_size, *(XLADDR*)p->data.var, p->data.vsize, p->data.offset);

	//zlog_info(zc, "op.idef:[%x] op.src_seq:[%d]op.src_idef:[%x] op.sdef:[%s] op.ivar:[%x] \n", \
	//	p->op.idef, p->op.src_seq, p->op.src_idef, p->op.sdef, p->op.ivar);


	return 1;
}





/*
优化等级1
标志位指令
*/
__declspec(noinline) int  xx_opmz_lv1(struct ST_VM_INST *pvm_inst, struct XX_OPMZ_INST **popmz_inst, int num)
{
	int n = 0;
	int m = 0;
	int iret = 0;
	int seq = 0;
	int tree_analyze = 0;
	int sp_offset = 0;
	int out_offset = 0;
	int inst_seq = 0;

	//zlog_info(zc, "=========xx_opmz_lv1 start [%d]-[%d] num:[%d] =========\n", pvm_inst[0].seq, pvm_inst[num - 1].seq, num);


	/*参数检查*/
	if (pvm_inst == 0  || popmz_inst == 0 || num == 0 )
	{
		zlog_error(zc, "pvm_inst:[%x]  popmz_inst:[%x] num:[%x] \n", pvm_inst, popmz_inst, num);
		return 0;
	}


	/*遍历指令组，找到有标志位操作的指令*/
	while(1)
	{
		if (n >= num)
		{
			break;
		}

		/*查找有标志位操作的指令*/
		iret = def_api_rflag(pvm_inst[n].idef);
		if (iret == 1)
		{
			//zlog_info(zc, "def_api_rflag seq:[%d] =========\n", pvm_inst[n].seq);
			/*找到后，查找标志位寄存器赋值指令pop reg，从产生标志的指令开始*/
			m = n + 1;
			sp_offset = 0;
			while (1)
			{
				if (m >= num)
				{
					zlog_error(zc, "flag inst error\n");
					break;
				}

				iret = def_api_get_iseq(pvm_inst[m].idef, &inst_seq);
				if (iret == 0)
				{
					zlog_error(zc, "def_api_get_iseq error\n");
					return 0;
				}

				/*获取执行后的sp变化*/
				iret = def_api_get_sp_offset1(inst_seq, &out_offset);
				if (iret == 0)
				{
					zlog_error(zc, "def_api_get_sp_offset1 error\n");
					return 0;
				}

				sp_offset = sp_offset + out_offset;


				/*指令为pop reg，并且sp_offset为0*/
				if ((sp_offset == 4 && pvm_inst[m].idef == 0x0300) || \
					(sp_offset == 8 && pvm_inst[m].idef == 0x10303))
				{

					/*此指令为标志位寄存器赋值指令*/
					//zlog_info(zc, "inst seq:[%d] flag seq:[%d]=========\n", pvm_inst[n].seq, pvm_inst[m].seq);

					/*还原此标志位操作指令，暂时设置为OPMZ_LV2，不显示标志位指令*/
					pvm_inst[m].opmz_flag = OPMZ_LV2;

					//popmz_inst[m] = malloc(sizeof(struct XX_OPMZ_INST));
					//if (popmz_inst[m] == 0)
					//{
					//	zlog_error(zc, "malloc popmz_inst[%d]error\n",m);
					//	return 0;
					//}
					//memset(popmz_inst[m], 0, sizeof(struct XX_OPMZ_INST));

					/*标志位中间指令，不使用还原结构*/
					if (pvm_inst[m].idef == 0x0300)
					{
						//popmz_inst[m]->idef = 0x0a200;
						pvm_inst[m].idef = 0x0a200;
					}
					else if (pvm_inst[m].idef == 0x10303)
					{
						//popmz_inst[m]->idef = 0x0a201;
						pvm_inst[m].idef = 0x0a201;
					}
					else
					{
						zlog_error(zc, "idef error. seq:[%d]error\n", pvm_inst[m].seq);
						return 0;
					}


					break;

				}

				m++;
			}

			
		}

		n++;
	}

	//zlog_info(zc, "=========xx_opmz_lv1 finish [%d]-[%d] num:[%d] =========\n", pvm_inst[0].seq, pvm_inst[num - 1].seq, num);

	return 1;
}





__declspec(noinline) int  xx_opmz_log(struct ST_VM_INST *pvm_inst, struct XX_OPMZ_INST **popmz_inst, int num)
{
	int n = 0;
	int iret = 0;
	int seq = 0;
	char tmp[0x100];
	char str_rtn[] = "\r\n";

	for (n = 0; n < num; n++)
	{
		/*判断优化标志，依次输出，有还原指令或者中间指令就输出，没有需要输出的就忽略*/
		if (popmz_inst[n])
		{
			/*输出指令序号*/
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, " %08d: ", pvm_inst[n].src_seq);
			iret = xx_append_file(gopmz_log, tmp, strlen(tmp));
			if (iret == 0)
			{
				zlog_error(zc, "xx_append_file. seq:[%d]error\n", pvm_inst[n].src_seq);
				return 0;
			}

			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "%s", popmz_inst[n]->asm);
			iret = xx_append_file(gopmz_log, tmp, strlen(tmp));
			if (iret == 0)
			{
				zlog_error(zc, "xx_append_file. seq:[%d]error\n", pvm_inst[n].src_seq);
				return 0;
			}

			/*输出换行*/
			iret = xx_append_file(gopmz_log, str_rtn, sizeof(str_rtn));
			if (iret == 0)
			{
				zlog_error(zc, "xx_append_file. str_rtn\n");
				return 0;
			}
		}

		if (pvm_inst[n].opmz_flag < OPMZ_LV2)
		{
			/*输出指令序号*/
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, " %08d: ", pvm_inst[n].src_seq);
			iret = xx_append_file(gopmz_log, tmp, strlen(tmp));
			if (iret == 0)
			{
				zlog_error(zc, "xx_append_file. seq:[%d]error\n", pvm_inst[n].src_seq);
				return 0;
			}

			/*输出指令*/
			seq = 0;
			iret = def_api_get_iseq(pvm_inst[n].idef, &seq);
			if (iret == 0)
			{
				zlog_error(zc, "def_api_get_iseq. seq:[%d]error\n", pvm_inst[n].src_seq);
				return 0;
			}
			memset(tmp, 0, sizeof(tmp));
			iret = def_api_get_sdef(seq, tmp, sizeof(tmp));
			if (iret == 0)
			{
				zlog_error(zc, "def_api_get_sdef. seq:[%d]error\n", pvm_inst[n].src_seq);
				return 0;
			}

			iret = xx_append_file(gopmz_log, tmp, strlen(tmp));
			if (iret == 0)
			{
				zlog_error(zc, "def_api_get_sdef. seq:[%d]error\n", pvm_inst[n].src_seq);
				return 0;
			}

			/*输出换行*/
			iret = xx_append_file(gopmz_log, str_rtn, sizeof(str_rtn));
			if (iret == 0)
			{
				zlog_error(zc, "xx_append_file. str_rtn\n");
				return 0;
			}
		}

		

	}


	return 1;
}




/*
树合并函数
参数2：树2的指针地址
*/
__declspec(noinline) int xx_opmz_tree_merge(struct ST_TREE *ptree1, struct ST_TREE **pptree2, struct ST_VM_INST *pvm_inst2)
{
	int iret = 0;
	int seq = 0;
	int chg_flag = 0;
	int chg_idef = 0;
	int expd_idef1 = 0;
	int expd_idef2 = 0;
	struct  ST_OP_DATA  *pop_data = 0;
	struct  ST_OP_DATA  *pop_data2 = 0;
	struct ST_TREE *ptree2;
	struct ST_TREE *pret_tree = 0;
	struct ST_TREE *pexpd_tree1 = 0;
	struct ST_TREE *pexpd_tree2 = 0;
	struct ST_VM_INST vm_inst;


	ptree2 = *pptree2;

	//zlog_info(zc, "xx_opmz_tree_merge  pvm_inst2.seq:[%d]\n", pvm_inst2->seq);

	/*正常合并，树2是树1的最低级子项*/
	iret = xx_tree_merge(ptree1, ptree2, func_cb_cmp_data);
	if (iret != 0)
	{
		/*合并成功返回*/
		return 1;
	}

	pop_data2 = xx_tree_root_data(ptree2);
	if (pop_data2 == 0)
	{
		zlog_error(zc, "xx_tree_root_data idef:[%x] seq:[%d]\n", pvm_inst2->idef, pvm_inst2->seq);
		return 0;
	}


	/*正常合并失败，判断树2是否是树1的最低级子项的拆分项，返回树1该节点的数据*/
	pop_data = xx_tree_find_lowdata(ptree1, pop_data2, func_cb_cmp_sepdata);
	if (pop_data)
	{
		/*是树1子项的拆分项，拆分树1的子项*/

		/*根据返回的数据找到原指令，并判断是否由操作项生成*/

		//zlog_info(zc, "sep pop_data->op.src_idef:[%x] seq:[%d] src_item_flag:[%d] src_item_seq:[%d]\n", \
		//	pop_data->op.src_idef, pop_data->op.src_seq, pop_data->op.src_item_flag, pop_data->op.src_item_seq);
		if (pop_data->op.src_item_flag != 2)
		{
			return 0;
		}

		iret = def_api_get_iseq(pop_data->op.src_idef, &seq);
		if (iret == 0)
		{
			zlog_error(zc, "def_api_get_iseq\n");
			return 0;
		}

		chg_flag = def_api_item_get_chg(seq, pop_data->op.src_item_seq);
		if (chg_flag == 0)
		{
			zlog_error(zc, "def_api_item_get_chg\n");
			return 0;
		}

		/*判断原指令该项是否可以生成拆分指令*/
		if ((chg_flag & 0x00000001) != 1)
		{
			zlog_error(zc, "no seprate [%x]\n", pop_data->op.src_idef);
			return 0;
		}

		/*获取拆分指令定义*/
		chg_idef = def_api_item_get_sep_idef(seq, pop_data->op.src_item_seq);
		if (chg_idef == 0)
		{
			zlog_error(zc, "def_api_item_get_chg_idef\n");
			return 0;
		}

		//zlog_info(zc, "sep_idef:[%x]\n", chg_idef);

		/*生成拆分指令树*/
		memset(&vm_inst, 0, sizeof(vm_inst));
		vm_inst.idef = chg_idef;
		vm_inst.seq = pop_data->op.src_seq;
		pret_tree = xx_opmz_tree_gen(&vm_inst, pop_data->data.offset, 0);
		if (pret_tree == 0)
		{
			zlog_error(zc, "xx_opmz_tree_gen\n");
			return 0;
		}
		/*将树合并到树1*/
		iret = xx_tree_merge(ptree1, pret_tree, func_cb_cmp_data);
		if (iret == 0)
		{
			zlog_error(zc, "xx_tree_merge ptree1-pret_tree\n");
			return 0;
		}

		/*合并树2到树1*/
		iret = xx_tree_merge(ptree1, ptree2, func_cb_cmp_data);
		if (iret == 0)
		{
			zlog_error(zc, "xx_tree_merge ptree1-ptree2\n");
			return 0;
		}

		//zlog_info(zc, "xx_tree_merge seprate.\n");
		/*合并成功，返回1*/
		return 1;
	}

	/*正常合并失败，判断树2是否是树1的最低级子项的扩大项*/
	pop_data = xx_tree_find_lowdata(ptree1, pop_data2, func_cb_cmp_expddata);
	if (pop_data)
	{
		/*是树1子项的扩大项，扩大树1的子项*/

		/*并保留树2的根项的拆分中间指令*/

		/*根据返回的数据找到原指令*/
		//zlog_info(zc, "expd pop_data->op.src_idef:[%x] seq:[%d] src_item_flag:[%d] src_item_seq:[%d]\n", \
		//	pop_data->op.src_idef, pop_data->op.src_seq, pop_data->op.src_item_flag, pop_data->op.src_item_seq);


		/*判断树2的指令是否可以拆分*/
		iret = def_api_get_iseq(pop_data2->op.src_idef, &seq);
		if (iret == 0)
		{
			zlog_error(zc, "def_api_get_iseq\n");
			return 0;
		}

		chg_flag = def_api_rlst_get_chg(seq);
		if (chg_flag == 0)
		{
			zlog_error(zc, "def_api_item_get_chg\n");
			return 0;
		}

		/*判断原指令该项是否可以生成扩展指令*/
		if ((chg_flag & 0x00000002) != 2)
		{
			zlog_error(zc, "no expand [%x]\n", pop_data2->op.src_idef);
			return 0;
		}

		/*获取扩展指令定义1*/
		expd_idef1 = def_api_rlst_get_expd_idef1(seq);
		if (expd_idef1 == 0)
		{
			zlog_error(zc, "def_api_rlst_get_expd_idef1\n");
			return 0;
		}

		/*获取扩展指令定义2*/
		expd_idef2 = def_api_rlst_get_expd_idef2(seq);
		if (expd_idef2 == 0)
		{
			zlog_error(zc, "def_api_rlst_get_expd_idef2\n");
			return 0;
		}

		/*生成扩展指令树1*/
		memset(&vm_inst, 0, sizeof(vm_inst));
		vm_inst.idef = expd_idef1;
		vm_inst.seq = pop_data2->op.src_seq;
		pexpd_tree1 = xx_opmz_tree_gen(&vm_inst, pop_data2->data.offset, 0);
		if (pexpd_tree1 == 0)
		{
			zlog_error(zc, "xx_opmz_tree_gen pexpd_tree1\n");
			return 0;
		}

		/*生成扩展指令树2*/
		memset(&vm_inst, 0, sizeof(vm_inst));
		vm_inst.idef = expd_idef2;
		vm_inst.seq = pop_data2->op.src_seq;
		pexpd_tree2 = xx_opmz_tree_gen(&vm_inst, pop_data2->data.offset, 0);
		if (pexpd_tree2 == 0)
		{
			zlog_error(zc, "xx_opmz_tree_gen pexpd_tree1\n");
			return 0;
		}

		/*合并树2到扩展指令树1*/
		iret = xx_tree_merge(pexpd_tree1, ptree2, func_cb_cmp_data);
		if (iret == 0)
		{
			zlog_error(zc, "xx_tree_merge pexpd_tree1\n");
			return 0;
		}
		/*打印树*/
		//zlog_info(zc, "=========xx_tree_print pexpd_tree1 start =========\n");
		xx_tree_print(pexpd_tree1, func_cb_print);
		//zlog_info(zc, "=========xx_tree_print pexpd_tree1 finish =========\n");

		/*合并树2到扩展指令树2*/
		iret = xx_tree_merge(pexpd_tree2, ptree2, func_cb_cmp_data);
		if (iret == 0)
		{
			zlog_error(zc, "xx_tree_merge pexpd_tree2\n");
			return 0;
		}
		/*打印树*/
		//zlog_info(zc, "=========xx_tree_print pexpd_tree2 start =========\n");
		xx_tree_print(pexpd_tree2, func_cb_print);
		//zlog_info(zc, "=========xx_tree_print pexpd_tree2 finish =========\n");

		iret = xx_tree_merge(ptree1, pexpd_tree1, func_cb_cmp_data);
		if (iret != 0)
		{
			/*如果扩展树1合并到树1成功，则保留扩展指令2，修改树2的指令*/
			xx_tree_free(pexpd_tree1);
			xx_tree_free(ptree2);
			*pptree2 = pexpd_tree2;
			pvm_inst2->idef = expd_idef2;
			pvm_inst2->opmz_flag = OPMZ_LV1;

			/*设置pexpd_tree2的子项定义为8*/
			xx_tree_set_low_idata(pexpd_tree2, func_cb_set_idef, 8);

			//zlog_info(zc, "xx_tree_merge expand 1\n"); 
			//zlog_info(zc, "=========xx_tree_print ptree1 start =========\n");
			xx_tree_print(ptree1, func_cb_print);
			//zlog_info(zc, "=========xx_tree_print ptree1 finish =========\n");
			return 1;
		}

		iret = xx_tree_merge(ptree1, pexpd_tree2, func_cb_cmp_data);
		if (iret != 0)
		{
			/*如果扩展树2合并到树1成功，则保留扩展指令1，并修改树2的指令*/
			xx_tree_free(pexpd_tree2);
			xx_tree_free(ptree2);
			*pptree2 = pexpd_tree1;
			pvm_inst2->idef = expd_idef1;
			pvm_inst2->opmz_flag = OPMZ_LV1;

			/*设置pexpd_tree2的子项定义为8*/
			xx_tree_set_low_idata(pexpd_tree1, func_cb_set_idef, 8);

			//zlog_info(zc, "xx_tree_merge expand 2\n");
			//zlog_info(zc, "=========xx_tree_print ptree1 start =========\n");
			xx_tree_print(ptree1, func_cb_print);
			//zlog_info(zc, "=========xx_tree_print ptree1 finish =========\n");
			return 1;
		}


		/*如果扩展指令树1，2都合并失败，则失败*/

		/*释放扩展指令树*/
		xx_tree_free(pexpd_tree1);
		xx_tree_free(pexpd_tree2);
		//zlog_info(zc, "=========xx_tree_print ptree1 start =========\n");
		xx_tree_print(ptree1, func_cb_print);
		//zlog_info(zc, "=========xx_tree_print ptree1 finish =========\n");
	}

	return 0;
}





/*
判断是否是常数寄存器
不需要判断返回值
并设置常数寄存器
必须要设置常量寄存器的常量值，用于优化模块
有则覆盖，没有则不操作，

*/
__declspec(noinline) void xx_opmz_creg(struct ST_VM_INST *pvm_inst, int num)
{
	int n = 0;
	int iret = 0;
	struct ST_VM_ITEM_DATA item_data;

	/**/
	if (pvm_inst == 0 || num < 2)
	{
		return;
	}

	n = 0;
	if (pvm_inst[n].idef != 0x0000 && pvm_inst[n].idef != 0x0f00)
	{
		return;
	}

	if (pvm_inst[n + 1].idef == 0x0300)
	{

		/*只检查2个*/
		def_api_creg_index_set(*(int*)pvm_inst[n + 1].var);

		//zlog_info(zc, "piece_start:[%d] num:[%d] seq:[%d] creg_index:[%x]\n", pvm_inst[n].seq, num, pvm_inst[n + 1].seq, *(int*)pvm_inst[n + 1].var);

		/*从文件中读取项数据*/
		iret = xx_read_file(ginst_dfile, (char*)&item_data, pvm_inst[n + 1].seq * sizeof(struct ST_VM_ITEM_DATA), sizeof(struct ST_VM_ITEM_DATA));
		if (iret == 0)
		{
			zlog_error(zc, "xx_read_file seq:[%d]", pvm_inst[n + 1].seq);
			return;
		}

		//zlog_info(zc, "result:[%08x]\n", *(XLADDR*)item_data.result);
		iret = def_api_creg_var_set(4, item_data.result);
		if (iret == 0)
		{
			zlog_error(zc, "def_api_creg_var_set seq:[%d]", pvm_inst[n + 1].seq);
			return;
		}
	}


	if (pvm_inst[n + 1].idef == 0x10303)
	{
		/*只检查2个*/
		def_api_creg_index_set(*(int*)pvm_inst[n + 1].var);

		//zlog_info(zc, "piece_start:[%d] num:[%d] seq:[%d] creg_index:[%x]\n", pvm_inst[n].seq, num, pvm_inst[n + 1].seq, *(XLADDR*)pvm_inst[n + 1].var);

		iret = xx_read_file(ginst_dfile, (char*)&item_data, pvm_inst[n + 1].seq * sizeof(struct ST_VM_ITEM_DATA), sizeof(struct ST_VM_ITEM_DATA));
		if (iret == 0)
		{
			zlog_error(zc, "xx_read_file seq:[%d]", pvm_inst[n + 1].seq);
			return;
		}

		//zlog_info(zc, "result:[%016llx]\n", *(XLADDR*)item_data.result);
		iret = def_api_creg_var_set(8, item_data.result);
		if (iret == 0)
		{
			zlog_error(zc, "def_api_creg_var_set seq:[%d]", pvm_inst[n + 1].seq);
			return;
		}
	}


	return;
}

















