#include "stdio.h"
#include <windows.h>
#include <shlwapi.h>
//#include "Plugin.h"
#include "xx_vm.h"
#include "xx_comm64.h"
#include "xxdisasm64.h"
#include "xx_opdef2.h"



extern int xx_read_fmem(unsigned long long addr, char* pdata, unsigned int datasize);
extern int xx_write_fmem(unsigned long long addr, char* pdata, unsigned int datasize);
extern int func_inst_prefix(struct XX_INST *pinst, unsigned char pc);

extern char debug_buf[500];
extern int debug_log(char *tmp);

extern void *pfunc_v;
extern int xx_get_fmem(unsigned long long addr, char *outname, unsigned long long *out_offset);


extern int asm_movsb(unsigned long long v_rcx, unsigned long long v_rsi, unsigned long long v_rdi,void *pv_inst);
///////////////////////////////////////////////////////////////////////////




__declspec(noinline) int func_op_movsb_ext(struct XX_INST *xx_inst, struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	int n = 0;
	unsigned char *pmem = 0;
	int mem_size = 0;
	char tmp1[500];
	char tmp2[500];
	unsigned long long v_addr = 0;
	unsigned long long v_rsi = 0;
	unsigned long long v_rdi = 0;
	unsigned char c_ret[] = { 0xc3 };
	unsigned long long v_rcx = 0;

	memset(tmp1, 0, sizeof(tmp1));
	iret = xx_get_fmem(in_context->r[REG_RSI], tmp1, &v_rsi);
	if (iret == 0)
	{
		return 0;
	}

	memset(tmp2, 0, sizeof(tmp2));
	iret = xx_get_fmem(in_context->r[REG_RDI], tmp2, &v_rdi);
	if (iret == 0)
	{
		return 0;
	}

	if (strcmp(tmp1, tmp2) != 0)
	{
		return 0;
	}

	mem_size = xx_get_file_size(tmp1);
	if (mem_size == 0)
	{
		return 0;
	}

	/*申请存储空间*/
	pmem = malloc(mem_size);
	if (pmem == 0)
	{
		return 0;
	}
	memset(pmem, 0, mem_size);

	iret = xx_read_file(tmp1, pmem, 0, mem_size);
	if (iret == 0)
	{
		goto l_err;
	}

	v_rsi = pmem + v_rsi;
	v_rdi = pmem + v_rdi;

	if (pfunc_v == 0)
	{
		goto l_err;
	}

	memcpy(pfunc_v, xx_inst->xx_inst_code.disasm, xx_inst->xx_inst_code.disasm_length);
	memcpy((unsigned char*)pfunc_v + xx_inst->xx_inst_code.disasm_length, c_ret, sizeof(c_ret));


	v_rcx = in_context->r[REG_RCX];
	asm_movsb(v_rcx, v_rsi, v_rdi, pfunc_v);

	iret = xx_cover_file(tmp1, pmem, mem_size);
	if (iret == 0)
	{
		goto l_err;
	}

	free(pmem);

	return 1;
l_err:

	free(pmem);

	return 0;
}

__declspec(noinline) int func_op_movsb(struct XX_INST *xx_inst, struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	int n = 0;
	unsigned char *pvar = 0;
	unsigned char c_f3 = 0xf3;
	int var_size = 0;
	unsigned char tmp[0x10];

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	iret = func_inst_prefix(xx_inst, c_f3);
	if (iret == 0)
	{
		memset(tmp, 0, sizeof(tmp));
		pvar = tmp;
		var_size = var[0].size;

		/*取数据*/
		iret = xx_read_fmem(out_context->r[REG_RSI], (unsigned char*)pvar, var_size);
		if (iret == 0)
		{
			return 0;
		}

		iret = xx_write_fmem(out_context->r[REG_RDI], (unsigned char*)pvar, var_size);
		if (iret == 0)
		{
			return 0;
		}

		if (out_context->eflag & 0x400)
		{
			out_context->r[REG_RSI] = out_context->r[REG_RSI] - var_size;
			out_context->r[REG_RDI] = out_context->r[REG_RDI] - var_size;
		}
		else
		{
			out_context->r[REG_RSI] = out_context->r[REG_RSI] + var_size;
			out_context->r[REG_RDI] = out_context->r[REG_RDI] + var_size;
		}
	}
	else
	{
		var_size = out_context->r[REG_RCX] * var[0].size;

		if (var_size != 0)
		{
			pvar = malloc(var_size);
			if (pvar == 0)
			{
				return 0;
			}
			memset(pvar, 0, var_size);

			/*如果esi和edi相差不足var_size，则单步执行*/
			if (out_context->eflag & 0x400)
			{
				if (out_context->r[REG_RSI] >= out_context->r[REG_RDI] && (out_context->r[REG_RSI] - out_context->r[REG_RDI]) <= (unsigned long)var_size)
				{
					out_context->r[REG_RSI] = out_context->r[REG_RSI] - var_size;
					out_context->r[REG_RDI] = out_context->r[REG_RDI] - var_size;
					iret = func_op_movsb_ext(xx_inst, var, in_context, out_context);
					if (iret == 0)
					{
						return 0;
					}
				}
				else
				{
					out_context->r[REG_RSI] = out_context->r[REG_RSI] - var_size;
					out_context->r[REG_RDI] = out_context->r[REG_RDI] - var_size;
					/*取数据*/
					iret = xx_read_fmem(out_context->r[REG_RSI] + var[0].size, (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}

					iret = xx_write_fmem(out_context->r[REG_RDI] + var[0].size, (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}
				}
			}
			else
			{
				if (out_context->r[REG_RSI] <= out_context->r[REG_RDI] && (out_context->r[REG_RDI] - out_context->r[REG_RSI]) <= (unsigned long)var_size)
				{

					iret = func_op_movsb_ext(xx_inst, var, in_context, out_context);
					if (iret == 0)
					{
						return 0;
					}
					out_context->r[REG_RSI] = out_context->r[REG_RSI] + var_size;
					out_context->r[REG_RDI] = out_context->r[REG_RDI] + var_size;
				}
				else
				{
					/*取数据*/
					iret = xx_read_fmem(out_context->r[REG_RSI], (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}

					iret = xx_write_fmem(out_context->r[REG_RDI], (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}

					out_context->r[REG_RSI] = out_context->r[REG_RSI] + var_size;
					out_context->r[REG_RDI] = out_context->r[REG_RDI] + var_size;
				}
			}

			free(pvar);

			out_context->r[REG_RCX] = 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


