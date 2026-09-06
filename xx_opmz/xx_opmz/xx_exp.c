#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Windows.h>
#include "xx_comm32.h"
#include "xx_exp_data.h"
#include "zlog.h"


#ifdef _WIN64
#define EXP_SYS_FLAG 2

#else
#define EXP_SYS_FLAG 1

#endif


//////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////
int xx_exp_rule(struct XX_EXP *pexp_tbl, int exp_num, struct XX_EXP *pxx_exp, int lv);
int xx_exp_gen(struct XX_EXP *pxx_exp, char *pexp);
int xx_exp_rule_calc_var(char *var1, char *var2, int calc_type);
int xx_str_replace(char *psrc, int src_size, char *preplace, char *pdes, char *pout);

int shex_hex(char* pstr_hex, char *phex);
//////////////////////////////////////////////////////////////////////////////
extern zlog_category_t *zc;
///////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////
/*
输入一个原始表达式，生成一条汇编指令
参数1：原始表达式
参数2：返回汇编指令
返回：1-成功；0-失败
*/
__declspec(noinline) int xx_exp_to_asm(char *pexp, struct XX_EXP * out_exp, int lv)
{
	int iret = 0;
	int exp_num = 0;
	struct XX_EXP *xx_exp_tbl = 0;
	struct XX_EXP xx_exp;
	int curt_len = 0;

	/*参数检查*/
	if (pexp == 0 || out_exp == 0)
	{
		zlog_error(zc, "pexp:[%s] out_exp:[%x] lv:[%d]\n", pexp, out_exp, lv);
		return 0;
	}

	curt_len = strlen(pexp);

	/*检查表达式的大小，是否超出表达式结构定义的大小*/
	if (curt_len >= DEF_EXP_SIZE)
	{
		zlog_error(zc, "curt_len:[%d]\n", curt_len);
		return 0;
	}


	/*获取还原信息数据，初始化在还原开始进行*/
	xx_exp_tbl = 0;
	xx_exp_tbl = xx_exp_data_get();
	if (xx_exp_tbl == 0)
	{
		zlog_error(zc, "xx_exp_tbl:[%d]\n", xx_exp_tbl);
		return 0;
	}
	exp_num = 0;
	exp_num = xx_exp_data_num();
	if (exp_num == 0)
	{
		zlog_error(zc, "exp_num:[%d]\n", exp_num);
		return 0;
	}

	/*生成表达式结构*/
	memset(&xx_exp, 0, sizeof(xx_exp));
	iret = xx_exp_gen(&xx_exp, pexp);
	if (iret == 0)
	{
		zlog_error(zc, "xx_exp_gen\n");
		return 0;
	}

	//zlog_info(zc, "xx_exp_gen :%s\n", xx_exp.pcomm_exp);

	/*调用规则生成汇编指令，返回参数赋值*/
	iret = xx_exp_rule(xx_exp_tbl, exp_num, &xx_exp, lv);
	if (iret == 0)
	{
		zlog_error(zc, "xx_exp_rule [%s]\n", xx_exp.pcomm_exp);
		return 0;
	}


	memcpy(out_exp, &xx_exp, sizeof(xx_exp));


	return 1;
}


