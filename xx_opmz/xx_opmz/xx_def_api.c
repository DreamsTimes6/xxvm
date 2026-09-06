#include "xx_def_inst_lv1.h"
#include "xx_def_inst_lv2.h"
#include "xx_def_inst_lv3.h"
#include "xx_def_inst_lv4.h"


XLADDR **def_tbl;
int   def_num;

/*常量寄存器索引*/
int g_creg_index;

/*常量寄存器的常量值，大小由当前环境的位数决定*/
unsigned char g_creg_var[0x10];

////////////////////  自定义指令操作  ///////////////////////////////

/*
参数1：优化等级
*/
__declspec(noinline) int def_api_init(int lv)
{
	def_tbl = 0;
	def_num = 0;

	switch (lv)
	{
	case OPMZ_LV1:
		def_num = 0;
		def_tbl = 0;
		break;
	case OPMZ_LV2:
		def_num = DEF_INST_LV2_NUM;
		def_tbl = def_inst_lv2_tbl;
		break;
	case OPMZ_LV3:
		def_num = DEF_INST_LV3_NUM;
		def_tbl = def_inst_lv3_tbl;
		break;
	case OPMZ_LV4:
		def_num = DEF_INST_LV4_NUM;
		def_tbl = def_inst_lv4_tbl;
		break;
	default:
		return 0;
	}
	return 1;
}




/*
获取指令的序号，其它操作全部使用索引序号
参数1：指令数字定义
参数2：返回指令序号
返回：成功-1；失败-0
*/
__declspec(noinline) int def_api_get_iseq(int idef,int *out_seq)
{
	int n = 0;
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num==0)
	{
		return 0;
	}

	for (n = 0; n < def_num; n++)
	{
		pdef = (struct XX_DEF_INST *)def_tbl[n];
		if (idef == pdef->idef)
		{
			*out_seq = n;
			return 1;
		}
	}
	return 0;
}


/*
获取指令示例文本
参数1：指令序号
参数2：接收缓冲区
返回：成功-1；失败-0
*/
__declspec(noinline) int def_api_get_sdef(int seq, char *pdata,int size)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}
	if (seq >= def_num || pdata == 0)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (size <= strlen(pdef->sdef))
	{
		return 0;
	}

	memcpy(pdata, pdef->sdef, strlen(pdef->sdef));

	return 1;
}

/*
获取指令是否必须标记
返回：1-标记，2-否
*/
__declspec(noinline) int def_api_get_mark(int idef)
{
	int n = 0;
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	for (n = 0; n < def_num; n++)
	{
		pdef = (struct XX_DEF_INST *)def_tbl[n];
		if (idef == pdef->idef)
		{
			if (pdef->mark_flag == 1)
			{
				return 1;
			}

		}
	}
	return 0;
}

/*
获取指令操作数字定义，
参数1：指令序号
参数2：返回数字定义
返回：成功-1；失败-0
*/
__declspec(noinline) int def_api_get_op_idef(int seq,int *out_idef)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}
	if (seq >= def_num )
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	*out_idef = pdef->op_idef;
	return 1;

}

/*
获取指令操作文本
参数1：指令序号
参数2：接收缓冲区
返回：成功-1；失败-0
*/
__declspec(noinline) int def_api_get_op_sdef(int seq, char *pdata,int size)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num || pdata == 0)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (size <= strlen(pdef->op_sdef))
	{
		return 0;
	}

	memcpy(pdata, pdef->op_sdef, strlen(pdef->op_sdef));

	return 1;
}


/*
获取指令是否参与分析树生成，1-是，0-否
参数1：指令序号
参数2：返回标志
返回：成功-1；失败-0
*/
__declspec(noinline) int def_api_get_tree_op(int seq,int *out_flag)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	*out_flag = pdef->tree_op;
	return 1;

}

