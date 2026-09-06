#include <windows.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>

//#include "comm_base.h"
#include "Plugin.h"
#include "xx_def.h"
#include "xx_opdef3.h"
#include "resource.h"
#include "xx_comm32.h"
#include "xxdisasm32.h"
#include "xx_iat.h"
#include "analyze_cfg.h"


#pragma comment(lib,"ollydbg.lib")

/////////////////////////////////////////////





///////////////////////////////////////////////

struct T_VM_IAT
{
	ulong ip;     //vmcall ip
	int fix_type;  //vmcall type 1.call [iat]  2.mov reg,[iat] 3.jmp [iat]
	ulong fix_ip;  //vmcall fix ip
	ulong iat;     //api addr
	int iat_flag;  //0-invalid  1-valid
	//ulong ret;     //
	int ret_flag;  //1.retn  2.retn 4  3.retn 8
	char iat_sym[TEXTLEN]; //
	int reg_index;  //mov reg,[iat]  reg index
	ulong iat_addr; //
};
struct T_VM_IAT *vm_iat=0;
int iat_num=0;

char xx_main_menu[0x200];

HINSTANCE hinst=0;

char log_file[0x400]={0};
char  xdbg_path[0x400];


ulong iat_base=0;


	unsigned int code_start=0;
	unsigned int code_end=0;
	unsigned int vmp0_start=0;
	unsigned int vmp0_end=0;
///////////////////////////////////////////////

int search_vmcall(ulong codestart,ulong codeend,ulong vmp0start,ulong vmp0end,struct T_VM_IAT*);
int xx_vm_iat();
int search_vmcall_2x(ulong codestart,ulong codeend,ulong vmp0start,ulong vmp0end,struct T_VM_IAT*);
int xx_vm_iat_2x();

ulong get_mem_value(ulong addr);
int judge_vmcall_type(struct XX_CONTEXT *old_reg,struct XX_CONTEXT *curt_reg,int );
void print_sig_iat(int n);
void print_iat();
void print_invalid_iat();
void replace_invalid_iat();
void replace_all_invalid_iat();
int fix_vmcall();
int log_vmcall();
int notes_vmcall();


int xreg_incmp(char* reg,char* sub);
int xreg_cmp(char* buf1,char* buf2);
int is_inst_ppr(ulong  ip);

//////////////////////////////////////////////////////////////////////////////

extern unsigned int seg_fs;
//////////////////////////////////////////////////////////
extern int init_file_path(char *ppath,char *pname,char *file);

extern int   cfg_win_create();
extern char xx_version[];

extern int xx_start_execute(struct XX_CONTEXT *init_context,struct XX_CONTEXT *out_context,int *out_type);


/*函数，取内存数据*/
extern int xx_read_fmem(unsigned int addr,char* pdata,unsigned int datasize);
extern int func_op_vmem();
///////////////////////////////////////////////////////////////////////////////
int __stdcall DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			hinst=(HINSTANCE)hModule;
			//dllhinst=GetModuleHandle(DLL_NAME);
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
};


// Report plugin name and return version of plugin interface.
extc int _export cdecl ODBG_Plugindata(char shortname[32]) {
  strcpy(shortname,"xx_vm_iat");    // Name of command line plugin
  return PLUGIN_VERSION;
};

//OD加载插件初始化
extc int _export cdecl ODBG_Plugininit(int ollydbgversion,HWND hw,ulong *features)
{
	
	
	int iret;
	HINSTANCE hxx_comm=0;
	char tmp[1000];
	int menuid=0;
	char *p=0;
	
	if (ollydbgversion<PLUGIN_VERSION)
        return -1;

	//_Addtolist(0,1,xx_version);
	_Addtolist(0,1,"xx_vm  http://xxdisasm.com");

	menuid=IDS_STRING2;
	
	memset(xx_main_menu,0,sizeof(xx_main_menu));

	memset(tmp,0,sizeof(tmp));
	iret=LoadString(hinst,menuid,tmp,sizeof(tmp));
	if(iret!=0)
	{
		memcpy(xx_main_menu,tmp,strlen(tmp));
	}

	vm_iat=0;
	iat_num=0;
	iat_base=0;

	memset(log_file, 0, 0x400);

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

	/*检查iat数据是否已经存在*/
	if(vm_iat)
	{
		VirtualFree((void*)vm_iat,0,MEM_DECOMMIT);
		iat_num=0;
		vm_iat=0;
		iat_base=0;
	}

	memset(log_file,0,sizeof(log_file));
	return;
};


