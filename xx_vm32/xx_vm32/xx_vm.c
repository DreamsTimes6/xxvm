#include <windows.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>

//#include "comm_base.h"
#include "Plugin.h"
#include "xx_vm.h"
#include "xx_comm32.h"
#include "xx_opdef3.h"
#include "resource.h"
#include "xxdisasm32.h"
//#include "xx_opmz.h"
#include "analyze_cfg.h"


#pragma comment(lib,"ollydbg.lib")


///////////////////////  菜单宏定义 /////////////////
#define MENU_LOG_INIT          0
#define MENU_ANALYZE_DYNAMIC   1
#define MENU_ANALYZE_STATIC    2
#define MENU_ANALYZE_PAUSE     3
#define MENU_LOG_OPEN          4
#define MENU_HELP              6

#define MENU_OPMZ              8
#define MENU_OPMZ_PAUSE        9
#define MENU_OPMZ_LOG          10
#define MENU_OPMZ_VERSION      11

#define MENU_ANA_CFG           12

////////////////////////////////////////////////////



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


//void print_inst_code(struct XX_INST*);
//void print_inst_exist(struct XX_INST*);
//void print_inst_mic(struct XX_INST*);
//void print_inst_item(struct XX_INST*);


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


extern int pause_flag;
extern HINSTANCE hinst;


//动态分析管理计数
extern int curt_count;
///////////////////////////////////////////////////////////////////////////////
extern int vm_push_reg(struct XX_INST *xx_inst,int num);
extern int init_vmreg(HANSTANCE);
extern int init_vmsreg(HINSTANCE hinst);
//extern int vm_handle(struct XX_INST *xx_inst,struct XX_CONTEXT *xx_contect,int num);
extern int vm_handle(struct XX_HTHREAD *x_thread);
extern int vm_shandle(unsigned long base,unsigned long ip,char*);

extern int write_file(char *file,char* data,int dsize);
extern int init_file_path(char *ppath,char *pname,char *file);


/////////////////////////////////
//extern void xx_get_user(HWND);
//extern int InitInstance(HWND *);
//extern int InitInstance_user(HWND *phwnd);


extern struct XX_CONTEXT start_context;

//extern int getlicensedate(unsigned int *iyear,unsigned int *imonth,unsigned int *iday);

extern int init_mem_file();
extern int xx_execute(struct XX_INST *inst,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context);
extern int xx_start_saly();
extern void xx_saly_stop();
//extern int xx_cfg();
//extern int  xx_cfg_init(char *xdbg_path);
extern void dbg_cmd_ana_static();
extern int dbg_cmd_ana_dynamic();
extern int is_inst_ret();
extern int func_op_vmem();
////////////////////////////////////////////////////////////////////////////////

struct XX_DISASM_RECORD
{
	unsigned int ip;
	char  cmd_hex[MAXCMDSIZE];
	int   cmd_len;
	char  cmd_mic[TEXTLEN];
	ulong r[8];
};

/*使用到的文件名*/
char inst_file_name[] = "vm_opmz.x";
char inst_dfile_name[] = "vm_opmz_data.x";
char opmz_log_name[] = "xx_opmz.txt";
char exp_file_name[] = "xx_exp.x";


char log_file[0x400];  //普通日志文件名  
char opmz_cmd[0x400];  
char opmz_log[0x400];  //优化日志文件名
char opmz_file[0x400];  //优化指令信息
char opmz_dfile[0x400];  //优化指令数据
char exp_file[0x400];    //优化还原数据
char  xdbg_path[0x400];


int g_inst_num;


HANDLE hprocess;  //进程句柄

int fix_flag=0;
int total_record=0;
//int single_flag=0;

int shandle_flag=0;
int call_flag=0;
HANDLE hGlobalWriteEvent; 

int g_unknow_handler;

/*变量,待修改到ollydbg.ini配置文件中*/
int trace_maxn=200;