/*
获取指令是否开始分析标志，1 - 开始，0 - 否
参数1：指令序号
参数2：返回标志
返回：成功-1；失败-0
*/
__declspec(noinline) int def_api_get_tree_analyze(int seq, int *out_flag)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	*out_flag = pdef->tree_analyze;

	return 1;
}

/*
查询tree_analyze标志操作
参数1：指令定义
返回：1-是；0-否
*/
__declspec(noinline) int def_api_tree_analyze(int idef)
{
	int n = 0;
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	for (n = 0; n < def_num; n++)
	{
		pdef = (struct XX_DEF_INST *)def_tbl[n];
		if (idef == pdef->idef)
		{
			if (pdef->tree_analyze == 1)
			{
				return 1;
			}
			
		}
	}
	return 0;
}



/*
获取指令是否继续分析标志，（分析树存在，是否继续分析）1 - 继续;2-只操作sp，不生成树;3-退出
参数1：指令序号
参数2：返回标志
返回：成功-1；失败-0
*/
__declspec(noinline) int def_api_get_tree_continue(int seq,int *out_flag)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	*out_flag = pdef->tree_continue;

	return 1;
}


/*
获取指令合并失败后下一步操作标志，1 - 忽略继续，0-退出
参数1：指令序号
参数2：返回标志
返回：成功-1；失败-0
*/
__declspec(noinline) int def_api_get_merge_flag(int seq,int *out_flag)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	*out_flag = pdef->merge_flag;

	return 1;
}

/*
查询rflag标志
参数1：指令定义
返回：1-有标志；0-没有
*/
__declspec(noinline) int def_api_rflag(int idef)
{
	int n = 0;
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	for (n = 0; n < def_num; n++)
	{
		pdef = (struct XX_DEF_INST *)def_tbl[n];
		if (idef == pdef->idef)
		{
			if (pdef->rflag == 1)
			{
				return 1;
			}

		}
	}
	return 0;
}


/*
获取指令执行后的sp变化，
参数1：指令序号
参数2：返回sp变化量
返回：成功-1；失败-0
*/
__declspec(noinline) int def_api_get_sp_offset1(int seq,int *out_offset)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	*out_offset = pdef->sp_offset;

	return 1;
}

/*
获取指令执行前的sp变化
参数1：指令序号
参数2：返回sp变化量
返回：成功-1；失败-0
*/
__declspec(noinline) int def_api_get_sp_offset2(int seq, int *out_offset)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	*out_offset = 0-(pdef->sp_offset);

	return 1;
}



////////////////////////////  结果项  /////////////////////////////////
/*
获取结果项的指令定义，如果结果项为指令变量
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
__declspec(noinline) int def_api_rlst_get_inst(int seq, int *out_inst)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (pdef->result.item_inst != 0)
	{
		*out_inst = pdef->result.item_inst;
		return 1;
	}
	else
	{
		return 0;
	}
}



/*
获取结果项的数字定义
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
__declspec(noinline) int def_api_rlst_get_idef(int seq, int *out_idef)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (pdef->result.item_idef != 0)
	{
		*out_idef = pdef->result.item_idef;
		return 1;
	}
	else
	{
		return 0;
	}
}



/*获取结果项的可变化标志*/
__declspec(noinline) int def_api_rlst_get_chg(int seq)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (pdef == 0)
	{
		return 0;
	}

	return pdef->result.chg_flag;

}






/*获取结果项的可变化指令定义*/
__declspec(noinline) int def_api_rlst_get_sep_idef(int seq)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (pdef == 0)
	{
		return 0;
	}

	return pdef->result.sep_idef;

}



/*获取结果项的可变化指令定义*/
__declspec(noinline) int def_api_rlst_get_expd_idef1(int seq)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (pdef == 0)
	{
		return 0;
	}

	return pdef->result.expd_idef1;

}

/*获取结果项的可变化指令定义*/
__declspec(noinline) int def_api_rlst_get_expd_idef2(int seq)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (pdef == 0)
	{
		return 0;
	}

	return pdef->result.expd_idef2;

}