//回调函数,OD调用,绘制菜单
extc int _export cdecl ODBG_Pluginmenu(int origin,char data[4096],void *item)
{
	switch(origin)
	{
	case PM_MAIN:
		memcpy(data,xx_main_menu,sizeof(xx_main_menu));
		break;
	case PM_DISASM:
		//memcpy(data,xx_disasm_menu,sizeof(xx_disasm_menu));
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
	HANDLE hthread=0;

	//首先判断是哪个菜单
	switch(origin)
	{
	case PM_MAIN:
		//再判断是哪个菜单项
		switch(action)
		{
		case 0:
			if(_Plugingetvalue(VAL_PROCESSNAME))
			{
				//xx_vm_iat();
				CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)xx_vm_iat,0,0,0);
			}
			break;

		case 1:
			if(vm_iat)
			{
				print_iat();
				_Addtolist(0,1,"iat_num:[%08x]",iat_num);
			}
			else
			{
				MessageBox(0,"no iat","error",MB_OK);
			}
			break;
		case 2:
			if(vm_iat)
			{
				print_invalid_iat();
			}
			else
			{
				MessageBox(0,"no iat","error",MB_OK);
			}
			break;
		case 3:
			//reload_invalid_iat();
			if(vm_iat)
			{
				replace_invalid_iat();
			}
			else
			{
				MessageBox(0,"no iat","error",MB_OK);
			}
			break;
		case 4:
			if(vm_iat)
			{
				replace_all_invalid_iat();
			}
			else
			{
				MessageBox(0,"no iat","error",MB_OK);
			}
			break;
		case 5:
			/*修复vmcall*/
			if(vm_iat)
			{
				hthread=0;
				hthread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)fix_vmcall,0,0,0);
				if(hthread!=0)
				{
					SetThreadPriority(hthread,THREAD_PRIORITY_HIGHEST);
				}
			}
			else
			{
				MessageBox(0,"search vm_iat please","error",MB_OK);
			}
			break;
		case 6:
			/*日志*/
			if(vm_iat)
			{
				CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)log_vmcall,0,0,0);
			}
			else
			{
				MessageBox(0,"search vm_iat please","error",MB_OK);
			}
			break;
		case 7:
			/*注释*/
			if(vm_iat)
			{

				CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)notes_vmcall,0,0,0);
			}
			else
			{
				MessageBox(0,"search vm_iat please","error",MB_OK);
			}
			break;
		case 8:
			/*释放空间*/
			if(vm_iat)
			{
				VirtualFree((void*)vm_iat,0,MEM_DECOMMIT);
				iat_num=0;
				vm_iat=0;
			}
			break;
		case 9:
			/*cfg*/
			hwnd=(HWND)_Plugingetvalue(VAL_HWMAIN);
			if(hwnd)
			{
				iret=cfg_win_create(hwnd);
			}
		
			break;
		}
		break;

	}
	
};



extc int _export cdecl ODBG_Pausedex(int reason, int extdata, t_reg *reg, DEBUG_EVENT *debugevent)
{
	int iret=0;
	

	switch(reason)
	{
	case PP_SINGLESTEP:
		break;
	case PP_HWBREAK:
		break;
	case PP_INT3BREAK:
		break;
	case (PP_SINGLESTEP | PP_PAUSE):
		break;
	}

	return 0;
};