char xx_main_menu[0x200];

HANDLE hthread_analyze;
HANDLE hthread_opmz;

///////////////////////////////////////////////////////////////////////////////

void xx_free();
void xx_init();
void xx_init_reg();
unsigned int  xx_trace(t_thread *st_thread);
void xx_vm();
int xx_svm();
int isexit(ulong ip);
int xx_record(t_reg *reg);

int xx_start_aly();
int xx_write_log();
int xx_write_log2();

int single_disasm();
int single_info();
int xreg_cmp(char* buf1,char* buf2);
int xx_cmd_filter(struct XX_INST* xx_inst);

int xx_get_xdbg_context(struct XX_CONTEXT *xx_context);
void  get_cpu_thread_status();
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// Report plugin name and return version of plugin interface.
extc int _export cdecl ODBG_Plugindata(char shortname[32]) {
  strcpy(shortname,"xx_vm");    // Name of command line plugin
  return PLUGIN_VERSION;
};

//OD加载插件初始化
extc int _export cdecl ODBG_Plugininit(int ollydbgversion,HWND hw,ulong *features)
{
	
	int n=0;
	int iret;
	HINSTANCE hxx_comm=0;
	char tmp[1000];
	int menuid=0;

	char *p=0;
	
	if (ollydbgversion<PLUGIN_VERSION)
        return -1;

	//_Addtolist(0,1,xx_version);
	_Addtolist(0,1,"xx_vm  http://www.microcode-tec.com");


	menuid=IDS_STRING2;

	hprocess=0;
	total_record=0;
	fix_flag=0;
	//single_flag=0;
	
	memset(xx_main_menu,0,sizeof(xx_main_menu));

	memset(tmp,0,sizeof(tmp));
	iret=LoadString(hinst,menuid,tmp,sizeof(tmp));
	if(iret!=0)
	{
		memcpy(xx_main_menu,tmp,strlen(tmp));
	}

	

	memcpy(vm_key,xreg_ebx,strlen(xreg_ebx));
	memcpy(vm_regbase,xreg_esp,strlen(xreg_esp));

	memset(log_file, 0, 0x400);
	memset(opmz_file, 0, 0x400);
	memset(opmz_dfile, 0, 0x400);
	memset(opmz_log, 0, 0x400);
	memset(exp_file, 0, 0x400);

	memset(xdbg_path, 0, sizeof(xdbg_path));
	iret = GetCurrentDirectory(sizeof(xdbg_path), xdbg_path);
	if (iret == 0)
	{
		MessageBox(0, "GetCurrentDirectory error", "error", MB_OK);
		return 0;
	}

	p=strstr(xdbg_path, "\\plugin");
	if (p)
	{
		memset(p, 0, strlen(p));
	}

	/*od目录*/
	strcat(xdbg_path,"\\");


	/*初始化分析配置文件*/
	xx_cfg_init(xdbg_path);

	/*自动登陆*/;

	/*授权*/

	iret = func_op_vmem();
	if (iret == 0)
	{
		MessageBox(0, "func_op_vmem", "error", MB_OK);
	}

	return 0;
};


extc void _export cdecl ODBG_Pluginreset()
{

	hprocess=0;
	total_record=0;
	fix_flag=0;
	//single_flag=0;

	memset(log_file, 0, sizeof(log_file));
	memset(opmz_file, 0, sizeof(opmz_file));
	memset(opmz_dfile, 0, sizeof(opmz_dfile));
	memset(opmz_log, 0, sizeof(opmz_log));
	memset(exp_file, 0, sizeof(exp_file));


	memset(vm_ip,0,strlen(vm_ip));
	memset(vm_sp,0,strlen(vm_sp));
	memset(vm_opcode,0,strlen(vm_opcode));

	memset(vm_oip,0,strlen(vm_oip));
	memset(vm_osp,0,strlen(vm_osp));
	memset(vm_oopcode,0,strlen(vm_oopcode));

	
	return;
};


