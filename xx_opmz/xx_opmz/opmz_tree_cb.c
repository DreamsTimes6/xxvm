#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xx_comm32.h"
#include "xx_opmz.h"
#include "xx_def_api.h"
#include "zlog.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern zlog_category_t *zc;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int func_cb_exp1(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp2(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp3(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp4(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp5(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp6(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp7(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp8(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp9(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp10(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp11(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp12(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp13(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp14(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp15(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp16(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);
int func_cb_exp17(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp);




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//extern char dbg_log[];
//extern char dbg_file[];

/*
int(*data_callback)(void *pdata1, void *pdata2);
定义节点数据比较回调函数，相等则返回0
data.type  data.offset  data.op_size
*/
__declspec(noinline)  int func_cb_cmp_data(void *pdata1, void *pdata2)
{
	int iret = 0;
	struct ST_OP_DATA *p1 = 0;
	struct ST_OP_DATA *p2 = 0;

	if (pdata1 == 0 || pdata2 == 0)
	{
		return 0;
	}

	p1 = pdata1;
	p2 = pdata2;

#if 0
	if (p1->data.idef == p2->data.idef && \
		p1->data.op_size == p2->data.op_size )
	{
		if (p1->data.idef == 4)
		{
			if (p1->data.offset == p2->data.offset)
			{
				return 1;
			}
		}
		else
		{
			return 1;
		}
	}
#else
	if (p1->data.offset == p2->data.offset && \
		p1->data.op_size == p2->data.op_size && \
		p1->data.idef == p2->data.idef && \
		memcmp(p1->data.var, p2->data.var, sizeof(p1->data.var)) == 0)
	{
		return 1;
	}
#endif

	return 0;
}