int xx_vm_iat()
{
	int threadid;
	t_thread *pt_thread=0;
	int iret=0;
	struct XX_CONTEXT in_context;
	struct XX_CONTEXT out_context;

	char tmp[200];

	int n=0;
	

	if(!code_start || !code_end || !vmp0_start || !vmp0_end)
	{
		MessageBox(0,"Configure,please","error",MB_OK);
		return -1;
	}

	/*暂停状态，防止内存发生变化*/
	_Addtolist(0,1,"code_start:[%08x],code_end:[%08x],vmp0_start:[%08x],vmp0_end:[%08x]",\
		code_start,code_end,vmp0_start,vmp0_end);

	/*检查iat数据是否已经存在*/
	if(vm_iat)
	{
		VirtualFree((void*)vm_iat,0,MEM_DECOMMIT);
		iat_num=0;
		vm_iat=0;
	}

	_Addtolist(0,1,"xx_iat  start");

	/*初始化内存*/
	iret=init_mem_file(xdbg_path);
	if(iret==0)
	{
		MessageBox(0,"init error","error",MB_OK);
		return 0;
	}
	_Addtolist(0,1,"init success");

	n=search_vmcall(code_start,code_end,vmp0_start,vmp0_end,0);
	if(n<=0)
	{
		_Addtolist(0,1,"search_vmcall 0 error");
		return -1;
	}

	VirtualFree((void*)vm_iat,0,MEM_DECOMMIT);
	vm_iat=VirtualAlloc(0,n*(sizeof(struct T_VM_IAT)),MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	memset(vm_iat,0,n*(sizeof(struct T_VM_IAT)));

	iat_num=n;  //global

	n=search_vmcall(code_start,code_end,vmp0_start,vmp0_end,vm_iat);
	if(n<=0)
	{
		_Addtolist(0,1,"search_vmcall 0 error");
		return -1;
	}

	_Addtolist(0,1,"=====search complete=====");	

	/*获取当前线程id*/
	threadid=_Getcputhreadid();
	if(threadid==0)
	{
		_Addtolist(0,1,"Getcputhreadid error");
		return 0;
	}

	/*获取线程结构t_thread*/
	pt_thread=_Findthread(threadid);
	if(pt_thread==0)
	{
		_Addtolist(0,1,"_Findthread error");
		return 0;
	}


	memset(&in_context,0,sizeof(struct XX_CONTEXT));

	memcpy(&in_context.r,pt_thread->reg.r,sizeof(pt_thread->reg.r));
	in_context.eflag=pt_thread->reg.flags;
	in_context.ip=pt_thread->reg.ip;


	seg_fs=pt_thread->datablock;
	if(seg_fs==0)
	{
		_Addtolist(0,1,"seg_fs error");
		return 0;
	}

	_Addtolist(0,1,"start----iat  total num [%08x]----",iat_num);
	

	for(n=0;n<iat_num;n++)
	{
		/*循环获取iat结构数据*/
		in_context.ip=vm_iat[n].ip;

		memset(&out_context,0,sizeof(out_context));
		iret=xx_start_execute(&in_context,&out_context,&vm_iat[n].ret_flag);
		if(iret==0)
		{
			_Addtolist(0,1,"execute error ip:[%08x]  ip:[%08x]",in_context.ip,out_context.ip);
			//return 0;
			vm_iat[n].iat_flag=0;
			vm_iat[n].fix_type=0;
			continue;
		}
#ifdef DEBUG_LOG
		print_sig_iat(n);
		_Addtolist(0,1,"in_context [%08x] [%08x] [%08x] [%08x] [%08x] [%08x] [%08x] [%08x]",\
			in_context.r[REG_EAX],in_context.r[REG_ECX],in_context.r[REG_EDX],in_context.r[REG_EBX],\
			in_context.r[REG_ESP],in_context.r[REG_EBP],in_context.r[REG_ESI],in_context.r[REG_EDI]);
		_Addtolist(0,1,"out_context [%08x] [%08x] [%08x] [%08x] [%08x] [%08x] [%08x] [%08x]",\
			out_context.r[REG_EAX],out_context.r[REG_ECX],out_context.r[REG_EDX],out_context.r[REG_EBX],\
			out_context.r[REG_ESP],out_context.r[REG_EBP],out_context.r[REG_ESI],out_context.r[REG_EDI]);
#endif

		iret=judge_vmcall_type(&in_context,&out_context,n);
		if(iret==0)
		{
			_Addtolist(0,1,"vmcall_type error ip:[%08x]  ip:[%08x]",in_context.ip,out_context.ip);
			vm_iat[n].iat_flag=0;
			vm_iat[n].fix_type=0;
			//return 0;
		}
		else
		{
			_Addtolist(0,1,"[%08x] ip:[%08x]--get iat data",n,in_context.ip);
		}
	}

	_Addtolist(0,1,"finish----iat  total num [%08x]----",iat_num);
	return 1;
}


int search_vmcall(ulong codestart,ulong codeend,ulong vmp0start,ulong vmp0end,struct T_VM_IAT* t_vm_iat)
{
	/*搜索所有的vmcall*/
	char tmp[200];
	char cmdbuf[200];

	int iret=0;

	ulong n=0;
	ulong ip=0;
	struct XX_INST xx_inst;
	int num=0;
	int cmdlen=0;       //cmd长度
	ulong base=0;       //模块基地址
	ulong des_addr=0;  //vmcall目的地址

	base=_Plugingetvalue(VAL_MAINBASE);

	n=codestart;
	num=0;
	for(n;n<codeend;n++)
	{
		//xx_inst=malloc(sizeof(struct XX_INST)*100);
		memset(&xx_inst,0,sizeof(struct XX_INST));
		
		ip=n;
		//_Addtolist(0,1,"ip:[%08x]",ip);

		memset(cmdbuf,0,sizeof(cmdbuf));
		cmdlen=_Readcommand(ip,(char*)cmdbuf);
		if(cmdlen==0)
		{
			_Addtolist(0,1,"ReadCommand error");
			return -1;
		}
		
		if(*cmdbuf!=0xe8)
		{
			continue;
		}

		xx_inst.xx_inst_code.inst_system = INST_SYSTEM_CURT;
		memcpy(xx_inst.xx_inst_code.base, (char*)&base, xx_inst.xx_inst_code.inst_system);
		memcpy(xx_inst.xx_inst_code.current_addr, (char*)&ip, xx_inst.xx_inst_code.inst_system);

		xx_disasm(&xx_inst,cmdbuf);
		//_Addtolist(0,1,"finish_flag:[%d]",xx_inst[n].xx_inst_exist.finish_flag);
		
		if(xx_inst.xx_inst_exist.finish_flag!=INST_FLAG_EXIST)
		{
			_Addtolist(0,1,"xx_disasm error ip:[%08x]",ip);
			continue;
		}
		//_Addtolist(0,1,"xx_disasm  ip:[%08x] [%s]",ip,xx_inst[n].xx_inst_mic.disasm);
		
		if(memcmp(xx_inst.xx_inst_code.opcode_type,sop_call,sop_len)==0 &&
			xx_inst.xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_DIS)
		{
			des_addr=*(int*)xx_inst.xx_inst_items.xx_inst_items_var[0].item_const;
			/*目的地址在vmp0段,也有可能在text段*/
			if(des_addr>=codestart && des_addr<vmp0end)
			{
				memset(tmp,0,sizeof(tmp));
				cmdlen=_Readcommand(des_addr,(char*)tmp);
				if(cmdlen==0)
				{
					_Addtolist(0,1,"ReadCommand error");
					return -1;
				}
				if(tmp[0]==0x90 )
				{
					memset(tmp,0,sizeof(tmp));
					cmdlen=_Readcommand(des_addr+1,(char*)tmp);
					if(cmdlen==0)
					{
						_Addtolist(0,1,"ReadCommand error");
						return -1;
					}
					if(tmp[1]!=0x90)
					{
						/*确定是vmcall*/
						if(t_vm_iat!=0)
						{
							_Addtolist(0,1,"=======vmcall [%08x][%s]",ip,xx_inst.xx_inst_mic.disasm);
							t_vm_iat[num].ip=ip;
						}
						num++;
					}
				}
			}
		}
	}
	
	_Addtolist(0,1,"search complete num:[%08x][%0d]",num,num);
	return num;
} 









/*判断vmcall类型*/
int judge_vmcall_type(struct XX_CONTEXT *old_reg,struct XX_CONTEXT *curt_reg,int curt_index)
{
	ulong callret=0;
	ulong iret=0;
	//ulong preip=0;

	int n=0;
	int m=0;
	int chgnum=0;
	ulong ret_var=0;

	//int pre_flag=0;
	//int next_flag=0;
	

	callret=vm_iat[curt_index].ip+5;
	//preip=vm_iat[curt_index].ip-1;

	vm_iat[curt_index].fix_ip=0;
	

	//pre_flag=is_inst_ppr(preip);

	//next_flag=is_inst_ppr(callret);


	

	/*2.mov reg,[iat] pop;mov reg,[iat]   [sp]=callret curt_sp=sp-8   retn
					  push;mov reg,[iat]  [sp]=callret curt_sp=sp    retn    
	                  mov reg,[iat];ret   [sp]=callret+1  curt_sp=sp-4  retn
	*/
	if(vm_iat[curt_index].ret_flag==1)
	{
		//iret=get_mem_value(curt_reg->r[REG_ESP]);
		ret_var=0;
		iret=xx_read_fmem(curt_reg->r[REG_ESP],(char*)&ret_var,sizeof(ret_var));
		if(iret==0)
		{
			return 0;
		}
		if(ret_var==callret && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x8) )
		{
			vm_iat[curt_index].fix_ip=vm_iat[curt_index].ip-1;
		}
		else if(ret_var==callret && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]) )
		{
			vm_iat[curt_index].fix_ip=vm_iat[curt_index].ip-1;
		}
		else if(ret_var==(callret+1) && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x04) )
		{
			vm_iat[curt_index].fix_ip=vm_iat[curt_index].ip;
		}
		else if(ret_var==(callret) && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x04) && old_reg->r[REG_EAX]!=curt_reg->r[REG_EAX])
		{
			
			vm_iat[curt_index].iat_flag=_Findsymbolicname(curt_reg->r[REG_EAX],vm_iat[curt_index].iat_sym);
			if(vm_iat[curt_index].iat_flag!=0)
			{
				vm_iat[curt_index].fix_ip=vm_iat[curt_index].ip;
				vm_iat[curt_index].fix_type=2;
				vm_iat[curt_index].iat=curt_reg->r[REG_EAX];
				vm_iat[curt_index].reg_index=REG_EAX;
				return 1;
			}
			memset(vm_iat[curt_index].iat_sym,0,sizeof(vm_iat[curt_index].iat_sym));
			vm_iat[curt_index].iat_flag=0;
		}

		if(vm_iat[curt_index].fix_ip)
		{
			vm_iat[curt_index].fix_type=2;
			if(old_reg->r[REG_EAX]!=curt_reg->r[REG_EAX])
			{
				vm_iat[curt_index].iat=curt_reg->r[REG_EAX];
				vm_iat[curt_index].iat_flag=_Findsymbolicname(vm_iat[curt_index].iat,vm_iat[curt_index].iat_sym);
				if(vm_iat[curt_index].iat_flag!=0)
				{
					vm_iat[curt_index].reg_index=REG_EAX;
					return 1;
				}
				memset(vm_iat[curt_index].iat_sym,0,sizeof(vm_iat[curt_index].iat_sym));
			}
			if(old_reg->r[REG_EBX]!=curt_reg->r[REG_EBX])
			{
				vm_iat[curt_index].iat=curt_reg->r[REG_EBX];
				vm_iat[curt_index].iat_flag=_Findsymbolicname(vm_iat[curt_index].iat,vm_iat[curt_index].iat_sym);
				if(vm_iat[curt_index].iat_flag!=0)
				{
					vm_iat[curt_index].reg_index=REG_EBX;
					return 1;
				}
				memset(vm_iat[curt_index].iat_sym,0,sizeof(vm_iat[curt_index].iat_sym));
			}
			if(old_reg->r[REG_ECX]!=curt_reg->r[REG_ECX])
			{
				vm_iat[curt_index].iat=curt_reg->r[REG_ECX];
				vm_iat[curt_index].iat_flag=_Findsymbolicname(vm_iat[curt_index].iat,vm_iat[curt_index].iat_sym);
				if(vm_iat[curt_index].iat_flag!=0)
				{
					vm_iat[curt_index].reg_index=REG_ECX;
					return 1;
				}
				memset(vm_iat[curt_index].iat_sym,0,sizeof(vm_iat[curt_index].iat_sym));
			}
			if(old_reg->r[REG_EDX]!=curt_reg->r[REG_EDX])
			{
				vm_iat[curt_index].iat=curt_reg->r[REG_EDX];
				vm_iat[curt_index].iat_flag=_Findsymbolicname(vm_iat[curt_index].iat,vm_iat[curt_index].iat_sym);
				if(vm_iat[curt_index].iat_flag!=0)
				{
					vm_iat[curt_index].reg_index=REG_EDX;
					return 1;
				}
				memset(vm_iat[curt_index].iat_sym,0,sizeof(vm_iat[curt_index].iat_sym));
			}
			if(old_reg->r[REG_ESI]!=curt_reg->r[REG_ESI])
			{
				vm_iat[curt_index].iat=curt_reg->r[REG_ESI];
				vm_iat[curt_index].iat_flag=_Findsymbolicname(vm_iat[curt_index].iat,vm_iat[curt_index].iat_sym);
				if(vm_iat[curt_index].iat_flag!=0)
				{
					vm_iat[curt_index].reg_index=REG_ESI;
					return 1;
				}
				memset(vm_iat[curt_index].iat_sym,0,sizeof(vm_iat[curt_index].iat_sym));
			}
			if(old_reg->r[REG_EDI]!=curt_reg->r[REG_EDI])
			{
				vm_iat[curt_index].iat=curt_reg->r[REG_EDI];
				vm_iat[curt_index].iat_flag=_Findsymbolicname(vm_iat[curt_index].iat,vm_iat[curt_index].iat_sym);
				if(vm_iat[curt_index].iat_flag!=0)
				{
					vm_iat[curt_index].reg_index=REG_EDI;
					return 1;
				}
				memset(vm_iat[curt_index].iat_sym,0,sizeof(vm_iat[curt_index].iat_sym));
			}
			if(old_reg->r[REG_EBP]!=curt_reg->r[REG_EBP])
			{
				vm_iat[curt_index].iat=curt_reg->r[REG_EBP];
				vm_iat[curt_index].iat_flag=_Findsymbolicname(vm_iat[curt_index].iat,vm_iat[curt_index].iat_sym);
				if(vm_iat[curt_index].iat_flag!=0)
				{
					vm_iat[curt_index].reg_index=REG_EBP;
					return 1;
				}
				memset(vm_iat[curt_index].iat_sym,0,sizeof(vm_iat[curt_index].iat_sym));
			}
			vm_iat[curt_index].iat=0;
		}
	}

	/*1.call [iat]  pop;call    [sp]=iat,[sp+4]=callret,curt_sp=sp-4  retn
					push;call   [sp]=iat,[sp+4]=callret,curt_sp=sp-4  retn
					call;ret    [sp]=iat,[sp+4]=callret+1,curt_sp=sp-8 retn
	*/
	if(vm_iat[curt_index].ret_flag==1)
	{
		//iret=get_mem_value(curt_reg->r[REG_ESP]+4);xx_read_fmem
		ret_var=0;
		iret=xx_read_fmem(curt_reg->r[REG_ESP]+4,(char*)&ret_var,sizeof(ret_var));
		if(iret==0)
		{
			return 0;
		}
		if(ret_var==callret && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x04) )
		{
			vm_iat[curt_index].fix_ip=vm_iat[curt_index].ip-1;
		}
		else if(ret_var==(callret+1) && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x08) )
		{
			vm_iat[curt_index].fix_ip=vm_iat[curt_index].ip;
		}

		if(vm_iat[curt_index].fix_ip)
		{
			vm_iat[curt_index].fix_type=1;
			//vm_iat[curt_index].iat=get_mem_value(curt_reg->r[REG_ESP]);
			iret=xx_read_fmem(curt_reg->r[REG_ESP],(char*)&(vm_iat[curt_index].iat),sizeof(vm_iat[curt_index].iat));
			if(iret==0)
			{
				return 0;
			}
			vm_iat[curt_index].iat_flag=_Findsymbolicname(vm_iat[curt_index].iat,vm_iat[curt_index].iat_sym);

			return 1;
		}
	
	}

	/*3.jmp [iat]    pop; jmp [iat]   [sp]=iat  [sp+4]=callret  curt_sp=sp-4  retn 4
					 push;jmp [iat]   [sp]=iat  [sp+4]=callret  curt_sp=sp-4  retn 4
	                 jmp [iat];ret    [sp]=iat  [sp+4]=callret  curt_sp=sp-8  retn 4
	*/
	if(vm_iat[curt_index].ret_flag==2)
	{
		if(curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x04) )
		{
			vm_iat[curt_index].fix_ip=vm_iat[curt_index].ip-1;
		}
		else if(curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x08))
		{
			vm_iat[curt_index].fix_ip=vm_iat[curt_index].ip;
		}

		if(vm_iat[curt_index].fix_ip)
		{
			vm_iat[curt_index].fix_type=3;
			//vm_iat[curt_index].iat=get_mem_value(curt_reg->r[REG_ESP]);
			iret=xx_read_fmem(curt_reg->r[REG_ESP],(char*)&(vm_iat[curt_index].iat),sizeof(vm_iat[curt_index].iat));
			if(iret==0)
			{
				return 0;
			}
			vm_iat[curt_index].iat_flag=_Findsymbolicname(vm_iat[curt_index].iat,vm_iat[curt_index].iat_sym);
			return 1;
		}
	}

	_Addtolist(0,1,"vm_iat type error [%08x] ",vm_iat[curt_index].ip);
	
	return 0;
}