//回调函数,OD调用,绘制菜单
extc int _export cdecl ODBG_Pluginmenu(int origin,char data[4096],void *item)
{
	switch(origin)
	{
	case PM_MAIN:
		memcpy(data,xx_main_menu,strlen(xx_main_menu)+1);
		
		break;
	case PM_DISASM:
		
		break;
	default:
		return 0;
	}

	return 1;	
};


//菜单动作响应
extc void _export cdecl ODBG_Pluginaction(int origin,int action,void *item)
{
	int iret=0;
	HWND hwnd=0;
	int log_flag = 0;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	//首先判断是哪个菜单
	switch(origin)
	{
	case PM_MAIN:
		//再判断是哪个菜单项
		switch(action)
		{
		case MENU_LOG_INIT:
			/*初始化日志*/
			iret = xx_run_status();
			if (iret == 0)
			{
				MessageBox(0, "xx_vm is running,please wait", "ERROR", MB_OK);
			}
			else
			{
				xx_init();
			}
			break;
		case MENU_ANALYZE_DYNAMIC:
			/*动态开始分析*/
			iret = xx_run_status();
			if (iret == 0)
			{
				MessageBox(0, "xx_vm is running,please wait", "ERROR", MB_OK);
			}
			else
			{
				curt_count=1;
				iret=xx_start_aly();
				if(iret==0)
				{
					_Addtolist(0,1,"log error. init log");
				}
			}
			break;
		case MENU_ANALYZE_STATIC:
			//xx_start_saly();
			/*静态*/
			iret = xx_run_status();
			if (iret == 0)
			{
				MessageBox(0, "xx_vm is running,please wait", "ERROR", MB_OK);
			}
			else
			{
				
				iret=xx_get_xdbg_context(&start_context);
				if(iret==0)
				{
					MessageBox(0, "context error", "ERROR", MB_OK);
					break;
				}

				hthread_analyze=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)dbg_cmd_ana_static,0,0,0);
			}
			break;
		case MENU_ANALYZE_PAUSE:
			xx_saly_stop();
			break;
		case MENU_LOG_OPEN:
			/*log*/
			log_flag = 0;
			iret = xx_cfg_get_int(4, &log_flag);
			if (iret == 0)
			{
				MessageBox(0, "error:xx_cfg.ini ", "error", MB_OK);
			}
			else
			{
				if (log_flag == 0)
				{
					ShellExecute(0, "edit", log_file, 0, 0, SW_SHOW);
				}
				else
				{
					ShellExecute(0, "open", log_file, 0, 0, SW_SHOW);
				}
			}
			break;
		case MENU_ANA_CFG:
			/*cfg*/
			iret=xx_cfg();
			if (iret == 0)
			{
				MessageBox(0, "cfg error", "ERROR", MB_OK);
			}
			break;
		case MENU_HELP:
			ShellExecute(0,"open","xx_vm.chm",0,0,SW_SHOW);
			break;
		case MENU_OPMZ:
			iret = xx_run_status();
			if (iret == 0)
			{
				MessageBox(0, "xx_vm is running,please wait", "ERROR", MB_OK);
			}
			else
			{
				memset(&si, 0, sizeof(si));
				memset(&pi, 0, sizeof(pi));
				si.cb = sizeof(si);
				//MessageBox(0, opmz_cmd, "1111", MB_OK);
				CreateProcess(0, opmz_cmd, 0, 0, 0, 0, 0, 0, &si, &pi);
			}
			break;
		case MENU_OPMZ_PAUSE:
			//xx_opmz_stop();
			MessageBox(0, "not used", "nothing", MB_OK);
			break;
		case MENU_OPMZ_LOG:
			log_flag = 0;
			iret = xx_cfg_get_int(4, &log_flag);
			if (iret == 0)
			{
				MessageBox(0, "error:xx_cfg.ini ", "error", MB_OK);
			}
			else
			{
				if (log_flag == 0)
				{
					ShellExecute(0, "edit", opmz_log, 0, 0, SW_SHOW);
				}
				else
				{
					ShellExecute(0, "open", opmz_log, 0, 0, SW_SHOW);
				}
			}
			break;
		case MENU_OPMZ_VERSION:
			iret = xx_run_status();
			if (iret == 0)
			{
				MessageBox(0, "xx_vm is running,please wait", "ERROR", MB_OK);
			}
			break;
		}
		break;
	}
	return;
};