//////////////////////////////////////////////////////////////
/*
拆分回调函数
数据2是数据1的子项
*/
__declspec(noinline) int func_cb_cmp_sepdata(void *pdata1, void *pdata2)
{
	int seq = 0;
	int iret = 0;
	int chg_flag = 0;
	struct ST_OP_DATA *p1 = 0;
	struct ST_OP_DATA *p2 = 0;

	if (pdata1 == 0 || pdata2 == 0)
	{
		return 0;
	}

	p1 = pdata1;
	p2 = pdata2;


	/*类型一样*/
	if (p1->data.idef != p2->data.idef)
	{
		return 0;
	}

	/*子项1*/
	if (p1->data.offset == p2->data.offset && \
		p1->data.op_size == (p2->data.op_size * 2) && \
		memcmp(p1->data.var, p2->data.var, sizeof(p1->data.var)) == 0)
	{
		return 1;
	}
	else if (p1->data.offset == (p2->data.offset - p2->data.op_size) && \
		p1->data.op_size == (p2->data.op_size * 2) && \
		memcmp(p1->data.var, p2->data.var, sizeof(p1->data.var)) == 0)
	{
		return 1;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////
/*扩大回调函数*/
__declspec(noinline) int func_cb_cmp_expddata(void *pdata1, void *pdata2)
{
	struct ST_OP_DATA *p1 = 0;
	struct ST_OP_DATA *p2 = 0;

	if (pdata1 == 0 || pdata2 == 0)
	{
		return 0;
	}

	p1 = pdata1;
	p2 = pdata2;


	/*类型一样*/
	if (p1->data.idef != p2->data.idef)
	{
		return 0;
	}

	if (p2->data.offset == p1->data.offset && \
		p2->data.op_size == (p1->data.op_size * 2) && \
		memcmp(p1->data.var, p2->data.var, sizeof(p1->data.var)) == 0)
	{
		return 1;
	}
	else if (p2->data.offset == (p1->data.offset - p1->data.op_size) && \
		p2->data.op_size == (p1->data.op_size * 2) && \
		memcmp(p1->data.var, p2->data.var, sizeof(p1->data.var)) == 0)
	{
		return 1;
	}

	return 0;
}

////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////

/*
int(*print_callback)(void *pdata, void *pext);
完整性检查回调函数
*/
__declspec(noinline) int func_cb_check(void *pdata)
{
	int iret = 0;
	struct ST_OP_DATA *p = 0;

	if (pdata == 0)
	{
		return 0;
	}

	p = pdata;

	if (p->data.idef == 1 || \
		p->data.idef == 2 || \
		p->data.idef == 3 || \
		p->data.idef == 7 || \
		p->data.idef == 8)
	{
		return 1;
	}

	return 0;
}




/*
设置还原指令的起始偏移回调函数
*/
__declspec(noinline) void func_cb_set_start_offset(void *pdata, int idata)
{
	int iret = 0;
	struct ST_OP_DATA *p = 0;

	if (pdata == 0)
	{
		return ;
	}

	p = pdata;


	p->data.start_offset = idata;

	return;
}




/*
设置最低级节点数据的定义
*/
__declspec(noinline) void func_cb_set_idef(void *pdata, int idef)
{
	int iret = 0;
	struct ST_OP_DATA *p = 0;

	if (pdata == 0)
	{
		return;
	}

	p = pdata;


	p->data.idef = idef;

	return;
}






/*
int(*print_callback)(void *pdata, void *pext);
打印回调函数

int func_cb_print(void *pdata, void *pext)
{
	int iret = 0;
	struct ST_OP_DATA *p = 0;

	if (pdata == 0)
	{
		return -1;
	}

	p = pdata;

#ifdef DBG_LOG
	memset(dbg_log, 0, sizeof(0x300));
	sprintf(dbg_log, "node==sdef:[%s] src_idef:[%x] idef:[%d] offset:[%d] op_size:[%d];op:[%x][%s] sub_flag:[%d] seq[%d][%d][%d][%d]\n", \
		p->data.sdef, p->op.src_idef, p->data.idef, p->data.offset, p->data.op_size, p->op.idef, p->op.sdef, p->op.sub_flag, \
		p->op.src_seq[0], p->op.src_seq[1], p->op.src_seq[2], p->op.src_seq[3]);
	iret = xx_append_file(dbg_file, dbg_log, strlen(dbg_log));
	if (iret == 0)
	{
		return 0;
	}

#endif

	return 1;
}
*/

/*定制回调函数，每个节点都会调用这个函数，自定义*/

/*
int(*cust2_callback)(void *pdata, void *pext, void *arg2);
定制回调函数2
*/
int func_cb_cust2(void *pdata, void *pext, void *arg1)
{
	return 1;
};

/*
int(*cust3_callback)(void *pdata, void *pext, void *arg3);
定制回调函数3
*/
int func_cb_cust3(void *pdata, void *pext, void *arg1)
{
	return 1;
};


/*
树表达式回调函数
*/
int func_cb_exp(struct ST_OP_DATA *pdata, char *proot_exp, char **pret_exp)
{
	char *pexp = 0;
	int exp_size = 0;

	if (pdata == 0 || proot_exp == 0 || pret_exp == 0)
	{
		return 0;
	}

	exp_size = strlen(proot_exp) + strlen(pdata->data.sdef) + 0x10;

	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	if (pdata->op.idef == 9)
	{
		/*特殊的情况，root表达式已经完整*/
		sprintf(pexp, "%s", proot_exp);
	}
	else
	{
		sprintf(pexp, "%s=%s", pdata->data.sdef, proot_exp);
	}

	*pret_exp = pexp;

	return exp_size;
}


/*
节点表达式回调函数
*/
int func_cb_node_exp(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;

	if (pdata == 0 || isub == 0 || psub_data == 0 || psub_exp == 0 || pret_exp == 0)
	{
		return 0;
	}

	switch (pdata->op.idef)
	{
	case 1:
		exp_size = func_cb_exp1(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 2:
		exp_size = func_cb_exp2(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 3:
		exp_size = func_cb_exp3(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 4:
		exp_size = func_cb_exp4(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 5:
		exp_size = func_cb_exp5(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 6:
		exp_size = func_cb_exp6(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 7:
		exp_size = func_cb_exp7(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 8:
		exp_size = func_cb_exp8(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 9:
		exp_size = func_cb_exp9(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 10:
		exp_size = func_cb_exp10(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 11:
		exp_size = func_cb_exp11(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 12:
		exp_size = func_cb_exp12(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 13:
		exp_size = func_cb_exp13(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 14:
		exp_size = func_cb_exp14(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 15:
		exp_size = func_cb_exp15(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 16:
		exp_size = func_cb_exp16(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	case 17:
		exp_size = func_cb_exp17(pdata, isub, psub_data, psub_exp, pret_exp);
		break;
	default:
		zlog_error(zc, "pdata->op.idef:[%x]\n", pdata->op.idef);
		return 0;
	}

	return exp_size;
}




/*
mov
*/
int func_cb_exp1(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;

	/*检查参数，子项*/
	if (isub != 1)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	//exp_size = exp_size + strlen(pdata->data.sdef);

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		sprintf(pexp, "%s", psub_exp[0]);
	}
	else
	{
		sprintf(pexp, "%s", psub_data[0]->data.sdef);
	}

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
add
*/
int func_cb_exp2(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 2)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}
	

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}

	if (psub_exp[1] != 0)
	{
		pstr2 = psub_exp[1];
	}
	else
	{
		pstr2 = psub_data[1]->data.sdef;
	}

	sprintf(pexp, "(%s+%s)", pstr1, pstr2);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
not
*/
int func_cb_exp3(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;

	/*检查参数，子项*/
	if (isub != 1)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		sprintf(pexp, "~%s", psub_exp[0]);
	}
	else
	{
		sprintf(pexp, "~%s", psub_data[0]->data.sdef);
	}

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
or
*/
int func_cb_exp4(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 2)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}


	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}

	if (psub_exp[1] != 0)
	{
		pstr2 = psub_exp[1];
	}
	else
	{
		pstr2 = psub_data[1]->data.sdef;
	}

	sprintf(pexp, "(%s|%s)", pstr1, pstr2);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
and
*/
int func_cb_exp5(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 2)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}


	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}

	if (psub_exp[1] != 0)
	{
		pstr2 = psub_exp[1];
	}
	else
	{
		pstr2 = psub_data[1]->data.sdef;
	}

	sprintf(pexp, "(%s&%s)", pstr1, pstr2);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
shr
*/
int func_cb_exp6(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 2)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}


	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}

	if (psub_exp[1] != 0)
	{
		pstr2 = psub_exp[1];
	}
	else
	{
		pstr2 = psub_data[1]->data.sdef;
	}

	sprintf(pexp, "%s>>%s", pstr1, pstr2);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*shl*/
int func_cb_exp7(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 2)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}


	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}

	if (psub_exp[1] != 0)
	{
		pstr2 = psub_exp[1];
	}
	else
	{
		pstr2 = psub_data[1]->data.sdef;
	}

	sprintf(pexp, "%s<<%s", pstr1, pstr2);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
mov*
*/
int func_cb_exp8(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;

	/*检查参数，子项*/
	if (isub != 1)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	//exp_size = exp_size + strlen(pdata->data.sdef);

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}

	if (pdata->data.op_size == 1)
	{
		sprintf(pexp, "b[%s]", pstr1);
	}
	else if (pdata->data.op_size == 2)
	{
		sprintf(pexp, "w[%s]", pstr1);
	}
	else if (pdata->data.op_size == 4)
	{
		sprintf(pexp, "d[%s]", pstr1);
	}
	else if (pdata->data.op_size == 8)
	{
		sprintf(pexp, "q[%s]", pstr1);
	}
	else
	{
		return 0;
	}

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
mov *a,b
*/
int func_cb_exp9(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 2)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}

	if (psub_exp[1] != 0)
	{
		pstr2 = psub_exp[1];
	}
	else
	{
		pstr2 = psub_data[1]->data.sdef;
	}

	if (pdata->data.op_size == 1)
	{
		sprintf(pexp, "b[%s]=%s", pstr1,pstr2);
	}
	else if (pdata->data.op_size == 2)
	{
		sprintf(pexp, "w[%s]=%s", pstr1, pstr2);
	}
	else if (pdata->data.op_size == 4)
	{
		sprintf(pexp, "d[%s]=%s", pstr1, pstr2);
	}
	else if (pdata->data.op_size == 8)
	{
		sprintf(pexp, "q[%s]=%s", pstr1, pstr2);
	}
	else
	{
		return 0;
	}

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
cwd
*/
int func_cb_exp10(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 2)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}

	if (psub_exp[1] != 0)
	{
		pstr2 = psub_exp[1];
	}
	else
	{
		pstr2 = psub_data[1]->data.sdef;
	}

	sprintf(pexp, "cwd(%s,%s)", pstr1, pstr2);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
cdq
*/
int func_cb_exp11(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 2)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}

	if (psub_exp[1] != 0)
	{
		pstr2 = psub_exp[1];
	}
	else
	{
		pstr2 = psub_data[1]->data.sdef;
	}

	sprintf(pexp, "cdq(%s,%s)", pstr1, pstr2);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
shrd
*/
int func_cb_exp12(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;
	char *pstr3 = 0;

	/*检查参数，子项*/
	if (isub != 3)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}

	if (psub_exp[1] != 0)
	{
		pstr2 = psub_exp[1];
	}
	else
	{
		pstr2 = psub_data[1]->data.sdef;
	}

	if (psub_exp[2] != 0)
	{
		pstr3 = psub_exp[2];
	}
	else
	{
		pstr3 = psub_data[2]->data.sdef;
	}

	sprintf(pexp, "shrd(%s-%s,%s)", pstr1, pstr2, pstr3);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
shld
*/
int func_cb_exp13(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;
	char *pstr3 = 0;

	/*检查参数，子项*/
	if (isub != 3)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}

	if (psub_exp[1] != 0)
	{
		pstr2 = psub_exp[1];
	}
	else
	{
		pstr2 = psub_data[1]->data.sdef;
	}

	if (psub_exp[2] != 0)
	{
		pstr3 = psub_exp[2];
	}
	else
	{
		pstr3 = psub_data[2]->data.sdef;
	}

	sprintf(pexp, "shrd(%s-%s,%s)", pstr1, pstr2, pstr3);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
cdw1
*/
int func_cb_exp14(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 1)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}


	sprintf(pexp, "cdw1(%s)", pstr1);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
cqd1
*/
int func_cb_exp15(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 1)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}


	sprintf(pexp, "cqd1(%s)", pstr1);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
cdw2
*/
int func_cb_exp16(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 1)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}


	sprintf(pexp, "cdw2(%s)", pstr1);

	*pret_exp = pexp;

	return strlen(pexp);
}

/*
cqd2
*/
int func_cb_exp17(struct ST_OP_DATA *pdata, int isub, struct ST_OP_DATA **psub_data, char **psub_exp, char **pret_exp)
{
	int iret = 0;
	int n = 0;
	int exp_size = 0;
	char *pexp = 0;
	char *pstr1 = 0;
	char *pstr2 = 0;

	/*检查参数，子项*/
	if (isub != 1)
	{
		return 0;
	}

	/*计算空间大小*/
	exp_size = 0;

	for (n = 0; n < isub; n++)
	{
		if (psub_exp[n] != 0)
		{
			exp_size = strlen(psub_exp[n]) + exp_size;
		}
		else
		{
			exp_size = strlen(psub_data[n]->data.sdef) + exp_size;
		}
	}

	if (exp_size == 0)
	{
		return 0;
	}

	/*操作符空间大小*/
	exp_size = exp_size + 10;

	/*申请表达式空间，该控件回调函数内不释放，由树自己释放*/
	pexp = malloc(exp_size);
	if (pexp == 0)
	{
		return 0;
	}
	memset(pexp, 0, exp_size);

	/*符号和变量的位置*/
	if (psub_exp[0] != 0)
	{
		pstr1 = psub_exp[0];
	}
	else
	{
		pstr1 = psub_data[0]->data.sdef;
	}


	sprintf(pexp, "cqd1(%s)", pstr1);

	*pret_exp = pexp;

	return strlen(pexp);
}


#if 0
void func_cb_exp_free(void *pdata, void *pext)
{
	int iret = 0;
	struct ST_OP_DATA *p = 0;

	if (pdata == 0)
	{
		return;
	}

	p = pdata;

	if (p->data.exp)
	{
		free(p->data.exp);
		p->data.exp = 0;
	}

	return;
}

char *func_cb_exp_get(void *pdata, int *size)
{
	int iret = 0;
	struct ST_OP_DATA *p = 0;
	char *ptmp = 0;
	int tmp_size = 0;

	if (pdata == 0)
	{
		return 0;
	}

	p = pdata;

	if (p->data.exp == 0 || p->data.exp_size == 0)
	{
		return 0;
	}

	/*说明到达终点*/

	/*运算符号空间，防止溢出*/
	tmp_size = 0x10;
	/*检查根节点是否生成，检查“=”*/
	if (strstr(p->data.exp, "=") == 0)
	{
		tmp_size = p->data.exp_size + sizeof(p->data.sdef);
		ptmp = malloc(tmp_size);
		if (ptmp == 0)
		{
			return 0;
		}
		memset(ptmp, 0, tmp_size);
		sprintf(ptmp, "%s=%s", p->data.sdef, p->data.exp);
	}
	else
	{
		tmp_size = p->data.exp_size;
		ptmp = malloc(tmp_size);
		if (ptmp == 0)
		{
			return 0;
		}
		memset(ptmp, 0, tmp_size);
		sprintf(ptmp, "%s", p->data.exp);
	}

	*size = tmp_size;
	return ptmp;
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////




