/*
获取结果项的文本
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
__declspec(noinline) int def_api_rlst_get_sdef(int seq, char *out_sdef, int size)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (size <= strlen(pdef->result.item_sdef))
	{
		return 0;
	}

	memcpy(out_sdef, pdef->result.item_sdef, strlen(pdef->result.item_sdef));
	return 1;
}


/*
获取结果项的大小
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
__declspec(noinline) int def_api_rlst_get_size(int seq, int *out_size)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	*out_size = pdef->result.item_size;
	return 1;
}


/*
获取执行后结果项的sp偏移值
无论结果项类型，都有当前对应的sp偏移
参数1：指令序号
参数2：操作项序号
参数3：返回偏移值
返回：是-1；否-0
*/
__declspec(noinline) int def_api_rlst_get_offset1(int seq, int *out_offset)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

#if 0
	/**/
	if ((pdef->result.item_idef != 4) && (pdef->result.item_idef != 5))
	{
		return 0;
	}
#endif

	*out_offset = pdef->result.offset;
	return 1;
}

/*
获取执行前结果项的sp偏移值
无论结果项类型，都有当前对应的sp偏移
参数1：指令序号
参数2：操作项序号
参数3：返回偏移值
返回：是-1；否-0
*/
__declspec(noinline) int def_api_rlst_get_offset2(int seq, int *out_offset)
{
	struct XX_DEF_INST *pdef = 0;
	int offset = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

#if 0
	if ((pdef->result.item_idef != 4) && (pdef->result.item_idef != 5))
	{
		return 0;
	}
#endif

	*out_offset = pdef->result.offset - (pdef->sp_offset);
	return 1;
}

///////////////////////  操作项  ///////////////////////////////

/*
获取指令操作项个数
参数1：指令序号
参数2：返回操作项个数
返回：成功-1；失败-0
*/
__declspec(noinline) int def_api_get_item_num(int seq,int *out_num)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	*out_num = pdef->item_num;
	return 1;
}


/*
判断操作项是否是指令变量，不使用
参数1：指令序号
参数2：操作项序号
返回：是-1；否-0
*/
__declspec(noinline) int def_api_item_judge_inst(int seq, int item_seq)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num )
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (item_seq >= pdef->item_num)
	{
		return 0;
	}

	if (pdef->item[item_seq].item_inst != 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


/*
获取操作项的指令定义，如果操作项为指令变量
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
__declspec(noinline) int def_api_item_get_inst(int seq, int item_seq,int *out_inst)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (item_seq >= pdef->item_num)
	{
		return 0;
	}

	if (pdef->item[item_seq].item_inst != 0)
	{
		*out_inst = pdef->item[item_seq].item_inst;
		return 1;
	}
	else
	{
		return 0;
	}
}



/*
获取操作项的数字定义
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
__declspec(noinline) int def_api_item_get_idef(int seq, int item_seq, int *out_idef)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (item_seq >= pdef->item_num)
	{
		return 0;
	}

	if (pdef->item[item_seq].item_idef != 0)
	{
		*out_idef = pdef->item[item_seq].item_idef;
		return 1;
	}
	else
	{
		return 0;
	}
}


/*
获取操作项的文本
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
__declspec(noinline) int def_api_item_get_sdef(int seq, int item_seq, char *out_sdef,int size)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (item_seq >= pdef->item_num)
	{
		return 0;
	}

	if (size <= strlen(pdef->item[item_seq].item_sdef))
	{
		return 0;
	}

	memcpy(out_sdef, pdef->item[item_seq].item_sdef, strlen(pdef->item[item_seq].item_sdef));
	return 1;
}



/*
获取操作项的大小
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
__declspec(noinline) int def_api_item_get_size(int seq, int item_seq, int *out_size)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (item_seq >= pdef->item_num)
	{
		return 0;
	}

	*out_size = pdef->item[item_seq].item_size;
	return 1;
}


/*
获取执行后操作项的sp偏移值
无论结果项类型，都有当前对应的sp偏移
参数1：指令序号
参数2：操作项序号
参数3：返回偏移值
返回：是-1；否-0
*/
__declspec(noinline) int def_api_item_get_offset1(int seq, int item_seq, int *out_offset)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (item_seq >= pdef->item_num)
	{
		return 0;
	}

#if 0
	if ((pdef->item[item_seq].item_idef != 4) && (pdef->item[item_seq].item_idef != 5))
	{
		return 0;
	}
#endif
	
	*out_offset = pdef->item[item_seq].offset;
	return 1;
}


