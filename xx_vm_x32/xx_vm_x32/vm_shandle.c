#include "stdio.h"
#include <windows.h>
//#include "Plugin.h"
#include "xx_vm.h"
#include "xx_comm32.h"
#include "xx_inst.h"
#include "xx_opdef.h"
#include "xxdisasm32.h"


extern char xreg_unknow[];
extern char xreg_rax[];
extern char xreg_rbx[];
extern char xreg_rcx[];
extern char xreg_rdx[];
extern char xreg_rbp[];
extern char xreg_rsp[];
extern char xreg_rsi[];
extern char xreg_rdi[];

extern char xreg_eax[];
extern char xreg_ebx[];
extern char xreg_ecx[];
extern char xreg_edx[];
extern char xreg_ebp[];
extern char xreg_esp[];
extern char xreg_esi[];
extern char xreg_edi[];

extern char xreg_ax[];
extern char xreg_bx[];
extern char xreg_cx[];
extern char xreg_dx[];
extern char xreg_bp[];
extern char xreg_sp[];
extern char xreg_si[];
extern char xreg_di[];

extern char xreg_al[];
extern char xreg_bl[];
extern char xreg_cl[];
extern char xreg_dl[];

extern char xreg_ah[];
extern char xreg_bh[];
extern char xreg_ch[];
extern char xreg_dh[];

extern void    cdecl _Addtolist(long addr,int highlight,char *format,...);
extern int     cdecl _Pluginreadstringfromini(HINSTANCE dllinst,char *key,char *s,char *def);
extern unsigned long   cdecl _Readcommand(unsigned long ip,char *cmd);

extern int write_file(char *file,char* data,int dsize);
extern int init_file_name(char *pname,char *file);
extern int get_reg_def(char *reg_mic,char *reg_def);
extern int xreg_cmp(char* buf1,char* buf2);
extern int xreg_incmp(char* buf1,char* buf2);
extern int xop_jcc(char *opdef);

//extern int vm_shandle_0000(struct XX_INST *xx_inst,int num,char *outs,int);
//extern int vm_shandle_0f00(struct XX_INST *xx_inst,int num,char *outs,int);
//extern int vm_shandle_0f01(struct XX_INST *xx_inst,int num,char *outs,int);
extern int vm_shandle_1200(struct XX_INST *xx_inst,int num,char *outs,int);
extern int vm_shandle_1401(struct XX_INST *xx_inst,int num,char *outs,int);


extern char *svm_handle_invalid;
extern char *rline;



extern char vm_sp[];
extern char vm_regbase[];
extern char vm_opcode[];
extern char vm_ip[];
extern char vm_key[];

extern char vm_osp[];
extern char vm_oregbase[];
extern char vm_oopcode[];
extern char vm_oip[];
extern char vm_okey[];

static ST_SHANDLE st_shandle_func[]=
{
	//{0x0000,"entry   ",               vm_shandle_0000},
	//{0x0f00,"jmp_1  ",                vm_shandle_0f00},
	//{0x0f01,"jmp  ",                  vm_shandle_0f01},
	{0x1200,"ret  ",                  vm_shandle_1200},
	{0x1401,"call dreg  ",            vm_shandle_1401},
};
#define ST_SHANDLE_NUM   sizeof(st_shandle_func)/sizeof(ST_SHANDLE)


extern char log_file[];  //日志文件名
extern char opmz_file[];  //优化数据文件名
extern char opmz_dfile[];  //优化数据文件名

//struct VM_REG_STATUS reg_status_regbase;
//struct VM_REG_STATUS reg_status_stack;

//extern struct XX_INST g_inst[];
//extern int g_num;
extern int call_flag;
extern int g_inst_num;
//////////////////////////////////////////////
int init_vmsreg2(struct XX_INST *xx_inst,int num);
///////////////////////////////////////////////


