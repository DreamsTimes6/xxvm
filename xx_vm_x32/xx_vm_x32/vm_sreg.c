#include "stdio.h"
#include <windows.h>
//#include "Plugin.h"
#include "xx_vm.h"
#include "xx_comm32.h"
#include "xxdisasm32.h"
#include "xx_opdef3.h"
#include "xxdisasm32.h"



char xreg_rax[] = "00000801080601";
char xreg_rcx[] = "00010801080601";
char xreg_rdx[] = "00020801080601";
char xreg_rbx[] = "00030801080601";
char xreg_rsp[] = "00040801080601";
char xreg_rbp[] = "00050801080601";
char xreg_rsi[] = "00060801080601";
char xreg_rdi[] = "00070801080601";


char xreg_r8[]  = "01000801080601";
char xreg_r9[]  = "01010801080601";
char xreg_r10[] = "01020801080601";
char xreg_r11[] = "01030801080601";
char xreg_r12[] = "01040801080601";
char xreg_r13[] = "01050801080601";
char xreg_r14[] = "01060801080601";
char xreg_r15[] = "01070801080601";



char xreg_eax[] = "00000401040401";
char xreg_ecx[] = "00010401040401";
char xreg_edx[] = "00020401040401";
char xreg_ebx[] = "00030401040401";
char xreg_esp[] = "00040401040401";
char xreg_ebp[] = "00050401040401";
char xreg_esi[] = "00060401040401";
char xreg_edi[] = "00070401040401";


char xreg_r8d[]  = "01000401040401";
char xreg_r9d[]  = "01010401040401";
char xreg_r10d[] = "01020401040401";
char xreg_r11d[] = "01030401040401";
char xreg_r12d[] = "01040401040401";
char xreg_r13d[] = "01050401040401";
char xreg_r14d[] = "01060401040401";
char xreg_r15d[] = "01070401040401";
//////////////////////////////////////////////////////////////


char vm_sp[100];
char vm_regbase[100];
char vm_opcode[100];
char vm_ip[100];
char vm_key[100];

char vm_osp[100];
char vm_oregbase[100];
char vm_oopcode[100];
char vm_oip[100];
char vm_okey[100];
////////////////////////////////////////////////////////////////




int xreg_cmp(char* def1, char* def2)
{
	int n = 0;
	char buf1[100];
	char buf2[100];

	memset(buf1, 0, sizeof(buf1));
	memset(buf2, 0, sizeof(buf2));

	nstr_hex(def1, strlen(def1), buf1);
	nstr_hex(def2, strlen(def2), buf2);

	if (buf1[0] == buf2[0] && buf1[1] == buf2[1])
	{
		if (buf1[4] == buf2[4] && buf1[5] == buf2[5])
		{
			return 0;
		}
	}
	return -1;
}

int xreg_incmp(char* def1, char* def2)
{
	int n = 0;
	char reg[100];
	char sub[100];

	memset(reg, 0, sizeof(reg));
	memset(sub, 0, sizeof(sub));

	nstr_hex(def1, strlen(def1), reg);
	nstr_hex(def2, strlen(def2), sub);

	if (reg[0] == sub[0] && reg[1] == sub[1])
	{
		if (reg[4] >= sub[4])
		{
			return 0;
		}
	}
	return -1;
}


int xx_item_maddr(struct XX_INST* xx_inst,int *nitem)
{
	int n = 0;
	for (n=0;n<xx_inst->xx_inst_items.nitem;n++)
	{
		if (xx_inst->xx_inst_items.xx_inst_items_flag[n].item_type != INST_ITEM_TYPE_IADDR && \
			xx_inst->xx_inst_items.xx_inst_items_flag[n].item_type != INST_ITEM_TYPE_DIS && \
			xx_inst->xx_inst_items.xx_inst_items_flag[n].item_type != INST_ITEM_TYPE_IMM && \
			xx_inst->xx_inst_items.xx_inst_items_flag[n].item_type != INST_ITEM_TYPE_CONST && \
			xx_inst->xx_inst_items.xx_inst_items_flag[n].item_type != INST_ITEM_TYPE_REG && \
			xx_inst->xx_inst_items.xx_inst_items_var[n].index_def_len==0)
		{
			*nitem = n;
			return 1;
		}
	}
	return 0;
}

int xx_vmp_sp(struct XX_INST *xx_inst,int nitem,char *sp_def)
{
	int n = 0;

	n = nitem;
	
	if (xreg_cmp(xx_inst->xx_inst_items.xx_inst_items_var[n].item_base_def, xreg_rsp) != 0 && \
		xreg_cmp(xx_inst->xx_inst_items.xx_inst_items_var[n].item_base_def, vm_ip) != 0 && \
		xreg_cmp(xx_inst->xx_inst_items.xx_inst_items_var[n].item_base_def, vm_opcode) != 0 )
	{
		memcpy(sp_def, xx_inst->xx_inst_items.xx_inst_items_var[n].item_base_def, \
			xx_inst->xx_inst_items.xx_inst_items_var[n].base_def_len);
		return 1;
	}
	
	return 0;
}