/*运行状态检查*/
int xx_run_status()
{
	int iret = 0;
	unsigned long status = 0;


	if (hthread_analyze != 0 )
	{
		status = 0;
		iret = GetExitCodeThread(hthread_analyze, &status);
		if (iret == 0)
		{
			return 0;
		}
		if (status == STILL_ACTIVE)
		{
			return 0;
		}
	}

	if (hthread_opmz != 0)
	{
		status = 0;
		iret = GetExitCodeThread(hthread_opmz, &status);
		if (iret == 0)
		{
			return 0;
		}
		if (status == STILL_ACTIVE)
		{
			return 0;
		}
	}

	return 1;
}



int xx_start_aly()
{
	int n=0;
	int iret=0;
	char sret[200];
	
	char tmp[1000];



	if(strlen(log_file)==0)
	{
		MessageBox(0, "Init first", "info", MB_OK);
		return 0;
	}

	memset(vm_ip,0,strlen(vm_ip));
	memset(vm_sp,0,strlen(vm_sp));
	memset(vm_opcode,0,strlen(vm_opcode));


	memset(vm_oip,0,strlen(vm_oip));
	memset(vm_osp,0,strlen(vm_osp));
	memset(vm_oopcode,0,strlen(vm_oopcode));


	g_inst_num=0;


	iret=xx_svm();
	if(iret==-2)
	{
		MessageBox(0,"EIP can not stop the 'retn',or were not the Virtual-code","error",MB_OK);
		fix_flag=0;
		return 0;
	}
	shandle_flag=iret;
	
	_Settracecount(trace_maxn);
	
	memset(tmp,0,sizeof(tmp));
	xx_get_reg_mic(vm_ip,tmp);
	
	memset(sret,0,sizeof(sret));
	sprintf(sret,"eip==%s",tmp);

	_Settracecondition(sret,0,0,0,0,0);
	
	fix_flag=1;
	
	
	
#ifdef DEBUG_LOG
	_Addtolist(0,1,"vm_ip:[%s]",vm_ip);
	_Addtolist(0,1,"vm_sp:[%s]",vm_sp);
	_Addtolist(0,1,"vm_regbase:[%s]",vm_regbase);
	_Addtolist(0,1,"vm_key:[%s]",vm_key);
	_Addtolist(0,1,"vm_opcode:[%s]",vm_opcode);
#endif
	xx_vm();

	return 1;
}


int xx_svm()
{
	unsigned long base=0;
	int iret=0;
	int threadid;
	t_thread *pt_thread=0;
	char *plogdata=0;
	char tmp[0x200];


	/*获取当前线程id*/
	threadid=_Getcputhreadid();
	if(threadid==0)
	{
		_Addtolist(0,1,"Getcputhreadid error");
		return -1;
	}
	
	/*获取线程结构t_thread*/
	pt_thread=_Findthread(threadid);
	if(pt_thread==0)
	{
		_Addtolist(0,1,"_Findthread error");
		return -1;
	}

	hprocess=(HANDLE)_Plugingetvalue(VAL_HPROCESS);
	//_Addtolist(0,1,"hprocess:[%08X]",hprocess);
	if(hprocess==0)
	{
		_Addtolist(0,1,"_Plugingetvalue VAL_HPROCESS error");
		return -1;
	}

#if 0
	iret=init_vmsreg(hinst);
	if(iret==-1)
	{
		_Addtolist(0,1,"init_vmreg error");
		return -1;
	}
#endif
	
	base=_Plugingetvalue(VAL_MAINBASE);


	memset(tmp,0,sizeof(tmp));
	plogdata=tmp;
 

	iret=vm_shandle(base,pt_thread->reg.ip,plogdata);
	if(iret==-1)
	{
		return -1;
	}
	else if(iret==-2)
	{
		return -2;
	}
	xx_append_file(log_file,plogdata,strlen(plogdata));
	return iret;
}


