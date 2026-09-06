#include <windows.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>

//#include "comm_base.h"
#include "Plugin.h"
#include "xx_def.h"
#include "xx_inst.h"
#include "xx_opdef.h"
#include "resource.h"
#include "xx_comm32.h"
#include "xxdisasm32.h"




#pragma comment(lib,"ollydbg.lib")

/////////////////////////////////////////////






////////////////////
	unsigned int code_start=0;
	unsigned int code_end=0;
	unsigned int vmp0_start=0;
	unsigned int vmp0_end=0;
///////////////////////////////////////////////
int search_vmcall(ulong codestart,ulong codeend,ulong vmp0start,ulong vmp0end,struct T_VM_IAT*);
int xx_vm_iat();
int vmcall_ret_bp(struct T_VM_IAT *t_vm_iat,int num);
void vm_iat_chgip();
void reset_curt_reg();
void save_old_reg();
ulong get_mem_value(ulong addr);
int judge_vmcall_type(t_reg *old_reg,t_reg *curt_reg);
void print_iat();
void print_invalid_iat();
void replace_invalid_iat();
void replace_all_invalid_iat();
int fix_vmcall();
int log_vmcall();
int notes_vmcall();

int search_vmrc_api();
int xreg_incmp(char* reg,char* sub);
int xreg_cmp(char* buf1,char* buf2);



char log_file[0x400]={0};
//////////////////////////////////////////////////////////
extern int init_file_path(char *ppath,char *pname,char *file);


extern int   cfg_win_create();
extern char xx_version[];
///////////////////////////////////////////////
struct T_VM_IAT
{
	ulong ip;     //vmcall ip
	int fix_type;  //vmcall type 1.call [iat]  2.mov reg,[iat] 3.jmp [iat]
	ulong fix_ip;  //vmcall fix ip
	ulong iat;     //api addr
	int iat_flag;  //iat是否有效,0无效，1有效
	ulong ret;     //返回地址，设置断点
	int ret_flag;  //1.retn  2.retn 4  3.retn 8
	char iat_sym[TEXTLEN]; //iat描述
	int reg_index;  //mov reg,[iat]  reg index
	ulong iat_addr; //目标进程iat的地址
};
struct T_VM_IAT *vm_iat=0;
int vm_iat_index=0;
////////////////////////////////////////////////////
char xx_main_menu[0x200];
//char xx_disasm_menu[]="#xx_vm{|0search vmcall|,|1print|,|2print invalid iat|,|3replace invalid iat|}";


HINSTANCE hinst=0;
int vm_iat_flag=0;
int iat_num=0;
t_reg old_reg;

char  xdbg_path[0x400];

/////////////////////////////////
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

	_Addtolist(0,1,xx_version);
	_Addtolist(0,1,"  Written by xxdisasm");



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
	vm_iat_index=0;

	vm_iat_flag=0;
	memset(&old_reg,0,sizeof(t_reg));


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


	return 0;
};