#if 1

int init_vmsreg2(struct XX_INST *xx_inst,int num)
{
	int n=0;
	int iret=0;
	char var_ip[100];
	char var_opcode[100];
	char var_sp[100];
	char tmp[200];
	int ebp_flag=0;
	int esi_flag=0;
	int edi_flag=0;
	
	

	memcpy(vm_oip,vm_ip,strlen(vm_ip));
	memcpy(vm_osp,vm_sp,strlen(vm_sp));
	memcpy(vm_oopcode,vm_opcode,strlen(vm_opcode));
	memcpy(vm_oregbase,vm_regbase,strlen(vm_regbase));
	memcpy(vm_okey,vm_key,strlen(vm_key));

	memset(vm_sp, 0, sizeof(vm_sp));
	memset(vm_ip, 0, sizeof(vm_ip));
	memset(vm_opcode, 0, sizeof(vm_opcode));

	memset(var_ip, 0, sizeof(var_ip));
	memset(var_opcode, 0, sizeof(var_opcode));
	memset(var_sp, 0, sizeof(var_sp));


	for(n=0;n<num;n++)
	{
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_ret,sop_len)==0 &&
			memcmp(xx_inst[n-1].xx_inst_code.opcode_type,sop_push,sop_len)==0 &&
			xx_inst[n-1].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG)
		{
			memset(var_ip,0,sizeof(var_ip));
			memcpy(var_ip,xx_inst[n-1].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
				xx_inst[n-1].xx_inst_items.xx_inst_items_var[0].reg_def_len);
			break;
		}
		else if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_jmp,sop_len)==0 &&
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG)
		{
			memset(var_ip,0,sizeof(var_ip));
			memcpy(var_ip,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);
			break;
		}
		else if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_ret,sop_len)==0)
		{
			return -2;
		}
	}
	if(strlen(var_ip))
	{
		//memcpy(vm_ip,var_ip,strlen(var_ip));
		if(xreg_cmp(var_ip,xreg_edi)==0 )
		{
			edi_flag=1;
		}
		else if(xreg_cmp(var_ip,xreg_esi)==0 )
		{
			esi_flag=1;
		}
		else if(xreg_cmp(var_ip,xreg_ebp)==0 )
		{
			ebp_flag=1;
		}
		else
		{
			memset(tmp,0,sizeof(tmp));
			xx_get_reg_mic(var_ip,tmp);
			//_Addtolist(0,1,"var_ip:%s",tmp);
			return -2;
		}
	}
	else
	{
		//_Addtolist(0,1,"var_ip");
		return -2;
	}

	for(n=0;n<num;n++)
	{
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_add,sop_len)==0 && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_IMM && \
			*(unsigned int*)xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const==0x04)
		{
			memset(var_opcode,0,sizeof(var_opcode));
			memcpy(var_opcode,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);
		}
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_lea,sop_len)==0 && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGDIS && \
			*(unsigned int*)xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const==0x04 && \
			xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def,\
			xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def)==0)
		{
			memset(var_opcode,0,sizeof(var_opcode));
			memcpy(var_opcode,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);
		}
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_sub,sop_len)==0 && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_IMM && \
			*(unsigned int*)xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const==0x04)
		{
			memset(var_opcode,0,sizeof(var_opcode));
			memcpy(var_opcode,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);
		}
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_lea,sop_len)==0 && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGDIS && \
			*(unsigned int*)xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const==0xfffffffc && \
			xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def,\
			xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def)==0)
		{
			memset(var_opcode,0,sizeof(var_opcode));
			memcpy(var_opcode,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);
		}
	}
	if(strlen(var_opcode))
	{
		//memcpy(vm_opcode,var_opcode,strlen(var_opcode));
		if(xreg_cmp(var_opcode,xreg_edi)==0 )
		{
			edi_flag=1;
		}
		else if(xreg_cmp(var_opcode,xreg_esi)==0 )
		{
			esi_flag=1;
		}
		else if(xreg_cmp(var_opcode,xreg_ebp)==0 )
		{
			ebp_flag=1;
		}
		else
		{
			memset(tmp,0,sizeof(tmp));
			xx_get_reg_mic(var_opcode,tmp);
			//_Addtolist(0,1,"var_opcode:%s",tmp);
			return -2;
		}
	}
	else
	{
		//_Addtolist(0,1,"var_opcode");
		return -2;
	}

	if((esi_flag+edi_flag+ebp_flag)!=2)
	{
		memset(tmp,0,sizeof(tmp));
		xx_get_reg_mic(var_ip,tmp);
		//_Addtolist(0,1,"var_ip:%s",tmp);
		memset(tmp,0,sizeof(tmp));
		xx_get_reg_mic(var_opcode,tmp);
		//_Addtolist(0,1,"var_opcode:%s",tmp);
		return -2;
	}

	if(esi_flag==0 )
	{
		memcpy(var_sp,xreg_esi,sizeof(xreg_esi));
		esi_flag=1;
	}
	if(edi_flag==0 )
	{
		memcpy(var_sp,xreg_edi,sizeof(xreg_edi));
		edi_flag=1;
	}
	if(ebp_flag==0 )
	{
		memcpy(var_sp,xreg_ebp,sizeof(xreg_ebp));
		ebp_flag=1;
	}

	if(strlen(var_sp)==0)
	{
		memset(tmp,0,sizeof(tmp));
		xx_get_reg_mic(var_ip,tmp);
		//_Addtolist(0,1,"var_ip:%s",tmp);
		memset(tmp,0,sizeof(tmp));
		xx_get_reg_mic(var_opcode,tmp);
		//_Addtolist(0,1,"var_opcode:%s",tmp);
		return -2;
	}

	
	memcpy(vm_sp,var_sp,strlen(var_sp));
	memcpy(vm_ip,var_ip,strlen(var_ip));
	memcpy(vm_opcode,var_opcode,strlen(var_opcode));
	return 0;
}