void xx_vm()
{
	int threadid;
	t_thread *pt_thread=0;


	/*获取当前线程id*/
	threadid=_Getcputhreadid();
	if(threadid==0)
	{
		_Addtolist(0,1,"Getcputhreadid error");
		return ;
	}
	
	/*获取线程结构t_thread*/
	pt_thread=_Findthread(threadid);
	if(pt_thread==0)
	{
		_Addtolist(0,1,"_Findthread error");
		return ;
	}
	
	fix_flag=1;

	_Startruntrace(&pt_thread->reg);

	xx_trace(pt_thread);
	return;
};

unsigned int  xx_trace(t_thread *st_thread)
{
	
	int iret=0;
	

	if(call_flag==1)
	{
		_Animate(ANIMATE_TROVER);
		_Go(st_thread->threadid,0,STEP_IN,0,0); 
	}
	else if(call_flag==0)
	{
		_Animate(ANIMATE_TRIN);
		_Go(st_thread->threadid,0,STEP_IN,0,0);  
	}
	else
	{
		_Addtolist(0,1,"_Findthread error");
	}

	return 0;
	
};

/*trace结束，开始记录，分析，重写指令*/
int xx_record(t_reg *reg)
{
	int n=0;
	int m=0;
	//t_reg lt_reg;  //上一个状态
	t_reg nt_reg;  //下一个状态
	int trace_num=0;
	unsigned char cmdbuf[MAXCMDSIZE];
	int cmdlen=0;
	struct XX_INST xx_inst;
	struct XX_INST *pxx_inst;
	unsigned long base=0;
	int iret=0;
	int inst_num=0;
	struct XX_CONTEXT *xx_context;
	char tmp[0x200];
	char sret[200];
	int tmpip=0;
	struct XX_HTHREAD x_thread;

	//_Addtolist(0,1,"shandle_flag:[%d] ",shandle_flag);
	
	if(shandle_flag==-1)
	{
		/*trace结束后，当前条件暂停指令为最近一条*/
		trace_num=_Runtracesize();
		//_Addtolist(0,1,"trace_num:[%d]",trace_num);
		if(trace_num==0)
		{
			_Addtolist(0,1,"_Runtracesize error");
			return -1;
		}
		if(trace_num>=trace_maxn)
		{
			trace_num=100;
		}
		
		trace_num=trace_num-1;
		total_record=total_record+trace_num;
		//disasm_record=(struct XX_DISASM_RECORD*)malloc(sizeof(struct XX_DISASM_RECORD)*trace_num);
		//memset(disasm_record,0,sizeof(struct XX_DISASM_RECORD)*trace_num);
		
		xx_context=(struct XX_CONTEXT*)malloc(sizeof(struct XX_CONTEXT)*trace_num);
		memset(xx_context,0,sizeof(struct XX_CONTEXT)*trace_num);

		pxx_inst=malloc(sizeof(struct XX_INST)*trace_num);
		memset(pxx_inst,0,sizeof(struct XX_INST)*trace_num);

		inst_num=0;
		
		
		m=0;
		for(n=trace_num;n>0;n--)
		{
			//_Addtolist(0,1,"n:[%d]",n);
			memset(&nt_reg,0,sizeof(nt_reg));
			iret=_Getruntraceregisters(n,&nt_reg,NULL,NULL,NULL);
			if(iret==-1)
			{
				_Addtolist(0,1,"Getruntraceregisters ret:[%d] error",iret);
				//return -1;
			}

			/*反汇编此处代码*/
			memset(cmdbuf,0,sizeof(cmdbuf));
			iret=_Readmemory((char*)cmdbuf,nt_reg.ip,MAXCMDSIZE,MM_RESTORE);
			if(iret!=MAXCMDSIZE)
			{
				_Addtolist(0,1,"ReadCommand error ip:[%08x]",nt_reg.ip);
				return -1;
			}
			
			memset(&xx_inst,0,sizeof(xx_inst));
			base=_Plugingetvalue(VAL_MAINBASE);
			xx_inst.xx_inst_code.inst_system = INST_SYSTEM_CURT;
			memcpy(xx_inst.xx_inst_code.base, (char*)&base, xx_inst.xx_inst_code.inst_system);
			memcpy(xx_inst.xx_inst_code.current_addr, (char*)&nt_reg.ip, xx_inst.xx_inst_code.inst_system);
			xx_disasm(&xx_inst,cmdbuf);
			if(xx_inst.xx_inst_exist.finish_flag==INST_FLAG_EXIST)
			{
				memcpy(&pxx_inst[inst_num],&xx_inst,sizeof(struct XX_INST));
				
				memcpy(xx_context[inst_num].r,nt_reg.r,sizeof(nt_reg.r));
				inst_num++;
			}
			else
			{
				_Addtolist(0,1,"xx_disasm error ip:[%08x]",nt_reg.ip);
			}

			
			m++;
		}
#ifdef DEBUG_LOG
		//_Addtolist(0,1,"*****vm_handle*******");
		memset(tmp,0,sizeof(tmp));
		xx_get_reg_mic(vm_ip,tmp);
		_Addtolist(0,1,"vm_ip:[%s][%s]",vm_ip,tmp);
		memset(tmp,0,sizeof(tmp));
		xx_get_reg_mic(vm_sp,tmp);
		_Addtolist(0,1,"vm_sp:[%s][%s]",vm_sp,tmp);
		memset(tmp,0,sizeof(tmp));
		xx_get_reg_mic(vm_regbase,tmp);
		_Addtolist(0,1,"vm_regbase:[%s][%s]",vm_regbase,tmp);
		memset(tmp,0,sizeof(tmp));
		xx_get_reg_mic(vm_key,tmp);
		_Addtolist(0,1,"vm_key:[%s][%s]",vm_key,tmp);
		memset(tmp,0,sizeof(tmp));
		xx_get_reg_mic(vm_opcode,tmp);
		_Addtolist(0,1,"vm_opcode:[%s][%s]",vm_opcode,tmp);
#endif

		//iret=vm_handle(pxx_inst,xx_context,inst_num);
		memset(&x_thread,0,sizeof(x_thread));
		x_thread.xx_inst=pxx_inst;
		x_thread.xx_context=xx_context;
		x_thread.inst_num=inst_num;
		memset(tmp,0,sizeof(tmp));
		x_thread.plogdata=tmp;
		iret=vm_handle(&x_thread);
		if(iret==-1)
		{
			_Addtolist(0,1,"vm_handle error invalid");
			g_unknow_handler++;
			//return -1;
		}
		else
		{
			g_unknow_handler=0;
		}

		if(g_unknow_handler>=3)
		{
			_Addtolist(0,1,"much more invalid handler");
			return -1;
		}
		xx_append_file(log_file,x_thread.plogdata,strlen(x_thread.plogdata));

		free(xx_context);
		//free(disasm_record);
		free(pxx_inst);
		
		_Startruntrace(reg);
	}
	//_Addtolist(0,1,"*****vm_shandle*******");
	iret=xx_svm();
	if(iret==-2)
	{
		//MessageBox(0,"ip不能停在ret指令上，或者非vm代码","error",MB_OK);
		MessageBox(0,"EIP can not stop the 'retn',or were not the Virtual-code","error",MB_OK);
		fix_flag=0;
		return -1;
	}
	shandle_flag=iret;
	//_Addtolist(0,1,"shandle_flag:[%d]",shandle_flag);

	
	memset(tmp,0,sizeof(tmp));
	xx_get_reg_mic(vm_ip,tmp);
	memset(sret,0,sizeof(sret));
	sprintf(sret,"eip==%s",tmp);
	//_Addtolist(0,1,"vm_piece_cnd:[%s]",sret);

	_Settracecondition(sret,0,0,0,0,0);
	
	return 0;
}