extc void _export cdecl ODBG_Pluginreset()
{
	memset(&old_reg,0,sizeof(t_reg));
	vm_iat_flag=0;
	vm_iat=0;
	iat_num=0;
	vm_iat_index=0;

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
				xx_vm_iat();
			}
			
			break;
		case 1:
			_Addtolist(0,1,"vm_iat_index:[%08x]  iat_num:[%08x]",vm_iat_index,iat_num);
			if(vm_iat_index==iat_num)
			{
				print_iat();
			}
			break;
		case 2:
	
			if(vm_iat_index==iat_num)
			{
				print_invalid_iat();
			}
			
			break;
		case 3:
			//reload_invalid_iat();
			if(vm_iat_index==iat_num)
			{
				replace_invalid_iat();
			}
			break;
		case 4:
			if(vm_iat_index==iat_num)
			{
				replace_all_invalid_iat();
			}
			break;
		case 5:
			/*修复vmcall*/
			if(vm_iat==0)
			{
				MessageBox(0,"search vm_iat please","error",MB_OK);
				break;
			}
			iret=fix_vmcall();
			if(iret==1)
			{
				MessageBox(0,"fix success","success",MB_OK);
			}
			else
			{
				MessageBox(0,"fix error","error",MB_OK);
			}
			break;
		case 6:
			if(vm_iat==0)
			{
				MessageBox(0,"search vm_iat please","error",MB_OK);
				break;
			}
			iret=log_vmcall();
			if(iret==1)
			{
				MessageBox(0,"log success","success",MB_OK);
			}
			else
			{
				MessageBox(0,"log error","error",MB_OK);
			}
			break;
		case 7:
			if(vm_iat==0)
			{
				MessageBox(0,"search vm_iat please","error",MB_OK);
				break;
			}
			iret=notes_vmcall();
			if(iret==1)
			{
				MessageBox(0,"set comment success","success",MB_OK);
			}
			else
			{
				MessageBox(0,"set comment error","error",MB_OK);
			}
			break;
		case 8:
			/*释放空间*/
			vm_iat_flag=0;
			iat_num=0;
			vm_iat_index=0;
			memset(&old_reg,0,sizeof(t_reg));
			VirtualFree((void*)vm_iat,0,MEM_DECOMMIT);
			vm_iat=0;
			break;
		case 9:
			hwnd=_Plugingetvalue(VAL_HWMAIN);
			if(hwnd)
			{
				iret=cfg_win_create(hwnd);
			}
		
			break;
		case 10:
			hwnd=_Plugingetvalue(VAL_HWMAIN);
			if(hwnd)
			{
				xx_get_user(hwnd);
				//CreateThread(NULL,NULL,(LPTHREAD_START_ROUTINE)InitInstance_user,&hwnd,0,0);
				//_Addtolist(0,1,"user 1");
			}
			break;
		}
		break;

	}
	
};



extc int _export cdecl ODBG_Pausedex(int reason, int extdata, t_reg *reg, DEBUG_EVENT *debugevent)
{
	int iret=0;
	
	if(vm_iat_flag==1 )
	{
		switch(reason)
		{
		case PP_SINGLESTEP:
			break;
		case PP_HWBREAK:
			break;
		case PP_INT3BREAK:
			/*暂停以后比较是否是iat ret*/
			if(reg->ip==vm_iat[vm_iat_index].ret)
			{
				
				_Addtolist(0,1,"vm_iat[%08x].ip :[%08x]",vm_iat_index,vm_iat[vm_iat_index].ip);
				_Addtolist(0,1,"esp:[%08x]..[%08x] [%08x] [%08x] [%08x]",reg->r[REG_ESP],\
					get_mem_value(reg->r[REG_ESP]),get_mem_value(reg->r[REG_ESP]+4),\
					get_mem_value(reg->r[REG_ESP]+0x8),get_mem_value(reg->r[REG_ESP]+0x0c));
				/*判断vmcall类型*/
				judge_vmcall_type(&old_reg,reg);

				reset_curt_reg();
				vm_iat_index++;
				if(vm_iat_index<iat_num)
				{
					vm_iat_chgip();
					return 1;
				}
				vm_iat_flag=0;
			}
			break;
		case (PP_SINGLESTEP | PP_PAUSE):
			break;
		}
	}
	return 0;
};