/*
生成表达式结构，填充表达式结构
参数1：表达式结构
参数2：原始表达式
返回：成功-1；失败-0
*/
__declspec(noinline) int xx_exp_gen(struct XX_EXP *pxx_exp, char *pexp)
{
	int iret = 0;
	char *pvar = 0;
	char *pend = 0;
	char ptmp[DEF_EXP_SIZE];
	char pbuf[DEF_EXP_SIZE];
	int exp_size = 0;
	unsigned char tmp[100];


	exp_size = strlen(pexp);
	/*参数检测*/
	if (pxx_exp == 0 || pexp == 0 || exp_size == 0)
	{
		return 0;
	}

	memset(pbuf, 0, sizeof(pbuf));
	memcpy(pbuf, pexp, exp_size);
	/*扫描变量*/
	while (1)
	{
		pvar = 0;
		pvar = strstr(pbuf, "<0x");
		if (pvar == 0)
		{
			break;
		}

		/*添加到结构*/
		pend = 0;
		pend = strstr(pvar, ">");
		if (pend == 0)
		{
			return 0;
		}
		memcpy(pxx_exp->exp_var[pxx_exp->var_num].svar, pvar, pend - pvar + 1);

		pxx_exp->exp_var[pxx_exp->var_num].idef = 2;

		sprintf(pxx_exp->exp_var[pxx_exp->var_num].sname, "const%d", pxx_exp->var_num);

		/*变量值*/
		memset(tmp, 0, sizeof(tmp));
		memcpy(tmp, pvar + 3, pend - (pvar + 3));
		iret = shex_hex(tmp, pxx_exp->exp_var[pxx_exp->var_num].ivar);
		if (iret == 0)
		{
			zlog_error(zc, "shex_hex error [%s]\n", pxx_exp->exp_var[pxx_exp->var_num].svar);
			return 0;
		}

		/*替换变量*/
		memset(ptmp, 0, sizeof(ptmp));
		iret = xx_str_replace(pbuf, sizeof(pbuf), pxx_exp->exp_var[pxx_exp->var_num].svar, \
			pxx_exp->exp_var[pxx_exp->var_num].sname, ptmp);
		if (iret == 0)
		{
			return 0;
		}

		pxx_exp->var_num++;

		memset(pbuf, 0, sizeof(pbuf));
		memcpy(pbuf, ptmp, sizeof(pbuf));

		if (pxx_exp->var_num >= (sizeof(pxx_exp->exp_var) / sizeof(struct XX_EXP_VAR)))
		{
			/*变量太多，说明有错误*/
			return 0;
		}
	}

	while (1)
	{
		pvar = 0;
		pvar = strstr(pbuf, "reg[");
		if (pvar == 0)
		{
			break;
		}

		pvar = pvar - 2;

		if (*pvar != '<')
		{
			return 0;
		}

		/*添加到结构*/
		pend = 0;
		pend = strstr(pvar, ">");
		if (pend == 0)
		{
			return 0;
		}
		memcpy(pxx_exp->exp_var[pxx_exp->var_num].svar, pvar, pend - pvar + 1);

		pxx_exp->exp_var[pxx_exp->var_num].idef = 1;

		sprintf(pxx_exp->exp_var[pxx_exp->var_num].sname, "reg%d", pxx_exp->var_num);

		/*替换变量*/
		memset(tmp, 0, sizeof(tmp));
		memcpy(tmp, pvar + 6, pend - 1 - (pvar + 6));
		iret = shex_hex(tmp, pxx_exp->exp_var[pxx_exp->var_num].ivar);
		if (iret == 0)
		{
			zlog_error(zc, "shex_hex error [%s]\n", pxx_exp->exp_var[pxx_exp->var_num].svar);
			return 0;
		}

		/*替换变量*/
		memset(ptmp, 0, sizeof(ptmp));
		iret = xx_str_replace(pbuf, sizeof(pbuf), pxx_exp->exp_var[pxx_exp->var_num].svar, \
			pxx_exp->exp_var[pxx_exp->var_num].sname, ptmp);
		if (iret == 0)
		{
			return 0;
		}

		pxx_exp->var_num++;

		memset(pbuf, 0, sizeof(pbuf));
		memcpy(pbuf, ptmp, sizeof(pbuf));

		if (pxx_exp->var_num >= (sizeof(pxx_exp->exp_var) / sizeof(struct XX_EXP_VAR)))
		{
			/*变量太多，说明有错误*/
			return 0;
		}
	}

	memset(pxx_exp->pcomm_exp, 0, sizeof(pxx_exp->pcomm_exp));
	memcpy(pxx_exp->pcomm_exp, pbuf, strlen(pbuf));

	

	return 1;
}



/*
匹配表达式定义，返回汇编指令
参数1：表达式数据表
参数2：表达式个数
参数3：待匹配的表达式
返回：成功-1；失败-0
*/
int xx_exp_rule_func(struct XX_EXP *pxx_exp1, struct XX_EXP *pxx_exp2);