void xx_init()
{
	
	char *pname;
	int iret = 0;

	char sret[200];
	char tmp[200];
	char dsret[] = "xxxx";


	

	if (strlen(xdbg_path) == 0)
	{
		MessageBox(0,"dbgpath error","info",MB_OK);
		return ;
	}

	memset(log_file, 0, sizeof(log_file));

	pname=(char *)_Plugingetvalue(VAL_PROCESSNAME);


	iret = init_file_path(xdbg_path, pname, log_file);
	if (iret)
	{
		memset(log_file, 0, sizeof(log_file));
		MessageBox(0,"log_file error","info",MB_OK);
		return ;
	}

	memset(opmz_file, 0, sizeof(opmz_file));
	memcpy(opmz_file, log_file, strlen(log_file));

	memset(opmz_dfile, 0, sizeof(opmz_dfile));
	memcpy(opmz_dfile, log_file, strlen(log_file)); 

	memset(opmz_log, 0, sizeof(opmz_log));
	memcpy(opmz_log, log_file, strlen(log_file));

	memset(opmz_cmd, 0, sizeof(opmz_cmd));
	sprintf(opmz_cmd, "xx_opmz.exe %s", log_file);

	memset(exp_file, 0, sizeof(exp_file));
	memcpy(exp_file, xdbg_path, strlen(xdbg_path));
	/*创建目录*/
	iret = CreateDirectory(log_file, NULL);
	
	/*初始化日志文件名*/
	strcat(log_file, pname);
	strcat(log_file, ".txt");
	//MessageBox(0, log_file, "success", MB_OK);

	/*初始化优化文件名opmz_file*/
	strcat(opmz_file, inst_file_name);
	//MessageBox(0, opmz_file, "success", MB_OK);
	strcat(opmz_dfile, inst_dfile_name);

	strcat(opmz_log, opmz_log_name);

	strcat(exp_file, exp_file_name);

	MessageBox(0, log_file, "success", MB_OK);
	return ;
};

