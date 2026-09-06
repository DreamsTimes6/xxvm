#include "stdio.h"
#include <windows.h>
#include "xx_vm.h"
#include "xx_comm64.h"
#include "xxdisasm64.h"
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


extern char xreg_r8[];
extern char xreg_r9[];
extern char xreg_r10[];
extern char xreg_r11[];
extern char xreg_r12[];
extern char xreg_r13[];
extern char xreg_r14[];
extern char xreg_r15[];


extern char xreg_r8d[];
extern char xreg_r9d[];
extern char xreg_r10d[];
extern char xreg_r11d[];
extern char xreg_r12d[];
extern char xreg_r13d[];
extern char xreg_r14d[];
extern char xreg_r15d[];



extern char log_file[];  //日志文件名
extern char opmz_file[];  //优化数据文件名
extern char opmz_dfile[];  //优化数据文件名

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


//extern void    cdecl _Addtolist(long addr,int highlight,char *format,...);
//extern int     cdecl _Pluginreadstringfromini(HINSTANCE dllinst,char *key,char *s,char *def);

////////////////////////////////////////////////
extern int vm_handle_0000(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_0100(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_0101(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_0102(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10103(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_0200(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_0201(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_0202(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10203(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_0300(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_0301(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_0302(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10303(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10400(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10401(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10402(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10403(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10500(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10501(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10502(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10503(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10610(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10611(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10612(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10613(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_0700(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_0701(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_0703(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_0800(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
//extern int vm_handle_0801(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int);
extern int vm_handle_10900(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10901(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10902(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10903(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10a00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10a10(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10a23(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10a33(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10b00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10b01(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10b02(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10b03(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_0b10(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_0b11(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10c00(struct XX_INST *xx_inst,int num,  struct XX_CONTEXT64 *xx_contect, char *outs,int, char*);
extern int vm_handle_10c10(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10c23(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10c33(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10d00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10d01(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10d02(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10d03(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10e00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10e01(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_10e02(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_10e03(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_0e10(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_0e11(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_0f00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_0f01(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_1000(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_1001(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_1100(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_1103(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_1200(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_1300(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);
extern int vm_handle_1303(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
extern int vm_handle_1400(struct XX_INST *xx_inst,int num,struct XX_CONTEXT64 *xx_contect,char *outs,int, char*);


char *svm_handle_invalid="invalid handle   ";
char *rline="\r\n";

//////////////////////////////////////////////////
int g_inst_num;

////////////////////////////////////////////////////

static ST_HANDLE64 st_handle_func[]=
{
	{0x0000,0,"entry   "            ,           vm_handle_0000},
	{0x0100,4,"push dreg           ",           vm_handle_0100},
	{0x0101,2,"push wreg           ",           vm_handle_0101},
	{0x0102,1,"push breg           ",           vm_handle_0102},
	{0x10103,8,"push qreg           ",          vm_handle_10103},
	{0x0200,4,"push dconst         ",           vm_handle_0200},
	{0x0201,2,"push wconst         ",           vm_handle_0201},
	{0x0202,1,"push bconst         ",           vm_handle_0202},
	{0x10203,8,"push qconst         ",          vm_handle_10203},
	{0x0300,4,"pop dreg            ",           vm_handle_0300},
	{0x0301,2,"pop wreg            ",           vm_handle_0301},
	{0x0302,1,"pop breg            ",           vm_handle_0302},
	{0x10303,8,"pop qreg            ",          vm_handle_10303},
	{0x10400,4,"add d[sp+4],d[sp]   ",           vm_handle_10400},
	{0x10401,2,"add w[sp+2],w[sp]   ",           vm_handle_10401},
	{0x10402,1,"add b[sp+2],b[sp]   ",           vm_handle_10402},
	{0x10403,8,"add q[sp+8],q[sp]   ",           vm_handle_10403},
	{0x10500,4,"mov d[sp],d[[sp]]   ",           vm_handle_10500},
	{0x10501,2,"mov w[sp],w[[sp]]   ",           vm_handle_10501},
	{0x10502,1,"mov w[sp],b[[sp]]   ",           vm_handle_10502},
	{0x10503,8,"mov q[sp],q[[sp]]   ",           vm_handle_10503},
	//{0x0600,8,"mov d[[sp]],d[sp+4] ",           vm_handle_0600},
	{0x10610,4,"mov d[[sp]],d[sp+8] ",           vm_handle_10610},
	{0x10611,2,"mov w[[sp]],w[sp+8] ",           vm_handle_10611},
	{0x10612,1,"mov b[[sp]],b[sp+8] ",           vm_handle_10612},
	{0x10613,8,"mov q[[sp]],q[sp+8] ",           vm_handle_10613},
	{0x0700,4,"push dsp            ",           vm_handle_0700},
	{0x0701,2,"push wsp            ",           vm_handle_0701},
	{0x0703,8,"push qsp            ",           vm_handle_0703},
	{0x0800,0,"crc                ",           vm_handle_0800},
	//{0x0801,"calc                ",           vm_handle_0801},
	{0x10900,4,"and ~d[sp+4],~d[sp] ",           vm_handle_10900},
	{0x10901,2,"and ~w[sp+2],~w[sp] ",           vm_handle_10901},
	{0x10902,1,"and ~b[sp+2],~b[sp] ",           vm_handle_10902},
	{0x10903,8,"and ~q[sp+8],~q[sp] ",           vm_handle_10903},
	{0x10a00,4,"div d[sp+4],d[sp+8]   ",           vm_handle_10a00},
	{0x10a10,4,"idiv d[sp+4],d[sp+8]   ",          vm_handle_10a10},
	{0x10a23,4,"div q[sp+8],q[sp+10]   ",           vm_handle_10a23},
	{0x10a33,4,"idiv q[sp+8],q[sp+10]   ",          vm_handle_10a33},
	{0x10b00,4,"shr d[sp],b[sp+4]   ",           vm_handle_10b00},
	{0x10b01,2,"shr w[sp],b[sp+2]   ",           vm_handle_10b01},
	{0x10b02,1,"shr b[sp],b[sp+2]   ",           vm_handle_10b02},
	{0x10b03,8,"shr q[sp],b[sp+8]   ",           vm_handle_10b03},
	{0x0b10,4,"shrd d[sp],d[sp+4],b[sp+8]   ",  vm_handle_0b10},
	{0x0b11,8,"shrd q[sp],q[sp+8],b[sp+10]  ",  vm_handle_0b11},
	{0x10c00,4,"mul  d[sp+4],d[sp]  ",           vm_handle_10c00},
	{0x10c10,4,"imul d[sp+4],d[sp]  ",           vm_handle_10c10},
	{0x10c23,8,"mul  q[sp+8],q[sp]  ",           vm_handle_10c23},
	{0x10c33,8,"imul  q[sp+8],q[sp] ",           vm_handle_10c33},
	{0x10d00,4,"or ~d[sp+4],~d[sp]  ",           vm_handle_10d00},
	{0x10d01,2,"or ~w[sp+2],~w[sp]  ",           vm_handle_10d01},
	{0x10d02,1,"or ~b[sp+2],~b[sp]  ",           vm_handle_10d02},
	{0x10d03,8,"or ~q[sp+8],~q[sp]  ",           vm_handle_10d03},
	{0x10e00,4,"shl d[sp],b[sp+4]   ",           vm_handle_10e00},
	{0x10e01,2,"shl w[sp],b[sp+2]   ",           vm_handle_10e01},
	{0x10e02,1,"shl b[sp],b[sp+2]   ",           vm_handle_10e02},
	{0x10e03,8,"shl q[sp],b[sp+8]   ",           vm_handle_10e03},
	{0x0e10,4,"shld d[sp],d[sp+4],b[sp+8]   ",  vm_handle_0e10},
	{0x0e11,8,"shld q[sp],q[sp+8],b[sp+10]  ",  vm_handle_0e11},
	{0x0f00,0,"jmp_1  ",                        vm_handle_0f00},
	{0x0f01,0,"jmp  ",                          vm_handle_0f01},
	{0x1000,0,"cpuid               ",           vm_handle_1000},
	{0x1001,0,"rdtsc               ",           vm_handle_1001},
	{0x1100,8,"mov sp,d[sp]        ",           vm_handle_1100},
	{0x1103,8,"mov sp,q[sp]        ",           vm_handle_1103},
	{0x1200,0,"ret   ",                         vm_handle_1200},
	{0x1300,4,"popfd               ",           vm_handle_1300},
	{0x1303,8,"popfq               ",           vm_handle_1303},
	{0x1400,0,"call dreg           ",           vm_handle_1400},
};

#define ST_HANDLE_NUM   sizeof(st_handle_func)/sizeof(ST_HANDLE64)



//int vm_handle(struct XX_INST *xx_inst,struct XX_CONTECT *xx_contect,int num)
int vm_handle(struct XX_HTHREAD64 *x_thread)
{
	char tmp[200];
	char buff1[200];
	char buff2[200];
	int iret=0;
	int iret2 = 0;
	int tmp_size=0;
	int n=0;
	struct XX_INST *xx_inst=0;
	struct XX_CONTEXT64 *xx_context=0;
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
				MessageBox(0, "opmz_data error", "info", MB_OK);
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