int xx_vm_iat()
{
	ulong iret=0;

	ulong dret=0xffffffff;

	int n=0;
	
	memset(&old_reg,0,sizeof(t_reg));

#if 0
	iret=_Pluginreadintfromini(hinst,"code_start",dret);
	if(iret==dret)
	{
		_Addtolist(0,1,"_Pluginreadintfromini code_start error");
		return -1;
	}
	code_start=iret;

	iret=_Pluginreadintfromini(hinst,"code_end",dret);
	if(iret==dret)
	{
		_Addtolist(0,1,"_Pluginreadintfromini code_end error");
		return -1;
	}
	code_end=iret;

	iret=_Pluginreadintfromini(hinst,"vmp0_start",dret);
	if(iret==dret)
	{
		_Addtolist(0,1,"_Pluginreadintfromini vmp0_start error");
		return -1;
	}
	vmp0_start=iret;

	iret=_Pluginreadintfromini(hinst,"vmp0_end",dret);
	if(iret==dret)
	{
		_Addtolist(0,1,"_Pluginreadintfromini vmp0_end error");
		return -1;
	}
	vmp0_end=iret;
#else
	if(!code_start || !code_end || !vmp0_start || !vmp0_end)
	{
		MessageBox(0,"Configure,please","error",MB_OK);
		return -1;
	}
#endif


	_Addtolist(0,1,"code_start:[%08x],code_end:[%08x],vmp0_start:[%08x],vmp0_end:[%08x]",\
		code_start,code_end,vmp0_start,vmp0_end);

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

	vmcall_ret_bp(vm_iat,iat_num);
	

	vm_iat_index=0;  //global

	/*保存原环境reg值*/
	save_old_reg();

	vm_iat_chgip();

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
		
		if(memcmp(xx_inst.xx_inst_code.opcode_type,op_call,op_len)==0 &&
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
				if(tmp[0]==0x90)
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
	
	_Addtolist(0,1,"search complete num:[%08x][%0d]",num,num);
	return num;
} 


void save_old_reg()
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

	memcpy(&old_reg,&pt_thread->reg,sizeof(struct t_reg));

}

void reset_curt_reg()
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
	memcpy(&pt_thread->reg,&old_reg,sizeof(struct t_reg));
}

void vm_iat_chgip()
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
	
	pt_thread->reg.ip=vm_iat[vm_iat_index].ip;

	vm_iat_flag=1;

	_Go(threadid,0,STEP_RUN,0,0);

	return;
};


int vmcall_ret_bp(struct T_VM_IAT *t_vm_iat,int num)
{
	//char tmp[200];
	char cmdbuf[200];

	int iret=0;

	int n=0;
	int m=0;
	ulong ip=0;
	struct XX_INST xx_inst;

	int cmdlen=0;       //cmd长度
	ulong base=0;       //模块基地址


	base=_Plugingetvalue(VAL_MAINBASE);

	for(n=0;n<num;n++)
	{
		ip=t_vm_iat[n].ip;
		m=0;
	
		while(m<100)
		{
			memset(cmdbuf,0,sizeof(cmdbuf));
			cmdlen=_Readcommand(ip,(char*)cmdbuf);
			if(cmdlen==0)
			{
				_Addtolist(0,1,"ReadCommand error");
				return -1;
			}
			
			memset(&xx_inst,0,sizeof(struct XX_INST));

			xx_inst.xx_inst_code.inst_system = INST_SYSTEM_CURT;
			memcpy(xx_inst.xx_inst_code.base, (char*)&base, xx_inst.xx_inst_code.inst_system);
			memcpy(xx_inst.xx_inst_code.current_addr, (char*)&ip, xx_inst.xx_inst_code.inst_system);

			xx_disasm(&xx_inst,cmdbuf);
			
			if(xx_inst.xx_inst_exist.finish_flag!=INST_FLAG_EXIST)
			{
				_Addtolist(0,1,"xx_disasm error ip:[%08x]",ip);
				return -1;
			}
			
			//_Addtolist(0,1,"n:[%08x] m:[%08x] ip:[%08x] disasm:[%s]",n,m,ip,xx_inst.xx_inst_mic.disasm);
			m++;
			if(memcmp(xx_inst.xx_inst_code.opcode_type,op_jmp,op_len)==0 &&
				xx_inst.xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_DIS)
			{
				ip=*(int*)xx_inst.xx_inst_items.xx_inst_items_var[0].item_const;
			}
			else if(memcmp(xx_inst.xx_inst_code.opcode_type,op_call,op_len)==0 &&
				xx_inst.xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_DIS)
			{
				ip=*(int*)xx_inst.xx_inst_items.xx_inst_items_var[0].item_const;
			}
			else if(memcmp(xx_inst.xx_inst_code.opcode_type,op_ret,op_len)==0)
			{
				if(xx_inst.xx_inst_items.nitem==0)
				{
					t_vm_iat[n].ret_flag=1;
				}
				else if(xx_inst.xx_inst_items.nitem==1 && xx_inst.xx_inst_items.xx_inst_items_var[0].item_const[0]==0x04 )
				{
					t_vm_iat[n].ret_flag=2;
				}
				/*取ret地址，设置软断点*/
				t_vm_iat[n].ret=ip;
				_Setbreakpointext(ip,TY_ONESHOT,0,0);
				break;
			}
			else
			{
				ip=ip+xx_inst.xx_inst_code.disasm_length;
			}
			
		}
	}

	return 1;
}