void xx_free()
{
	int iret=0;
	fix_flag=0;

};

extc int _export cdecl ODBG_Pausedex(int reason, int extdata, t_reg *reg, DEBUG_EVENT *debugevent)
{
	int iret=0;
	int threadid=0;

	if(fix_flag==1 )
	{
		_Animate(ANIMATE_OFF);
		//_Addtolist(0,1,"trace_num:[%d]",trace_num);
		switch(reason)
		{
		case PP_SINGLESTEP:
			//_Addtolist(0,1,"fix_flag:[%0d]",fix_flag);

			iret=xx_record(reg);
			if(shandle_flag==0x1200)
			{
				_Addtolist(0,1,"vm_ret");
				fix_flag=2;
				_Sendshortcut(PM_MAIN,0,WM_KEYDOWN,1,0,VK_F9);
				return 0;
			}
			if(reg->r[get_reg_index(vm_ip)]==reg->ip || reg->r[get_reg_index(vm_oip)]==reg->ip)
			{
				if(iret==0)
				{
					xx_vm();
					return 1;
				}
				else
				{
					fix_flag=0;
					return 0;
				}
			}

			fix_flag=0;
			break;
		case PP_HWBREAK:
			fix_flag=0;
			break;
		case PP_INT3BREAK:
			fix_flag=0;
			break;
		case (PP_SINGLESTEP | PP_PAUSE):
			fix_flag=0;
			break;
		}
	}
	else if(fix_flag==2 )
	{
		_Animate(ANIMATE_OFF);
		switch(reason)
		{
		case PP_SINGLESTEP:
			//_Addtolist(0,1,"PP_SINGLESTEP");
			/*说明从vm_ret出来，检查是否是ret指令*/
			iret=is_inst_ret();
			if(iret==1)
			{
				_Addtolist(0,1,"ret");
				fix_flag=3;
				_Sendshortcut(PM_MAIN,0,WM_KEYDOWN,0,0,VK_F7);
			}
			else
			{
				_Addtolist(0,1,"not ret");
				fix_flag=0;
			}
			break;
		case PP_HWBREAK:
			//_Addtolist(0,1,"PP_HWBREAK");
			fix_flag=0;
			break;
		case PP_INT3BREAK:
			//_Addtolist(0,1,"PP_INT3BREAK");
			fix_flag=0;
			break;
		case (PP_SINGLESTEP | PP_PAUSE):
			//_Addtolist(0,1,"PP_SINGLESTEP | PP_PAUSE");
			fix_flag=0;
			break;
		}
	}
	else if(fix_flag==3 )
	{
		_Animate(ANIMATE_OFF);
		switch(reason)
		{
		case PP_SINGLESTEP:
			//_Addtolist(0,1,"PP_SINGLESTEP");
			/*说明从vm_ret出来*/
			/*调用动态分析管理*/
			iret=dbg_cmd_ana_dynamic();
			if(iret==0)
			{
				fix_flag=0;
			}
			else
			{
				memset(&start_context,0,sizeof(struct XX_CONTEXT));
				xx_start_aly();
				return 1;
			}
			//curt_count=0;
			//fix_flag=0;
			break;
		case PP_HWBREAK:
			//_Addtolist(0,1,"PP_HWBREAK");
			fix_flag=0;
			break;
		case PP_INT3BREAK:
			//_Addtolist(0,1,"PP_INT3BREAK");
			fix_flag=0;
			break;
		case (PP_SINGLESTEP | PP_PAUSE):
			//_Addtolist(0,1,"PP_SINGLESTEP | PP_PAUSE");
			fix_flag=0;
			break;
		}
	}
	return 0;
};


