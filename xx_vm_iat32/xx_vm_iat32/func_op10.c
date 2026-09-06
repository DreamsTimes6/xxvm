#include "stdio.h"
#include <windows.h>
#include <shlwapi.h>
//#include "Plugin.h"
#include "xx_iat.h"
#include "xx_comm32.h"
#include "xxdisasm32.h"
#include "xx_opdef2.h"



extern int xx_read_fmem(unsigned long addr, char* pdata, unsigned int datasize);
extern int xx_write_fmem(unsigned long addr, char* pdata, unsigned int datasize);
extern int calc_item(struct XX_INST *inst, int nitem, struct ITEM_VAR *out_var, struct XX_CONTEXT *out_context, unsigned  long result, int sign_flag);
extern void item_sign_ext(unsigned long size1, unsigned long size2, unsigned char *pvar2);
extern int func_inst_prefix(struct XX_INST *pinst, unsigned char pc);

extern char debug_buf[500];
extern int debug_log(char *tmp);

extern void *pfunc_v;
extern int xx_get_fmem(unsigned  long addr, char *outname, unsigned  long *out_offset);
///////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////

__declspec(noinline) int func_op_lodsb(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;
	unsigned long n = 0;
	unsigned char c_f3 = 0xf3;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	iret = func_inst_prefix(xx_inst, c_f3);
	if (iret == 0)
	{
		/*取数据*/
		iret = xx_read_fmem(out_context->r[REG_ESI], (unsigned char*)&var1, var[0].size);
		if (iret == 0)
		{
			return 0;
		}

		memcpy(&out_context->r[REG_EAX], &var1, var[0].size);

		if (out_context->eflag & 0x400)
		{
			out_context->r[REG_ESI] = out_context->r[REG_ESI] - var[0].size;
		}
		else
		{
			out_context->r[REG_ESI] = out_context->r[REG_ESI] + var[0].size;
		}
	}
	else
	{
		n = out_context->r[REG_ECX];
		if (n != 0)
		{

			if (out_context->eflag & 0x400)
			{
				out_context->r[REG_ESI] = out_context->r[REG_ESI] - (n*var[0].size);
				/*取数据*/
				iret = xx_read_fmem(out_context->r[REG_ESI] + var[0].size, (unsigned char*)&var1, var[0].size);
				if (iret == 0)
				{
					return 0;
				}
			}
			else
			{
				out_context->r[REG_ESI] = out_context->r[REG_ESI] + (n*var[0].size);
				/*取数据*/
				iret = xx_read_fmem(out_context->r[REG_ESI] - var[0].size, (unsigned char*)&var1, var[0].size);
				if (iret == 0)
				{
					return 0;
				}
			}

			memcpy(&out_context->r[REG_EAX], &var1, var[0].size);

			out_context->r[REG_ECX] = 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_lods(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;
	unsigned long n = 0;
	unsigned char c_f3 = 0xf3;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	iret = func_inst_prefix(xx_inst, c_f3);
	if (iret == 0)
	{
		/*取数据*/
		iret = xx_read_fmem(out_context->r[REG_ESI], (unsigned char*)&var1, var[0].size);
		if (iret == 0)
		{
			return 0;
		}

		memcpy(&out_context->r[REG_EAX], &var1, var[0].size);

		if (out_context->eflag & 0x400)
		{
			out_context->r[REG_ESI] = out_context->r[REG_ESI] - var[0].size;
		}
		else
		{
			out_context->r[REG_ESI] = out_context->r[REG_ESI] + var[0].size;
		}
	}
	else
	{
		n = out_context->r[REG_ECX];
		if (n != 0)
		{

			if (out_context->eflag & 0x400)
			{
				out_context->r[REG_ESI] = out_context->r[REG_ESI] - (n*var[0].size);
				/*取数据*/
				iret = xx_read_fmem(out_context->r[REG_ESI] + var[0].size, (unsigned char*)&var1, var[0].size);
				if (iret == 0)
				{
					return 0;
				}
			}
			else
			{
				out_context->r[REG_ESI] = out_context->r[REG_ESI] + (n*var[0].size);
				/*取数据*/
				iret = xx_read_fmem(out_context->r[REG_ESI] - var[0].size, (unsigned char*)&var1, var[0].size);
				if (iret == 0)
				{
					return 0;
				}
			}

			memcpy(&out_context->r[REG_EAX], &var1, var[0].size);

			out_context->r[REG_ECX] = 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_movsb_ext(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int n = 0;
	unsigned char *pmem = 0;
	int mem_size = 0;
	char tmp1[500];
	char tmp2[500];
	unsigned long v_addr = 0;
	unsigned long v_esi = 0;
	unsigned long v_edi = 0;
	unsigned char c_ret[] = { 0xc3 };
	unsigned long v_ecx = 0;

	memset(tmp1, 0, sizeof(tmp1));
	iret = xx_get_fmem(in_context->r[REG_ESI], tmp1, &v_esi);
	if (iret == 0)
	{
		return 0;
	}

	memset(tmp2, 0, sizeof(tmp2));
	iret = xx_get_fmem(in_context->r[REG_EDI], tmp2, &v_edi);
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

	v_esi = pmem + v_esi;
	v_edi = pmem + v_edi;
	
	if (pfunc_v == 0)
	{
		goto l_err;
	}

	memcpy(pfunc_v, xx_inst->xx_inst_code.disasm, xx_inst->xx_inst_code.disasm_length);
	memcpy((unsigned char*)pfunc_v + xx_inst->xx_inst_code.disasm_length, c_ret, sizeof(c_ret));


	v_ecx = in_context->r[REG_ECX];
	__asm
	{
		pushad
		mov esi,v_esi
		mov edi,v_edi
		mov ecx, v_ecx
		mov eax, pfunc_v
		call eax
		mov v_esi,esi
		mov v_edi,edi
		popad
	};

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

__declspec(noinline) int func_op_movsb(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int n = 0;
	unsigned char *pvar = 0;
	unsigned char c_f3 = 0xf3;
	int var_size = 0;
	unsigned char tmp[0x10];

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	iret = func_inst_prefix(xx_inst, c_f3);
	if (iret == 0)
	{
		memset(tmp, 0, sizeof(tmp));
		pvar = tmp;
		var_size = var[0].size;

		/*取数据*/
		iret = xx_read_fmem(out_context->r[REG_ESI], (unsigned char*)pvar, var_size);
		if (iret == 0)
		{
			return 0;
		}

		iret = xx_write_fmem(out_context->r[REG_EDI], (unsigned char*)pvar, var_size);
		if (iret == 0)
		{
			return 0;
		}

		if (out_context->eflag & 0x400)
		{
			out_context->r[REG_ESI] = out_context->r[REG_ESI] - var_size;
			out_context->r[REG_EDI] = out_context->r[REG_EDI] - var_size;
		}
		else
		{
			out_context->r[REG_ESI] = out_context->r[REG_ESI] + var_size;
			out_context->r[REG_EDI] = out_context->r[REG_EDI] + var_size;
		}
	}
	else
	{
		var_size = out_context->r[REG_ECX] * var[0].size;

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
				if (out_context->r[REG_ESI] >= out_context->r[REG_EDI] && (out_context->r[REG_ESI] - out_context->r[REG_EDI]) <= (unsigned long)var_size)
				{
					out_context->r[REG_ESI] = out_context->r[REG_ESI] - var_size;
					out_context->r[REG_EDI] = out_context->r[REG_EDI] - var_size;
					iret = func_op_movsb_ext(xx_inst, var, in_context, out_context);
					if (iret == 0)
					{
						return 0;
					}
				}
				else
				{
					out_context->r[REG_ESI] = out_context->r[REG_ESI] - var_size;
					out_context->r[REG_EDI] = out_context->r[REG_EDI] - var_size;
					/*取数据*/
					iret = xx_read_fmem(out_context->r[REG_ESI] + var[0].size, (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}

					iret = xx_write_fmem(out_context->r[REG_EDI] + var[0].size, (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}
				}
			}
			else
			{
				if (out_context->r[REG_ESI] <= out_context->r[REG_EDI] && (out_context->r[REG_EDI] - out_context->r[REG_ESI]) <= (unsigned long)var_size)
				{

					iret = func_op_movsb_ext(xx_inst, var, in_context, out_context);
					if (iret == 0)
					{
						return 0;
					}
					out_context->r[REG_ESI] = out_context->r[REG_ESI] + var_size;
					out_context->r[REG_EDI] = out_context->r[REG_EDI] + var_size;
				}
				else
				{
					/*取数据*/
					iret = xx_read_fmem(out_context->r[REG_ESI], (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}

					iret = xx_write_fmem(out_context->r[REG_EDI], (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}

					out_context->r[REG_ESI] = out_context->r[REG_ESI] + var_size;
					out_context->r[REG_EDI] = out_context->r[REG_EDI] + var_size;
				}
			}

			free(pvar);

			out_context->r[REG_ECX] = 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}






__declspec(noinline) int func_op_movs(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int n = 0;
	unsigned char *pvar = 0;
	unsigned char c_f3 = 0xf3;
	int var_size = 0;
	unsigned char tmp[0x10];

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	iret = func_inst_prefix(xx_inst, c_f3);
	if (iret == 0)
	{
		memset(tmp, 0, sizeof(tmp));
		pvar = tmp;
		var_size = var[0].size;

		/*取数据*/
		iret = xx_read_fmem(out_context->r[REG_ESI], (unsigned char*)pvar, var_size);
		if (iret == 0)
		{
			return 0;
		}

		iret = xx_write_fmem(out_context->r[REG_EDI], (unsigned char*)pvar, var_size);
		if (iret == 0)
		{
			return 0;
		}

		if (out_context->eflag & 0x400)
		{
			out_context->r[REG_ESI] = out_context->r[REG_ESI] - var_size;
			out_context->r[REG_EDI] = out_context->r[REG_EDI] - var_size;
		}
		else
		{
			out_context->r[REG_ESI] = out_context->r[REG_ESI] + var_size;
			out_context->r[REG_EDI] = out_context->r[REG_EDI] + var_size;
		}
	}
	else
	{
		var_size = out_context->r[REG_ECX] * var[0].size;

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
				if (out_context->r[REG_ESI] >= out_context->r[REG_EDI] && (out_context->r[REG_ESI] - out_context->r[REG_EDI]) <= (unsigned long)var_size)
				{
					out_context->r[REG_ESI] = out_context->r[REG_ESI] - var_size;
					out_context->r[REG_EDI] = out_context->r[REG_EDI] - var_size;
					iret = func_op_movsb_ext(xx_inst, var, in_context, out_context);
					if (iret == 0)
					{
						return 0;
					}
				}
				else
				{
					out_context->r[REG_ESI] = out_context->r[REG_ESI] - var_size;
					out_context->r[REG_EDI] = out_context->r[REG_EDI] - var_size;
					/*取数据*/
					iret = xx_read_fmem(out_context->r[REG_ESI] + var[0].size, (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}

					iret = xx_write_fmem(out_context->r[REG_EDI] + var[0].size, (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}
				}
			}
			else
			{
				if (out_context->r[REG_ESI] <= out_context->r[REG_EDI] && (out_context->r[REG_EDI] - out_context->r[REG_ESI]) <= (unsigned long)var_size)
				{

					iret = func_op_movsb_ext(xx_inst, var, in_context, out_context);
					if (iret == 0)
					{
						return 0;
					}
					out_context->r[REG_ESI] = out_context->r[REG_ESI] + var_size;
					out_context->r[REG_EDI] = out_context->r[REG_EDI] + var_size;
				}
				else
				{
					/*取数据*/
					iret = xx_read_fmem(out_context->r[REG_ESI], (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}

					iret = xx_write_fmem(out_context->r[REG_EDI], (unsigned char*)pvar, var_size);
					if (iret == 0)
					{
						free(pvar);
						return 0;
					}

					out_context->r[REG_ESI] = out_context->r[REG_ESI] + var_size;
					out_context->r[REG_EDI] = out_context->r[REG_EDI] + var_size;
				}
			}

			free(pvar);

			out_context->r[REG_ECX] = 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}




__declspec(noinline) int func_op_stosb(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;
	unsigned long n = 0;
	unsigned char c_f3 = 0xf3;
	unsigned char* pvar = 0;
	int var_size = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	iret = func_inst_prefix(xx_inst, c_f3);
	if (iret == 0)
	{
		memcpy(&var1, &out_context->r[REG_EAX], var[0].size);

		iret = xx_write_fmem(out_context->r[REG_EDI], (unsigned char*)&var1, var[0].size);
		if (iret == 0)
		{
			return 0;
		}

		if (out_context->eflag & 0x400)
		{
			out_context->r[REG_EDI] = out_context->r[REG_EDI] - var[0].size;
		}
		else
		{
			out_context->r[REG_EDI] = out_context->r[REG_EDI] + var[0].size;
		}
	}
	else
	{
		var_size = out_context->r[REG_ECX] * var[0].size;

		if (var_size != 0)
		{
			pvar = malloc(var_size);
			if (pvar == 0)
			{
				return 0;
			}	

			memset(pvar, 0, var_size);
			for (n = 0; n < out_context->r[REG_ECX]; n++)
			{
				memcpy(pvar + (n* var[0].size), &out_context->r[REG_EAX], var[0].size);
			}

			if (out_context->eflag & 0x400)
			{
				out_context->r[REG_EDI] = out_context->r[REG_EDI] - var_size;
				iret = xx_write_fmem(out_context->r[REG_EDI] + var[0].size, pvar, var_size);
				if (iret == 0)
				{
					free(pvar);
					return 0;
				}
			}
			else
			{
				iret = xx_write_fmem(out_context->r[REG_EDI], pvar, var_size);
				if (iret == 0)
				{
					free(pvar);
					return 0;
				}
				out_context->r[REG_EDI] = out_context->r[REG_EDI] + var_size;
			}

			out_context->r[REG_ECX] = 0;

			free(pvar);
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}




__declspec(noinline) int func_op_stos(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;
	unsigned long n = 0;
	unsigned char c_f3 = 0xf3;
	unsigned char* pvar = 0;
	int var_size = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	iret = func_inst_prefix(xx_inst, c_f3);
	if (iret == 0)
	{
		memcpy(&var1, &out_context->r[REG_EAX], var[0].size);

		iret = xx_write_fmem(out_context->r[REG_EDI], (unsigned char*)&var1, var[0].size);
		if (iret == 0)
		{
			return 0;
		}

		if (out_context->eflag & 0x400)
		{
			out_context->r[REG_EDI] = out_context->r[REG_EDI] - var[0].size;
		}
		else
		{
			out_context->r[REG_EDI] = out_context->r[REG_EDI] + var[0].size;
		}
	}
	else
	{
		var_size = out_context->r[REG_ECX] * var[0].size;

		if (var_size != 0)
		{
			pvar = malloc(var_size);
			if (pvar == 0)
			{
				return 0;
			}

			memset(pvar, 0, var_size);
			for (n = 0; n < out_context->r[REG_ECX]; n++)
			{
				memcpy(pvar + (n* var[0].size), &out_context->r[REG_EAX], var[0].size);
			}

			if (out_context->eflag & 0x400)
			{
				out_context->r[REG_EDI] = out_context->r[REG_EDI] - var_size;
				iret = xx_write_fmem(out_context->r[REG_EDI] + var[0].size, pvar, var_size);
				if (iret == 0)
				{
					free(pvar);
					return 0;
				}
			}
			else
			{
				iret = xx_write_fmem(out_context->r[REG_EDI], pvar, var_size);
				if (iret == 0)
				{
					free(pvar);
					return 0;
				}
				out_context->r[REG_EDI] = out_context->r[REG_EDI] + var_size;
			}

			out_context->r[REG_ECX] = 0;

			free(pvar);
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}