/*判断指令是否时push,pop,ret*/
int is_inst_ppr(ulong  ip)
{
	struct XX_INST xx_inst;
	int cmdlen=0;       //cmd长度
	ulong base=0;       //模块基地址
	ulong des_addr=0;  //vmcall目的地址
	char cmdbuf[100];

	memset(&xx_inst,0,sizeof(struct XX_INST));

	base=_Plugingetvalue(VAL_MAINBASE);

	memset(cmdbuf,0,sizeof(cmdbuf));
	cmdlen=_Readcommand(ip,(char*)cmdbuf);
	if(cmdlen==0)
	{
		_Addtolist(0,1,"ReadCommand error");
		return 0;
	}

#if 0
	xx_inst.xx_inst_code.inst_system = INST_SYSTEM_CURT;
	memcpy(xx_inst.xx_inst_code.base, (char*)&base, xx_inst.xx_inst_code.inst_system);
	memcpy(xx_inst.xx_inst_code.current_addr, (char*)&ip, xx_inst.xx_inst_code.inst_system);

	xx_disasm(&xx_inst,cmdbuf);
	//_Addtolist(0,1,"finish_flag:[%d]",xx_inst[n].xx_inst_exist.finish_flag);

	if(xx_inst.xx_inst_exist.finish_flag!=INST_FLAG_EXIST)
	{
		//_Addtolist(0,1,"xx_disasm error ip:[%08x]",ip);
		return 0;
	}
	//_Addtolist(0,1,"xx_disasm  ip:[%08x] [%s]",ip,xx_inst[n].xx_inst_mic.disasm);

	if(xx_inst.xx_inst_code.disasm_length!=1)
	{
		return 0;
	}

	if(memcmp(xx_inst.xx_inst_code.opcode_type,op_push,op_len)!=0 && \
		memcmp(xx_inst.xx_inst_code.opcode_type,op_pop,op_len)!=0 && \
		memcmp(xx_inst.xx_inst_code.opcode_type,op_ret,op_len)!=0 )
	{
		return 0;
	}
#else
#endif
	return 1;
}