/*判断vmcall类型*/
int judge_vmcall_type(t_reg *old_reg,t_reg *curt_reg)
{
	ulong callret=0;
	ulong iret=0;

	int n=0;
	int m=0;
	int chgnum=0;
	

	callret=vm_iat[vm_iat_index].ip+5;
	vm_iat[vm_iat_index].fix_ip=0;
	/*1.call [iat]  pop;call    [sp]=iat,[sp+4]=callret,curt_sp=sp-4  retn
					push;call   [sp]=iat,[sp+4]=callret,curt_sp=sp-4  retn
					call;ret    [sp]=iat,[sp+4]=callret+1,curt_sp=sp-8 retn
	*/
	if(vm_iat[vm_iat_index].ret_flag==1)
	{
		iret=get_mem_value(curt_reg->r[REG_ESP]+4);
		if(iret==callret && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x04))
		{
			vm_iat[vm_iat_index].fix_ip=vm_iat[vm_iat_index].ip-1;
		}
		else if(iret==callret && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x4))
		{
			vm_iat[vm_iat_index].fix_ip=vm_iat[vm_iat_index].ip-1;
		}
		else if(iret==(callret+1) && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x08))
		{
			vm_iat[vm_iat_index].fix_ip=vm_iat[vm_iat_index].ip;
		}
		if(vm_iat[vm_iat_index].fix_ip)
		{
			vm_iat[vm_iat_index].fix_type=1;
			vm_iat[vm_iat_index].iat=get_mem_value(curt_reg->r[REG_ESP]);
			vm_iat[vm_iat_index].iat_flag=_Findsymbolicname(vm_iat[vm_iat_index].iat,vm_iat[vm_iat_index].iat_sym);

			return 1;
		}
	
	}

	/*2.mov reg,[iat] pop;mov reg,[iat]   [sp]=callret curt_sp=sp-8   retn
					  push;mov reg,[iat]  [sp]=callret curt_sp=sp    retn    
	                  mov reg,[iat];ret   [sp]=callret+1  curt_sp=sp-4  retn
	*/
	if(vm_iat[vm_iat_index].ret_flag==1)
	{
		iret=get_mem_value(curt_reg->r[REG_ESP]);
		if(iret==callret && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x8))
		{
			vm_iat[vm_iat_index].fix_ip=vm_iat[vm_iat_index].ip-1;
		}
		else if(iret==callret && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]))
		{
			vm_iat[vm_iat_index].fix_ip=vm_iat[vm_iat_index].ip-1;
		}
		else if(iret==(callret+1) && curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x04))
		{
			vm_iat[vm_iat_index].fix_ip=vm_iat[vm_iat_index].ip;
		}
		if(vm_iat[vm_iat_index].fix_ip)
		{
			vm_iat[vm_iat_index].fix_type=2;
			if(old_reg->r[REG_EAX]!=curt_reg->r[REG_EAX])
			{
				vm_iat[vm_iat_index].iat=curt_reg->r[REG_EAX];
				vm_iat[vm_iat_index].iat_flag=_Findsymbolicname(vm_iat[vm_iat_index].iat,vm_iat[vm_iat_index].iat_sym);
				if(vm_iat[vm_iat_index].iat_flag!=0)
				{
					vm_iat[vm_iat_index].reg_index=REG_EAX;
					return 1;
				}
				memset(vm_iat[vm_iat_index].iat_sym,0,sizeof(vm_iat[vm_iat_index].iat_sym));
			}
			if(old_reg->r[REG_EBX]!=curt_reg->r[REG_EBX])
			{
				vm_iat[vm_iat_index].iat=curt_reg->r[REG_EBX];
				vm_iat[vm_iat_index].iat_flag=_Findsymbolicname(vm_iat[vm_iat_index].iat,vm_iat[vm_iat_index].iat_sym);
				if(vm_iat[vm_iat_index].iat_flag!=0)
				{
					vm_iat[vm_iat_index].reg_index=REG_EBX;
					return 1;
				}
				memset(vm_iat[vm_iat_index].iat_sym,0,sizeof(vm_iat[vm_iat_index].iat_sym));
			}
			if(old_reg->r[REG_ECX]!=curt_reg->r[REG_ECX])
			{
				vm_iat[vm_iat_index].iat=curt_reg->r[REG_ECX];
				vm_iat[vm_iat_index].iat_flag=_Findsymbolicname(vm_iat[vm_iat_index].iat,vm_iat[vm_iat_index].iat_sym);
				if(vm_iat[vm_iat_index].iat_flag!=0)
				{
					vm_iat[vm_iat_index].reg_index=REG_ECX;
					return 1;
				}
				memset(vm_iat[vm_iat_index].iat_sym,0,sizeof(vm_iat[vm_iat_index].iat_sym));
			}
			if(old_reg->r[REG_EDX]!=curt_reg->r[REG_EDX])
			{
				vm_iat[vm_iat_index].iat=curt_reg->r[REG_EDX];
				vm_iat[vm_iat_index].iat_flag=_Findsymbolicname(vm_iat[vm_iat_index].iat,vm_iat[vm_iat_index].iat_sym);
				if(vm_iat[vm_iat_index].iat_flag!=0)
				{
					vm_iat[vm_iat_index].reg_index=REG_EDX;
					return 1;
				}
				memset(vm_iat[vm_iat_index].iat_sym,0,sizeof(vm_iat[vm_iat_index].iat_sym));
			}
			if(old_reg->r[REG_ESI]!=curt_reg->r[REG_ESI])
			{
				vm_iat[vm_iat_index].iat=curt_reg->r[REG_ESI];
				vm_iat[vm_iat_index].iat_flag=_Findsymbolicname(vm_iat[vm_iat_index].iat,vm_iat[vm_iat_index].iat_sym);
				if(vm_iat[vm_iat_index].iat_flag!=0)
				{
					vm_iat[vm_iat_index].reg_index=REG_ESI;
					return 1;
				}
				memset(vm_iat[vm_iat_index].iat_sym,0,sizeof(vm_iat[vm_iat_index].iat_sym));
			}
			if(old_reg->r[REG_EDI]!=curt_reg->r[REG_EDI])
			{
				vm_iat[vm_iat_index].iat=curt_reg->r[REG_EDI];
				vm_iat[vm_iat_index].iat_flag=_Findsymbolicname(vm_iat[vm_iat_index].iat,vm_iat[vm_iat_index].iat_sym);
				if(vm_iat[vm_iat_index].iat_flag!=0)
				{
					vm_iat[vm_iat_index].reg_index=REG_EDI;
					return 1;
				}
				memset(vm_iat[vm_iat_index].iat_sym,0,sizeof(vm_iat[vm_iat_index].iat_sym));
			}
			if(old_reg->r[REG_EBP]!=curt_reg->r[REG_EBP])
			{
				vm_iat[vm_iat_index].iat=curt_reg->r[REG_EBP];
				vm_iat[vm_iat_index].iat_flag=_Findsymbolicname(vm_iat[vm_iat_index].iat,vm_iat[vm_iat_index].iat_sym);
				if(vm_iat[vm_iat_index].iat_flag!=0)
				{
					vm_iat[vm_iat_index].reg_index=REG_EBP;
					return 1;
				}
				memset(vm_iat[vm_iat_index].iat_sym,0,sizeof(vm_iat[vm_iat_index].iat_sym));
			}
			vm_iat[vm_iat_index].iat=0;
		}
	}

	/*3.jmp [iat]    pop; jmp [iat]   [sp]=iat  [sp+4]=callret  curt_sp=sp-4  retn 4
					 push;jmp [iat]   [sp]=iat  [sp+4]=callret  curt_sp=sp-4  retn 4
	                 jmp [iat];ret    [sp]=iat  [sp+4]=callret  curt_sp=sp-8  retn 4
	*/
	if(vm_iat[vm_iat_index].ret_flag==2)
	{
		if(curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-0x04))
		{
			vm_iat[vm_iat_index].fix_ip=vm_iat[vm_iat_index].ip-1;
		}
		else if(curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-4))
		{
			vm_iat[vm_iat_index].fix_ip=vm_iat[vm_iat_index].ip-1;
		}
		else if(curt_reg->r[REG_ESP]==(old_reg->r[REG_ESP]-8))
		{
			vm_iat[vm_iat_index].fix_ip=vm_iat[vm_iat_index].ip;
		}
		if(vm_iat[vm_iat_index].fix_ip)
		{
			vm_iat[vm_iat_index].fix_type=3;
			vm_iat[vm_iat_index].iat=get_mem_value(curt_reg->r[REG_ESP]);
			vm_iat[vm_iat_index].iat_flag=_Findsymbolicname(vm_iat[vm_iat_index].iat,vm_iat[vm_iat_index].iat_sym);
			return 1;
		}
	}

	_Addtolist(0,1,"vm_iat type error [%08x] ",vm_iat[vm_iat_index].ip);

	return 1;
}