int vm_shandle(unsigned long base,unsigned long curt_ip,char *plog)
{
	char tmp[200];
	char cmdbuf[200];
	char buff1[200];
	char buff2[200];

	int iret=0;
	int iret2=0;
	int tmp_size=0;
	int n=0;
	unsigned long ip=0;
	struct XX_INST *xx_inst;
	struct XX_INST v_inst[150];
	int num=0;
	int cmdlen=0;
	char out_var[0x10];


	ip=curt_ip;

#if 0
	xx_inst=malloc(sizeof(struct XX_INST)*100);
	memset(xx_inst,0,sizeof(struct XX_INST)*100);
#else
	xx_inst=v_inst;
	memset(&v_inst,0,sizeof(v_inst));
#endif


	n=0;
	while(n<140)
	{
		memset(cmdbuf,0,sizeof(cmdbuf));
		iret=_Readmemory((char*)cmdbuf,ip,MAXCMDSIZE,MM_RESTORE);
		if(iret!=MAXCMDSIZE)
		{
			_Addtolist(0,1,"ReadCommand error ip:[%08x]",ip);
			return -1;
		}
		
		xx_inst[n].xx_inst_code.inst_system = INST_SYSTEM_CURT;
		memcpy(xx_inst[n].xx_inst_code.base, (char*)&base, xx_inst[n].xx_inst_code.inst_system);
		memcpy(xx_inst[n].xx_inst_code.current_addr, (char*)&ip, xx_inst[n].xx_inst_code.inst_system);
		xx_disasm(&xx_inst[n],cmdbuf);
		//_Addtolist(0,1,"finish_flag:[%d]",xx_inst[n].xx_inst_exist.finish_flag);

		if(xx_inst[n].xx_inst_exist.finish_flag!=INST_FLAG_EXIST)
		{
			_Addtolist(0,1,"xx_disasm error ip:[%08x]",ip);
			return -1;
		}
		//_Addtolist(0,1,"xx_disasm  ip:[%08x] [%s]",ip,xx_inst[n].xx_inst_mic.disasm);

		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_jmp,op_len)==0 &&
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_DIS)
		{
			ip=*(int*)xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_const;
			//_Addtolist(0,1,"jmp ip:[%08x]",ip);
		}
		else if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_ja,op_len)==0 &&
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_DIS)
		{
			ip=*(int*)xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_const;
			//_Addtolist(0,1,"jmp ip:[%08x]",ip);
		}
		else if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_call,op_len)==0 && \
			xx_inst[n].xx_inst_items.xx_inst_items_var[0].const_size!=0)
		{
			ip=*(int*)xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_const;
		}
		else
		{
			ip=ip+xx_inst[n].xx_inst_code.disasm_length;
		}

		
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_ret,op_len)==0 &&
			memcmp(xx_inst[n-1].xx_inst_code.opcode_type,op_push,op_len)==0 &&
			xx_inst[n-1].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG)
		{
			break;
		}
		else if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_jmp,op_len)==0 &&
			xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG)
		{
			break;
		}
		else if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_ret,op_len)==0)
		{
			break;
		}

		n++;
	}
	num=n+1;

	

	//_Addtolist(0,1,"xx_disasm  ip:[%08x] [%s]",curt_ip,xx_inst[0].xx_inst_mic.disasm);

	
	tmp_size=sizeof(tmp);
	memset(tmp,0,sizeof(tmp));
	

	for(n=0;n<ST_SHANDLE_NUM;n++)
	{
		iret=(st_shandle_func[n].func_shandle)(xx_inst,num,tmp,tmp_size);
		if(iret==st_shandle_func[n].seq_handle)
		{
			break;
		}
	}

	call_flag=0;
	if(iret==0x1401)
	{
		call_flag=1;
		iret=-1;
	}
	//free(xx_inst);
	if(iret==0x1200)
	{
		memset(out_var,0,sizeof(out_var));
		memset(buff1,0,sizeof(buff1));
		memset(buff2,0,sizeof(buff2));
		byte_opposite(xx_inst->xx_inst_code.current_addr,xx_inst->xx_inst_code.inst_system,buff1);
		nhex_str(buff1,xx_inst->xx_inst_code.inst_system,buff2,"");
		//strcat(buff2,"  ");
		//strcat(plog,buff2);
		//strcat(plog,st_handle_func[n].str_handle);
		//strcat(plog,tmp);
		//strcat(plog,rline);
		sprintf(plog, "%8d: %s  %s%s%s", g_inst_num, buff2, st_shandle_func[n].str_handle, tmp, rline);
		g_inst_num++;
		/*判断优化标志*/
		iret2 = xx_opmz_data(opmz_file, opmz_dfile, iret, out_var, plog);
		if (iret2 == 0)
		{
			MessageBox(0,"opmz_data error","info",MB_OK);
			return -1;
		}
	}
	else
	{
		/*检查,重写*/
		iret2=init_vmsreg2(xx_inst,num);
		if(iret2==-2)
		{
			_Addtolist(0,1,"init_vmsreg2 error");
			return -2;
		}
	}


	return iret;
}


#if 0


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