void print_sig_iat(int n)
{
	
		_Addtolist(0,1,"vm_iat[%08x].ip [%08x]",n,vm_iat[n].ip);
		//_Addtolist(0,1,"vm_iat[%08x].ret [%08x]",n,vm_iat[n].ret);
		_Addtolist(0,1,"vm_iat[%08x].ret_flag [%08x]",n,vm_iat[n].ret_flag);
		_Addtolist(0,1,"vm_iat[%08x].fix_ip [%08x]",n,vm_iat[n].fix_ip);
		_Addtolist(0,1,"vm_iat[%08x].fix_type [%08x]",n,vm_iat[n].fix_type);
		_Addtolist(0,1,"vm_iat[%08x].iat [%08x]",n,vm_iat[n].iat);
		_Addtolist(0,1,"vm_iat[%08x].iat_flag [%08x]",n,vm_iat[n].iat_flag);
		_Addtolist(0,1,"vm_iat[%08x].iat_sym [%s]",n,vm_iat[n].iat_sym);
		_Addtolist(0,1,"vm_iat[%08x].reg_index [%08x]",n,vm_iat[n].reg_index);
		_Addtolist(0,1,"                        ");

}



void print_iat()
{
	int n=0;

	
	for(n=0;n<iat_num;n++)
	{
		_Addtolist(0,1,"vm_iat[%08x].ip [%08x]",n,vm_iat[n].ip);
		//_Addtolist(0,1,"vm_iat[%08x].ret [%08x]",n,vm_iat[n].ret);
		_Addtolist(0,1,"vm_iat[%08x].ret_flag [%08x]",n,vm_iat[n].ret_flag);
		_Addtolist(0,1,"vm_iat[%08x].fix_ip [%08x]",n,vm_iat[n].fix_ip);
		_Addtolist(0,1,"vm_iat[%08x].fix_type [%08x]",n,vm_iat[n].fix_type);
		_Addtolist(0,1,"vm_iat[%08x].iat [%08x]",n,vm_iat[n].iat);
		_Addtolist(0,1,"vm_iat[%08x].iat_flag [%08x]",n,vm_iat[n].iat_flag);
		_Addtolist(0,1,"vm_iat[%08x].iat_sym [%s]",n,vm_iat[n].iat_sym);
		_Addtolist(0,1,"vm_iat[%08x].reg_index [%08x]",n,vm_iat[n].reg_index);
		_Addtolist(0,1,"                        ");
	}
}


