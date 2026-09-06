#include "stdio.h"
#include <windows.h>
#include "xx_vm.h"
#include "xx_comm32.h"
#include "xxdisasm32.h"
#include "xx_opdef3.h"
#include "xx_opmz.h"

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

////////////////////////////////////////////////
extern int vm_handle_0000(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0100(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0101(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0102(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0200(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0201(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0202(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0300(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0301(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0302(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0400(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0401(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0402(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0500(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0501(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0502(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0600(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0601(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0602(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0700(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0701(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0800(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
//extern int vm_handle_0801(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int);
extern int vm_handle_0900(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0901(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0902(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0a00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0a10(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0b00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0b01(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0b02(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0b10(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0c00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0c10(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0d00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0d01(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0d02(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0e00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0e01(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0e02(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0e10(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0f00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_0f01(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_1000(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_1001(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_1100(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_1200(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_1300(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
extern int vm_handle_1400(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);


char *svm_handle_invalid="invalid handle   ";
char *rline="\r\n";


extern int g_inst_num;
//////////////////////////////////////////////////


extern char log_file[];  //日志文件名
extern char opmz_file[];  //优化数据文件名
extern char opmz_dfile[];  //优化数据文件名

//extern struct VM_REG_STATUS reg_status_regbase;
//extern struct VM_REG_STATUS reg_status_stack;


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


int push_record=0;


//typedef int(*PITEM_TYPE_FUNC)(int,struct XX_INST*,uchar*);


static ST_HANDLE st_handle_func[]=
{
	{0x0000,"entry   "            ,           vm_handle_0000},
	{0x0100,"push dreg           ",           vm_handle_0100},
	{0x0101,"push wreg           ",           vm_handle_0101},
	{0x0102,"push breg           ",           vm_handle_0102},
	{0x0200,"push dconst         ",           vm_handle_0200},
	{0x0201,"push wconst         ",           vm_handle_0201},
	{0x0202,"push bconst         ",           vm_handle_0202},
	{0x0300,"pop dreg            ",           vm_handle_0300},
	{0x0301,"pop wreg            ",           vm_handle_0301},
	{0x0302,"pop breg            ",           vm_handle_0302},
	{0x0400,"add d[sp+4],d[sp]   ",           vm_handle_0400},
	{0x0401,"add w[sp+2],w[sp]   ",           vm_handle_0401},
	{0x0402,"add b[sp+2],b[sp]   ",           vm_handle_0402},
	{0x0500,"mov d[sp],d[[sp]]   ",           vm_handle_0500},
	{0x0501,"mov w[sp],w[[sp]]   ",           vm_handle_0501},
	{0x0502,"mov w[sp],b[[sp]]   ",           vm_handle_0502},
	{0x0600,"mov d[[sp]],d[sp+4] ",           vm_handle_0600},
	{0x0601,"mov w[[sp]],w[sp+4] ",           vm_handle_0601},
	{0x0602,"mov b[[sp]],b[sp+4] ",           vm_handle_0602},
	{0x0700,"push dsp            ",           vm_handle_0700},
	{0x0701,"push wsp            ",           vm_handle_0701},
	{0x0800,"calc                ",           vm_handle_0800},
	//{0x0801,"calc                ",           vm_handle_0801},
	{0x0900,"and ~d[sp+4],~d[sp] ",           vm_handle_0900},
	{0x0901,"and ~w[sp+2],~w[sp] ",           vm_handle_0901},
	{0x0902,"and ~b[sp+2],~b[sp] ",           vm_handle_0902},
	{0x0a00,"div d[sp+4],d[sp+8]   ",           vm_handle_0a00},
	{0x0a10,"idiv d[sp+4],d[sp+8]   ",          vm_handle_0a10},
	{0x0b00,"shr d[sp],b[sp+4]   ",           vm_handle_0b00},
	{0x0b01,"shr w[sp],b[sp+2]   ",           vm_handle_0b01},
	{0x0b02,"shr b[sp],b[sp+2]   ",           vm_handle_0b02},
	{0x0b10,"shrd d[sp],d[sp+4],b[sp+8]   ",  vm_handle_0b10},
	{0x0c00,"mul d[sp+4],d[sp]   ",           vm_handle_0c00},
	{0x0c10,"imul d[sp+4],d[sp]  ",           vm_handle_0c10},
	{0x0d00,"or ~d[sp+4],~d[sp]  ",           vm_handle_0d00},
	{0x0d01,"or ~w[sp+2],~w[sp]  ",           vm_handle_0d01},
	{0x0d02,"or ~b[sp+2],~b[sp]  ",           vm_handle_0d02},
	{0x0e00,"shl d[sp],b[sp+4]   ",           vm_handle_0e00},
	{0x0e01,"shl w[sp],b[sp+2]   ",           vm_handle_0e01},
	{0x0e02,"shl b[sp],b[sp+2]   ",           vm_handle_0e02},
	{0x0e10,"shld d[sp],d[sp+4],b[sp+8]   ",  vm_handle_0e10},
	{0x0f00,"jmp  ",                          vm_handle_0f00},
	//{0x0f01,"jmp  ",                          vm_handle_0f01},
	{0x1000,"cpuid               ",           vm_handle_1000},
	{0x1001,"rdtsc               ",           vm_handle_1001},
	{0x1100,"mov sp,[sp]         ",           vm_handle_1100},
	{0x1200,"ret   ",                         vm_handle_1200},
	{0x1300,"popfd               ",           vm_handle_1300},
	{0x1400,"call dreg           ",           vm_handle_1400},
};

#define ST_HANDLE_NUM   sizeof(st_handle_func)/sizeof(ST_HANDLE)



//int vm_handle(struct XX_INST *xx_inst,struct XX_CONTECT *xx_contect,int num)
int vm_handle(struct XX_HTHREAD *x_thread)
{
	char tmp[200];
	char buff1[200];
	char buff2[200];
	int iret=0;
	int iret2 = 0;
	int tmp_size=0;
	int n=0;
	struct XX_INST *xx_inst=0;
	struct XX_CONTEXT *xx_context=0;
	int num=0;
	char *plog=0;
	char out_var[0x10];

	xx_inst=x_thread->xx_inst;
	xx_context=x_thread->xx_context;
	num=x_thread->inst_num;
	plog=x_thread->plogdata;

	tmp_size=sizeof(tmp);

	memset(tmp,0,sizeof(tmp));


	

	for(n=0;n<ST_HANDLE_NUM;n++)
	{
		memset(out_var, 0, sizeof(out_var));
		iret=(st_handle_func[n].func_handle)(xx_inst,num,xx_context,tmp,tmp_size,out_var);
		if(iret==st_handle_func[n].seq_handle)
		{
			memset(buff1,0,sizeof(buff1));
			memset(buff2,0,sizeof(buff2));
			byte_opposite(xx_inst->xx_inst_code.current_addr,xx_inst->xx_inst_code.inst_system,buff1);
			nhex_str(buff1,xx_inst->xx_inst_code.inst_system,buff2,"");
			//strcat(buff2,"  ");
			//strcat(plog,buff2);
			//strcat(plog,st_handle_func[n].str_handle);
			//strcat(plog,tmp);
			//strcat(plog,rline);
			sprintf(plog, "%8d: %s  %s%s%s", g_inst_num, buff2, st_handle_func[n].str_handle, tmp, rline);
			
			/*判断优化标志*/
			iret2 = xx_opmz_data(opmz_file, g_inst_num, iret, out_var);
			if (iret2 == 0)
			{
				MessageBox(0,"opmz_data error","info",MB_OK);
				return -1;
			}

			g_inst_num++;

			return iret;
		}
	}


	memset(tmp, 0, sizeof(tmp));
	memset(out_var, 0, sizeof(out_var));
	memset(buff1, 0, sizeof(buff1));
	memset(buff2, 0, sizeof(buff2));
	byte_opposite(xx_inst->xx_inst_code.current_addr, xx_inst->xx_inst_code.inst_system, buff1);
	nhex_str(buff1, xx_inst->xx_inst_code.inst_system, buff2, "");
	//strcat(buff2,"  ");
	//strcat(plog,buff2);
	//strcat(plog,st_handle_func[n].str_handle);
	//strcat(plog,tmp);
	//strcat(plog,rline);
	sprintf(plog, "%8d: %s  %s%s%s", g_inst_num, buff2, svm_handle_invalid, tmp, rline);
	
	/*判断优化标志*/
	//iret2 = xx_opmz_data(opmz_file, g_inst_num, 1, out_var);
	//if (iret2 == 0)
	//{
	//	MessageBox(0,"opmz_data error","info",MB_OK);
	//	return -1;
	//}

	xx_opmz_write_invalid(opmz_file, opmz_dfile, g_inst_num);

	g_inst_num++;

	return iret;

}