int xx_get_xdbg_context(struct XX_CONTEXT *xx_context)
{
	int threadid;
	t_thread *pt_thread=0;
	int iret=0;

	/*获取当前线程id*/
	threadid=_Getcputhreadid();
	if(threadid==0)
	{
		return 0;
	}

	/*获取线程结构t_thread*/
	pt_thread=_Findthread(threadid);
	if(pt_thread==0)
	{
		return 0;
	}

	memset(xx_context,0,sizeof(struct XX_CONTEXT));

	memcpy(xx_context->r,pt_thread->reg.r,sizeof(pt_thread->reg.r));

	xx_context->eflag=pt_thread->reg.flags;
	xx_context->ip=pt_thread->reg.ip;

	return 1;
}


void  get_cpu_thread_status()
{
	int threadid;
	t_thread *pt_thread=0;
	int iret=0;
	int status=0;
	int n=0;

	/*获取当前线程id*/
	threadid=_Getcputhreadid();
	if(threadid==0)
	{
		return ;
	}

	/*获取线程结构t_thread*/
	pt_thread=_Findthread(threadid);
	if(pt_thread==0)
	{
		return ;
	}

	while(n<=10000)
	{
		status = 0;
		iret = GetExitCodeThread(pt_thread->thread, &status);
		if (iret == 0)
		{
			_Addtolist(0,1,"status:error ");
		}

		if (status == STILL_ACTIVE)
		{
			_Addtolist(0,1,"status:STILL_ACTIVE");
		}
		else
		{
			_Addtolist(0,1,"status:[%d]",status);
		}
		
		Sleep(500);
		n++;
	}
	return ;
}