ulong get_mem_value(ulong addr)
{
	int iret=0;
	ulong var=0;
	char tmp[200];

	memset(tmp,0,sizeof(tmp));
	iret=_Readmemory(tmp,addr,sizeof(int),MM_RESTORE);
	if(iret!=sizeof(int))
	{
		_Addtolist(0,1,"_Readmemory [%08x] error",addr);
	}

	var=*(int*)tmp;

	return var;
}


void print_iat()
{
	int n=0;

	
	for(n=0;n<iat_num;n++)
	{
		_Addtolist(0,1,"vm_iat[%08x].ip [%08x]",n,vm_iat[n].ip);
		_Addtolist(0,1,"vm_iat[%08x].ret [%08x]",n,vm_iat[n].ret);
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
			_Addtolist(0,1,"vm_iat[%08x].ret [%08x]",n,vm_iat[n].ret);
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


/*修复vmcall*/
int fix_vmcall()
{
	int n=0;
	t_asmmodel t_asm;
	char cmdbuf[200];
	
	HANDLE hp=0;
	ulong iat_base=0;
	ulong offset=0;
	int iret=0;


	char tmp[1000];
	/*申请一个内存,存放iat的地址*/
	hp=_Plugingetvalue(VAL_HPROCESS);
	iat_base=VirtualAllocEx(hp,0,iat_num*(sizeof(int)),MEM_COMMIT,PAGE_EXECUTE_READWRITE);
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
				return 1;
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
			return 0;
		}
		memset(&t_asm,0,sizeof(t_asm));
		memset(tmp,0,sizeof(tmp));
		_Assemble(cmdbuf,vm_iat[n].fix_ip,&t_asm,0,0,tmp);
		
		iret=_Writememory(t_asm.code,vm_iat[n].fix_ip,t_asm.length,MM_RESTORE);
		if(iret!=t_asm.length)
		{
			_Addtolist(0,1,"_Writememory error");
			return 0;
		}
		_Addtolist(0,1,"write cmd:[%s] cmdlen[%08x]",cmdbuf,t_asm.length);
	}
	return 1;
}