void print_invalid_iat()
{
	int n=0;

	
	for(n=0;n<iat_num;n++)
	{
		if(vm_iat[n].iat_flag==0)
		{
			_Addtolist(0,1,"vm_iat[%08x].ip [%08x]",n,vm_iat[n].ip);
			//_Addtolist(0,1,"vm_iat[%08x].ret [%08x]",n,vm_iat[n].ret);
			_Addtolist(0,1,"vm_iat[%08x].ret_flag [%08x]",n,vm_iat[n].ret_flag);
			_Addtolist(0,1,"vm_iat[%08x].fix_ip [%08x]",n,vm_iat[n].fix_ip);
			_Addtolist(0,1,"vm_iat[%08x].fix_type [%08x]",n,vm_iat[n].fix_type);
			_Addtolist(0,1,"vm_iat[%08x].iat [%08x]",n,vm_iat[n].iat);
			_Addtolist(0,1,"vm_iat[%08x].iat_flag [%08x]",n,vm_iat[n].iat_flag);
			_Addtolist(0,1,"vm_iat[%08x].iat_sym [%s]",n,vm_iat[n].iat_sym);
			_Addtolist(0,1,"vm_iat[%08x].reg_index [%08x]",n,vm_iat[n].reg_index);
			_Addtolist(0,1,"                        ");
		}
	}
}



void replace_invalid_iat()
{
	int iat_index=0;
	ulong new_iat=0;
	/*输入无效iat的替换*/
	_Getlong("input iat index",&iat_index,4,0,DIA_HEXONLY);
	_Addtolist(0,1,"iat_index:[%08x]",iat_index);
	if(iat_index<iat_num)
	{
		_Getlong("input valid iat ",&new_iat,4,0,DIA_HEXONLY);
		_Addtolist(0,1,"new_iat:[%08x]",new_iat);
		if(new_iat)
		{
			vm_iat[iat_index].iat=new_iat;
			vm_iat[iat_index].iat_flag=_Findsymbolicname(vm_iat[iat_index].iat,vm_iat[iat_index].iat_sym);
			_Addtolist(0,1,"iat_index:[%08x] new_iat:[%08x] iat_sym:[%s] ",\
				iat_index,vm_iat[iat_index].iat,vm_iat[iat_index].iat_sym);
			return;
		}
	}

	_Addtolist(0,1,"iat_index:[%08x] new_iat:[%08x] error ",iat_index,new_iat);

}