__declspec(noinline) int xx_exp_rule(struct XX_EXP *pexp_tbl, int exp_num, struct XX_EXP *pxx_exp, int lv)
{
	int n = 0;
	int m = 0;
	int iret = 0;
	char tmp1[DEF_EXP_SIZE];
	char tmp2[DEF_EXP_SIZE];
	unsigned long lv_var = 0;


	/*参数检查*/
	if (pexp_tbl == 0 || exp_num == 0 || pxx_exp == 0)
	{
		return 0;
	}

	/*等级值修复*/
	lv_var = lv / OPMZ_LV_FIX;


	/*匹配*/
	for (n = 0; n < exp_num; n++)
	{
		/*检查优化等级标志*/
		if (((pexp_tbl[n].lv_flag >> lv_var) & 0x01) != 0x01)
		{
			continue;
		}

#if 0
		/*忽略表达式的系统位数*/
		if ((pexp_tbl[n].sys_flag&EXP_SYS_FLAG) != EXP_SYS_FLAG)
		{
			continue;
		}
#endif

		if (strcmp(pexp_tbl[n].pcomm_exp, pxx_exp->pcomm_exp) == 0 && \
			pexp_tbl[n].var_num == pxx_exp->var_num)
		{
			if (xx_exp_rule_func(&pexp_tbl[n], pxx_exp) == 1)
			{
				/*变量回写*/
				memset(tmp1, 0, sizeof(tmp1));
				memcpy(tmp1, pexp_tbl[n].pasm_inst, strlen(pexp_tbl[n].pasm_inst));
				for (m = 0; m < pxx_exp->var_num; m++)
				{
					memset(tmp2, 0, sizeof(tmp2));
					iret = xx_str_replace(tmp1, sizeof(tmp1), pxx_exp->exp_var[m].sname, \
						pxx_exp->exp_var[m].svar, tmp2);
					if (iret == 0)
					{
						return 0;
					}
					memset(tmp1, 0, sizeof(tmp1));
					memcpy(tmp1, tmp2, strlen(tmp2));
				}

				//memcpy(pxx_exp, &pexp_tbl[n], sizeof(struct XX_EXP));
				//不完全覆盖，保留原结构信息
				memset(pxx_exp->pasm_inst, 0, sizeof(pxx_exp->pasm_inst));
				memcpy(pxx_exp->pasm_inst, tmp1, strlen(tmp1));

				pxx_exp->idef = pexp_tbl[n].idef;

				pxx_exp->mark_flag = pexp_tbl[n].mark_flag;
				pxx_exp->mark_place = pexp_tbl[n].mark_place;

				//pxx_exp->restart_flag = pexp_tbl[n].restart_flag;
				return 1;
			}
		}
	}

	return 0;
}


/*
参数1：tbl
参数2：生成的结构
*/
__declspec(noinline) int xx_exp_rule_func(struct XX_EXP *pxx_exp1, struct XX_EXP *pxx_exp2)
{
	int m = 0;

	for (m = 0; m < pxx_exp1->var_num; m++)
	{
		if (pxx_exp1->exp_var[m].calc_var != -1)
		{
			/*检查变量类型是否是立即数，不是立即数，不处理，可能是配置错误*/
			if (pxx_exp2->exp_var[m].idef == 2 && pxx_exp2->exp_var[pxx_exp1->exp_var[m].calc_var].idef == 2)
			{
				/*检查变量转换是否符合*/
				if (xx_exp_rule_calc_var(pxx_exp2->exp_var[m].svar, \
					pxx_exp2->exp_var[pxx_exp1->exp_var[m].calc_var].svar, pxx_exp1->exp_var[m].calc_type) == 0)
				{
					return 0;
				}
			}
		}
		if (pxx_exp1->exp_var[m].idef != pxx_exp2->exp_var[m].idef)
		{
			return 0;
		}
		if (pxx_exp1->exp_var[m].flag == 1)
		{
			if (strcmp(pxx_exp1->exp_var[m].svar, pxx_exp2->exp_var[m].svar) != 0)
			{
				return 0;
			}
		}
	}

	return 1;
}