void xx_init()
{
	
	char *pname;
	int iret = 0;

	char sret[200];
	char tmp[200];
	char dsret[] = "xxxx";
	char ppath[1000];


	if (strlen(xdbg_path) == 0)
	{
		MessageBox(0,"dbgpath error","info",MB_OK);
		return ;
	}

	memset(log_file, 0, sizeof(log_file));

	pname=(char *)_Plugingetvalue(VAL_PROCESSNAME);

	memset(ppath, 0, sizeof(ppath));
	sprintf(ppath, "%s", xdbg_path);

	iret = init_file_path(ppath, pname, log_file);
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
				return 0;
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
			return 0;
		}

		/*写入日志*/
		memset(tmp,0,sizeof(tmp));
		sprintf(tmp,"%08x  < %s >   [ %s ]\r\n",vm_iat[n].ip,cmdbuf,vm_iat[n].iat_sym);
		xx_append_file(log_file,tmp,strlen(tmp));
		//_Addtolist(0,1,"write cmd:[%s] cmdlen[%08x]",cmdbuf,t_asm.length);
	}
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
	return 1;
}


/*搜索api
未使用
*/
int search_vmrc_api()
{
	/*搜索所有的vmcall*/
	char tmp[200];
	char cmdbuf[200];

	ulong iret=0;
	ulong dret=0xffffffff;

	ulong n=0;
	ulong ip=0;
	struct XX_INST xx_inst;
	int num=0;
	int cmdlen=0;       //cmd长度
	ulong base=0;       //模块基地址

	ulong vmp0_start=0;
	ulong vmp0_end=0;

	char cmd[]={0x8b,0xec};

	base=_Plugingetvalue(VAL_MAINBASE);

	iret=_Pluginreadintfromini(hinst,"vmp0_start",dret);
	if(iret==dret)
	{
		_Addtolist(0,1,"_Pluginreadintfromini vmp0_start error");
		return -1;
	}
	vmp0_start=iret;

	iret=_Pluginreadintfromini(hinst,"vmp0_end",dret);
	if(iret==dret)
	{
		_Addtolist(0,1,"_Pluginreadintfromini vmp0_end error");
		return -1;
	}
	vmp0_end=iret;

	n=vmp0_start;
	num=0;
	for(n;n<vmp0_end;n++)
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
		
		if(memcmp(cmdbuf,cmd,2)==0)
		{
			continue;
		}
		
	#if 0
		xx_disasm(&xx_inst,cmdbuf,base,ip);
		//_Addtolist(0,1,"finish_flag:[%d]",xx_inst[n].xx_inst_exist.finish_flag);
		
		if(xx_inst.xx_inst_exist.finish_flag!=INST_FLAG_EXIST)
		{
			_Addtolist(0,1,"xx_disasm error ip:[%08x]",ip);
			continue;
		}

		//_Addtolist(0,1,"xx_disasm  ip:[%08x] [%s]",ip,xx_inst[n].xx_inst_mic.disasm);
		if(memcmp(xx_inst.xx_inst_code.opcode_type,op_mov,op_len)==0 && \
			xx_inst.xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG && \
			xx_inst.xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG && \
			xreg_cmp(xx_inst.xx_inst_items.xx_inst_items_var[0].item_base_def,xreg_ebp)==0 && \
			xreg_cmp(xx_inst.xx_inst_items.xx_inst_items_var[1].item_base_def,xreg_esp)==0)
		{
			num++;
			_Addtolist(0,1,"ip:[%08x] [%s]",ip,xx_inst.xx_inst_mic.disasm);
			
		}
#endif
		num++;
	}
	
	_Addtolist(0,1,"search api complete num:[%08x][%0d]",num,num);
	return num;
} 


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