void replace_all_invalid_iat()
{
	int n=0;
	ulong old_iat=0;
	ulong new_iat=0;
	/*输入无效iat的替换*/
	_Getlong("input invalid iat",&old_iat,4,0,DIA_HEXONLY);
	_Addtolist(0,1,"old_iat:[%08x]",old_iat);
	if(old_iat )
	{
		
		_Getlong("input valid iat ",&new_iat,4,0,DIA_HEXONLY);
		_Addtolist(0,1,"new_iat:[%08x]",new_iat);
		if(new_iat)
		{
			for(n=0;n<iat_num;n++)
			{
				if(vm_iat[n].iat_flag==0 && old_iat==vm_iat[n].iat)
				{
					vm_iat[n].iat=new_iat;
					vm_iat[n].iat_flag=_Findsymbolicname(vm_iat[n].iat,vm_iat[n].iat_sym);
					_Addtolist(0,1,"iat_index:[%08x] new_iat:[%08x] iat_sym:[%s] ",\
						n,vm_iat[n].iat,vm_iat[n].iat_sym);
				}
			}
			
			return;
		}
	}

	_Addtolist(0,1,"old_iat:[%08x] new_iat:[%08x] error ",old_iat,new_iat);

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

	/*创建目录*/
	iret = CreateDirectory(log_file, NULL);
	
	/*初始化日志文件名*/
	strcat(log_file, pname);
	strcat(log_file, ".txt");
	//MessageBox(0, log_file, "success", MB_OK);


	MessageBox(0, log_file, "success", MB_OK);
	return ;
};