/*
获取执行前操作项的sp偏移值
无论结果项类型，都有当前对应的sp偏移
参数1：指令序号
参数2：操作项序号
参数3：返回偏移值
返回：是-1；否-0
*/
__declspec(noinline) int def_api_item_get_offset2(int seq, int item_seq, int *out_offset)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (item_seq >= pdef->item_num)
	{
		return 0;
	}

#if 0
	if ((pdef->item[item_seq].item_idef != 4) && (pdef->item[item_seq].item_idef != 5))
	{
		return 0;
	}
#endif

	*out_offset = pdef->item[item_seq].offset - (pdef->sp_offset);
	return 1;
}



/*
获取指令操作项的可变化标志
参数1：指令序号
参数2：操作项序号
返回：成功-拆分指令定义；失败-0
*/
__declspec(noinline) int def_api_item_get_chg(int seq, int item_seq)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (item_seq >= pdef->item_num)
	{
		return 0;
	}



	return pdef->item[item_seq].chg_flag;
}



/*
获取指令的操作项的变化指令定义
参数1：指令定义
参数2：操作项序号
返回：成功-拆分指令定义；失败-0
*/
__declspec(noinline) int def_api_item_get_sep_idef(int seq, int item_seq)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (item_seq >= pdef->item_num)
	{
		return 0;
	}



	return pdef->item[item_seq].sep_idef;
}


/*
获取指令的操作项的变化指令定义
参数1：指令定义
参数2：操作项序号
返回：成功-扩展指令定义；失败-0
*/
__declspec(noinline) int def_api_item_get_expd_idef1(int seq, int item_seq)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (item_seq >= pdef->item_num)
	{
		return 0;
	}



	return pdef->item[item_seq].expd_idef1;
}


/*
获取指令的操作项的变化指令定义
参数1：指令定义
参数2：操作项序号
返回：成功-扩展指令定义；失败-0
*/
__declspec(noinline) int def_api_item_get_expd_idef2(int seq, int item_seq)
{
	struct XX_DEF_INST *pdef = 0;

	if (def_tbl == 0 || def_num == 0)
	{
		return 0;
	}

	if (seq >= def_num)
	{
		return 0;
	}

	pdef = (struct XX_DEF_INST *)def_tbl[seq];

	if (item_seq >= pdef->item_num)
	{
		return 0;
	}



	return pdef->item[item_seq].expd_idef2;
}
/////////////////////////////  拆分指令操作 ////////////////////////////////////////////////////


/*
设置常量寄存器的索引
*/
__declspec(noinline) void def_api_creg_index_set(int index)
{
	g_creg_index = index;
}


/*
设置常量寄存器的值
*/
__declspec(noinline) int def_api_creg_var_set(int curt_sys, unsigned char *pvar)
{
	if (curt_sys != 4 && curt_sys != 8)
	{
		return 0;
	}

	if (pvar == 0)
	{
		return 0;
	}

	memset(g_creg_var, 0, sizeof(g_creg_var));
	memcpy(g_creg_var, pvar, curt_sys);

	return 1;
}

/*
获取常量寄存器的索引
*/
__declspec(noinline) int def_api_creg_index_get()
{
	return g_creg_index;
}


/*
获取常量寄存器的值
*/
__declspec(noinline) unsigned char* def_api_creg_var_get()
{
	return g_creg_var;
}