#else
/*64Î»Ê¹ÓÃ*/
int init_vmsreg2(struct XX_INST *xx_inst, int num)
{
	int n = 0;
	int iret = 0;
	char var_ip[40];
	char var_opcode[40];
	char var_sp[40];
	char tmp[200];
	int ebp_flag = 0;
	int esi_flag = 0;
	int edi_flag = 0;
	int nitem = 0;


	memcpy(vm_oip, vm_ip, strlen(vm_ip));
	memcpy(vm_osp, vm_sp, strlen(vm_sp));
	memcpy(vm_oopcode, vm_opcode, strlen(vm_opcode));
	memcpy(vm_oregbase, vm_regbase, strlen(vm_regbase));
	memcpy(vm_okey, vm_key, strlen(vm_key));


	memset(vm_sp, 0, sizeof(vm_sp));
	memset(vm_ip, 0, sizeof(vm_ip));
	memset(vm_opcode, 0, sizeof(vm_opcode));
	memset(vm_regbase, 0, sizeof(vm_regbase));


	memcpy(vm_regbase, xreg_rsp, strlen(xreg_rsp));



	for (n = 0; n < num; n++)
	{
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, op_ret, op_len) == 0 &&
			memcmp(xx_inst[n - 1].xx_inst_code.opcode_type, op_push, op_len) == 0 &&
			xx_inst[n - 1].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
		{
			memset(var_ip, 0, sizeof(var_ip));
			memcpy(var_ip, xx_inst[n - 1].xx_inst_items.xx_inst_items_var[0].item_reg_def, \
				xx_inst[n - 1].xx_inst_items.xx_inst_items_var[0].reg_def_len);
			break;
		}
		else if (memcmp(xx_inst[n].xx_inst_code.opcode_type, op_jmp, op_len) == 0 &&
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
		{
			memset(var_ip, 0, sizeof(var_ip));
			memcpy(var_ip, xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def, \
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);
			break;
		}
		else if (memcmp(xx_inst[n].xx_inst_code.opcode_type, op_ret, op_len) == 0)
		{
			return -1;
		}
	}
	
	if (strlen(var_ip)==0)
	{
		//_Addtolist(0, 1, "var_ip");
		return -2;
	}
	memcpy(vm_ip, var_ip, strlen(var_ip));

	for (n = 0; n < num; n++)
	{
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, op_add, op_len) == 0 && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_IMM && \
			*(unsigned int*)xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const == 0x04)
		{
			memset(var_opcode, 0, sizeof(var_opcode));
			memcpy(var_opcode, xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def, \
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);
		}
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, op_lea, op_len) == 0 && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_MREGDIS && \
			*(unsigned int*)xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const == 0x04 && \
			xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def, \
				xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def) == 0)
		{
			memset(var_opcode, 0, sizeof(var_opcode));
			memcpy(var_opcode, xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def, \
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);
		}
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, op_sub, op_len) == 0 && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_IMM && \
			*(unsigned int*)xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const == 0x04)
		{
			memset(var_opcode, 0, sizeof(var_opcode));
			memcpy(var_opcode, xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def, \
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);
		}
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, op_lea, op_len) == 0 && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG && \
			xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_MREGDIS && \
			*(unsigned int*)xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const == 0xfffffffc && \
			xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def, \
				xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def) == 0)
		{
			memset(var_opcode, 0, sizeof(var_opcode));
			memcpy(var_opcode, xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def, \
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);
		}
	}
	if (strlen(var_opcode)==0)
	{
		//_Addtolist(0, 1, "var_opcode");
		return -2;
	}
	memcpy(vm_opcode, var_opcode, strlen(var_opcode));

	for (n = 0; n < num; n++)
	{
		iret=xx_item_maddr(&xx_inst[n],&nitem);
		if (iret)
		{
			memset(var_sp, 0, sizeof(var_sp));
			iret = xx_vmp_sp(&xx_inst[n], nitem, var_sp);
			if (iret)
			{
				break;
			}
		}
	}
	if (strlen(var_sp) == 0)
	{
		//_Addtolist(0, 1, "var_opcode");
		return -3;
	}


	memcpy(vm_sp, var_sp, strlen(var_sp));
	
	

	
	return 0;
}
#endif