/*修复vmcall*/
int fix_vmcall()
{
	int n=0;
	t_asmmodel t_asm;
	char cmdbuf[200];
	
	HANDLE hp=0;
	
	ulong offset=0;
	int iret=0;


	char tmp[1000];


	hp=(HANDLE)_Plugingetvalue(VAL_HPROCESS);

	if(iat_base!=0)
	{
		VirtualFreeEx(hp,(void*)iat_base,0,MEM_DECOMMIT);
		
	}
	iat_base=0;
	/*申请一个内存,存放iat的地址*/

	iat_base=(unsigned long)VirtualAllocEx(hp,0,iat_num*(sizeof(int)),MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	if(iat_base==0)
	{
		_Addtolist(0,1,"VirtualAllocEx error");
		return 0;
	}
	
	//memset(iat_base,0,iat_num*(sizeof(int)));

	offset=0;
	/*修复vmcall*/
	for(n=0;n<iat_num;n++)
	{
		iret=_Writememory(&vm_iat[n].iat,iat_base+offset,sizeof(int),MM_RESTORE);
		if(iret!=sizeof(int))
		{
			VirtualFreeEx(hp,(void*)iat_base,0,MEM_DECOMMIT);
			iat_base=0;
			_Addtolist(0,1,"_Writememory error");
			return 0;
		}
		vm_iat[n].iat_addr=iat_base+offset;
		offset=offset+sizeof(int);


		if(vm_iat[n].fix_type==1)
		{
			/*call [iat]*/
			memset(cmdbuf,0,sizeof(cmdbuf));
			sprintf(cmdbuf+strlen(cmdbuf),"call dword ptr ");
			sprintf(cmdbuf+strlen(cmdbuf),"[");
			sprintf(cmdbuf+strlen(cmdbuf),"%08x",vm_iat[n].iat_addr);
			sprintf(cmdbuf+strlen(cmdbuf),"]");
		}
		else if(vm_iat[n].fix_type==2)
		{
			/*mov reg,[iat]*/
			memset(cmdbuf,0,sizeof(cmdbuf));
			sprintf(cmdbuf+strlen(cmdbuf),"mov ");
			switch(vm_iat[n].reg_index)
			{
			case REG_EAX:
				sprintf(cmdbuf+strlen(cmdbuf),"eax,");
				break;
			case REG_EBX:
				sprintf(cmdbuf+strlen(cmdbuf),"ebx,");
				break;
			case REG_ECX:
				sprintf(cmdbuf+strlen(cmdbuf),"ecx,");
				break;
			case REG_EDX:
				sprintf(cmdbuf+strlen(cmdbuf),"edx,");
				break;
			case REG_EBP:
				sprintf(cmdbuf+strlen(cmdbuf),"ebp,");
				break;
			case REG_ESI:
				sprintf(cmdbuf+strlen(cmdbuf),"esi,");
				break;
			case REG_EDI:
				sprintf(cmdbuf+strlen(cmdbuf),"edi,");
				break;
			default:
				_Addtolist(0,1,"fix_type 2 error index:[%08x] ip:[%08x]",n,vm_iat[n].ip);
				continue;
			}
			sprintf(cmdbuf+strlen(cmdbuf),"dword ptr ");
			sprintf(cmdbuf+strlen(cmdbuf),"[");
			sprintf(cmdbuf+strlen(cmdbuf),"%08x",vm_iat[n].iat_addr);
			sprintf(cmdbuf+strlen(cmdbuf),"]");
		}
		else if(vm_iat[n].fix_type==3)
		{
			/*jmp [iat]*/
			memset(cmdbuf,0,sizeof(cmdbuf));
			sprintf(cmdbuf+strlen(cmdbuf),"jmp dword ptr ");
			sprintf(cmdbuf+strlen(cmdbuf),"[");
			sprintf(cmdbuf+strlen(cmdbuf),"%08x",vm_iat[n].iat_addr);
			sprintf(cmdbuf+strlen(cmdbuf),"]");
		}
		else
		{
			_Addtolist(0,1,"fix_type  error index:[%08x] ip:[%08x]",n,vm_iat[n].ip);
			continue;
			//return 0;
		}
		memset(&t_asm,0,sizeof(t_asm));
		memset(tmp,0,sizeof(tmp));
		_Assemble(cmdbuf,vm_iat[n].fix_ip,&t_asm,0,0,tmp);
		
		iret=_Writememory(t_asm.code,vm_iat[n].fix_ip,t_asm.length,MM_RESTORE);
		if(iret!=t_asm.length)
		{
			VirtualFreeEx(hp,(void*)iat_base,0,MEM_DECOMMIT);
			iat_base=0;
			_Addtolist(0,1,"_Writememory error");
			return 0;
		}
		_Addtolist(0,1,"[%08x] write cmd:[%s] cmdlen[%08x]",n,cmdbuf,t_asm.length);
	}

	//VirtualFreeEx(hp,(void*)iat_base,0,MEM_DECOMMIT);
	MessageBox(0,"fix finish","success",MB_OK);
	return 1;
}





/*日志，详细信息*/
int log_vmcall()
{
	int n=0;

	char cmdbuf[200];

	HANDLE hp=0;
	ulong iat_base=0;
	ulong offset=0;
	int iret=0;

	

	char tmp[1000];


	xx_init();
	if(strlen(log_file)==0)
	{
		return 0;
	}

	iat_base=0x10000000;
	
	xx_cover_file(log_file,"websit: xxdisasm.com\r\n",strlen("websit: xxdisasm.com\r\n"));

	memset(tmp,0,sizeof(tmp));
	sprintf(tmp,"iat num:%08x  \r\n",iat_num);
	xx_append_file(log_file,tmp,strlen(tmp));

	/*修复vmcall*/
	for(n=0;n<iat_num;n++)
	{
		vm_iat[n].iat_addr=iat_base+offset;
		offset=offset+sizeof(int);

		if(vm_iat[n].fix_type==1)
		{
			/*call [iat]*/
			memset(cmdbuf,0,sizeof(cmdbuf));
			sprintf(cmdbuf+strlen(cmdbuf),"call dword ptr ");
			sprintf(cmdbuf+strlen(cmdbuf),"[");
			sprintf(cmdbuf+strlen(cmdbuf),"%08x",vm_iat[n].iat_addr);
			sprintf(cmdbuf+strlen(cmdbuf),"]");
		}
		else if(vm_iat[n].fix_type==2)
		{
			/*mov reg,[iat]*/
			memset(cmdbuf,0,sizeof(cmdbuf));
			sprintf(cmdbuf+strlen(cmdbuf),"mov ");
			switch(vm_iat[n].reg_index)
			{
			case REG_EAX:
				sprintf(cmdbuf+strlen(cmdbuf),"eax,");
				break;
			case REG_EBX:
				sprintf(cmdbuf+strlen(cmdbuf),"ebx,");
				break;
			case REG_ECX:
				sprintf(cmdbuf+strlen(cmdbuf),"ecx,");
				break;
			case REG_EDX:
				sprintf(cmdbuf+strlen(cmdbuf),"edx,");
				break;
			case REG_EBP:
				sprintf(cmdbuf+strlen(cmdbuf),"ebp,");
				break;
			case REG_ESI:
				sprintf(cmdbuf+strlen(cmdbuf),"esi,");
				break;
			case REG_EDI:
				sprintf(cmdbuf+strlen(cmdbuf),"edi,");
				break;
			default:
				_Addtolist(0,1,"fix_type 2 error index:[%08x] ip:[%08x]",n,vm_iat[n].ip);
				continue;
			}
			sprintf(cmdbuf+strlen(cmdbuf),"dword ptr ");
			sprintf(cmdbuf+strlen(cmdbuf),"[");
			sprintf(cmdbuf+strlen(cmdbuf),"%08x",vm_iat[n].iat_addr);
			sprintf(cmdbuf+strlen(cmdbuf),"]");
		}
		else if(vm_iat[n].fix_type==3)
		{
			/*jmp [iat]*/
			memset(cmdbuf,0,sizeof(cmdbuf));
			sprintf(cmdbuf+strlen(cmdbuf),"jmp dword ptr ");
			sprintf(cmdbuf+strlen(cmdbuf),"[");
			sprintf(cmdbuf+strlen(cmdbuf),"%08x",vm_iat[n].iat_addr);
			sprintf(cmdbuf+strlen(cmdbuf),"]");
		}
		else
		{
			_Addtolist(0,1,"fix_type  error index:[%08x] ip:[%08x]",n,vm_iat[n].ip);
			//return 0;
			continue;
		}

		/*写入日志*/
		memset(tmp,0,sizeof(tmp));
		sprintf(tmp,"%08x  < %s >   [ %s ]\r\n",vm_iat[n].ip,cmdbuf,vm_iat[n].iat_sym);
		xx_append_file(log_file,tmp,strlen(tmp));
		//_Addtolist(0,1,"write cmd:[%s] cmdlen[%08x]",cmdbuf,t_asm.length);
	}

	MessageBox(0,"log finish","success",MB_OK);
	return 1;
}




/*只注释api*/
int notes_vmcall()
{
	int n=0;
	int iret=0;
	char snote[]="API: ";


	char tmp[1000];



	/*vmcall*/
	for(n=0;n<iat_num;n++)
	{
		memset(tmp,0,sizeof(tmp));
		sprintf(tmp,"%s%s",snote,vm_iat[n].iat_sym);
		_Insertname(vm_iat[n].ip,NM_COMMENT,tmp);
	}


	MessageBox(0,"notes finish","success",MB_OK);
	return 1;
}