/*
参数1：目的变量
参数2：运算变量
参数3：运算类型
返回：符合-1；不符合-0
*/
__declspec(noinline) int xx_exp_rule_calc_var(char *var1, char *var2, int calc_type)
{
	int n = 0;
	int iret = 0;
	int len1 = 0;
	int len2 = 0;
	int max_len = 0;
	char tmp1[100];
	char tmp2[100];
	char buf1[100];
	char buf2[100];
	char *ptmp = 0;
	/*检查参数*/
	if (var1 == 0 || var2 == 0 || calc_type == 0)
	{
		return 0;
	}

	if (strlen(var1) == 0 || strlen(var2) == 0)
	{
		return 0;
	}

	/*去掉变量1的辅助符合<0x>*/
	ptmp = 0;
	ptmp = strstr(var1, "<0x");
	if (ptmp == 0)
	{
		return 0;
	}
	memset(tmp1, 0, sizeof(tmp1));
	memcpy(tmp1, ptmp + strlen("<0x"), strlen(ptmp) - strlen("<0x"));
	ptmp = 0;
	ptmp = strstr(tmp1, ">");
	if (ptmp == 0)
	{
		return 0;
	}
	*ptmp = (char)0x0;

	/*去掉变量2的辅助符合<0x>*/
	ptmp = 0;
	ptmp = strstr(var2, "<0x");
	if (ptmp == 0)
	{
		return 0;
	}
	memset(tmp2, 0, sizeof(tmp2));
	memcpy(tmp2, ptmp + strlen("<0x"), strlen(ptmp) - strlen("<0x"));
	ptmp = 0;
	ptmp = strstr(tmp2, ">");
	if (ptmp == 0)
	{
		return 0;
	}
	*ptmp = (char)0x0;



	len1 = strlen(tmp1);
	len2 = strlen(tmp2);

	/*转换前修复长度*/
	if (len1 % 2)
	{
		for (n = len1; n > 0; n--)
		{
			tmp1[n] = tmp1[n - 1];
		}
		tmp1[0] = '0';
		len1 = len1 + 1;
	}
	if (len2 % 2)
	{
		for (n = len2; n > 0; n--)
		{
			tmp2[n] = tmp2[n - 1];
		}
		tmp2[0] = '0';
		len2 = len2 + 1;
	}

	/*变量，按各自长度字符转16进制数据*/
	memset(buf1, 0, sizeof(buf1));
	iret = nstr_hex(tmp1, len1, buf1);
	if (iret == -1)
	{
		return 0;
	}
	/*反转字节*/
	len1 = len1 / 2;
	memset(tmp1, 0, sizeof(tmp1));
	iret = byte_opposite(buf1, len1, tmp1);
	if (iret == -1)
	{
		return 0;
	}

	memset(buf2, 0, sizeof(buf2));
	iret = nstr_hex(tmp2, len2, buf2);
	if (iret == -1)
	{
		return 0;
	}

	/*反转字节*/
	len2 = len2 / 2;
	memset(tmp2, 0, sizeof(tmp2));
	iret = byte_opposite(buf2, len2, tmp2);
	if (iret == -1)
	{
		return 0;
	}

	/*获取两个变量的最长长度*/
	if (len1 < len2)
	{
		max_len = len2;
	}
	else
	{
		max_len = len1;
	}


	switch (calc_type)
	{
	case 3:
		/*将运算变量按最大长度字节单位取反*/
		for (n = 0; n < max_len; n++)
		{
			tmp2[n] = ~tmp2[n];
		}
		break;
	default:
		return 0;
	}


	/*比较是否一样*/
	if (memcmp(tmp1, tmp2, max_len) != 0)
	{
		return 0;
	}

	/*一样返回1*/
	return 1;
}









/*
替换字符
参数1：原字符串
参数2：原字符串大小
参数3：待替换的字符串
参数4：目标字符串
参数5：输出字符串
返回：1-成功；0-失败
*/
__declspec(noinline) int xx_str_replace(char *psrc, int src_size, char *preplace, char *pdes, char *pout)
{
	int iret = 0;
	int src_len = 0;
	int rep_len = 0;
	int des_len = 0;
	char *p = 0;
	char *pbuf1 = 0;
	char *pbuf2 = 0;

	if (psrc == 0 || src_size == 0 || preplace == 0 || pdes == 0)
	{
		return 0;
	}

	src_len = strlen(psrc);
	rep_len = strlen(preplace);
	des_len = strlen(pdes);
	if (src_len == 0 || rep_len == 0 || des_len == 0)
	{
		return 0;
	}


	pbuf1 = malloc(src_size);
	if (pbuf1 == 0)
	{
		return 0;
	}
	memset(pbuf1, 0, src_size);

	pbuf2 = malloc(src_size);
	if (pbuf2 == 0)
	{
		return 0;
	}
	memset(pbuf2, 0, src_size);

	memcpy(pbuf1, psrc, src_len);


	while (1)
	{
		p = strstr(pbuf1, preplace);
		if (p == 0)
		{
			break;
		}
		*p = 0x0;
		strcat(pbuf2, pbuf1);
		strcat(pbuf2, pdes);
		strcat(pbuf2, p + rep_len);

		memset(pbuf1, 0, src_size);
		memcpy(pbuf1, pbuf2, src_size);
		memset(pbuf2, 0, src_size);
	}

	memcpy(pout, pbuf1, src_size);

	free(pbuf1);
	free(pbuf2);

	return 1;
}



/*
32位十六进制字符串转换为十六进制数据
*/

__declspec(noinline) int shex_hex(char* pstr_hex, char *phex)
{
	char tmp[100];
	int iret = 0;
	int str_len = 0;
	char str_tmp[100];

	/*参数检查*/
	if (pstr_hex == 0 || phex == 0)
	{
		return 0;
	}

	str_len = strlen(pstr_hex);
	if (str_len == 0 || str_len > 16)
	{
		return 0;
	}

	memset(str_tmp, 0x30, sizeof(str_tmp));

	if (str_len % 2)
	{
		memcpy(str_tmp + 1, pstr_hex, str_len);
		str_len = str_len + 1;
	}
	else
	{
		memcpy(str_tmp, pstr_hex, str_len);
	}

	memset(tmp, 0, sizeof(tmp));
	iret = nstr_hex(str_tmp, str_len, tmp);
	if (iret)
	{
		return 0;
	}

	iret = byte_opposite(tmp, str_len / 2, phex);
	if (iret)
	{
		return 0;
	}


	return 1;
}



























































