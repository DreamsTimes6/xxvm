// xx_vm64.cpp : 定义 DLL 应用程序的导出函数。
//

//#include "stdafx.h"
//#include "x64dbg.h"
#include <stdio.h>
#include "xx_plugin.h"
#include "xx_comm64.h"
#include "xxdisasm64.h"
#include "xx_iat64.h"
#include "analyze_cfg.h"
#include "xx_opdef3.h"
#include "st_slist.h"

#ifndef DLL_EXPORT
#define DLL_EXPORT extern "C" __declspec(dllexport)
#endif 


#pragma comment(lib,"x64dbg.lib")
#pragma comment(lib,"x64bridge.lib")

// menu identifiers



#define MENU_IAT_SEARCH         1
#define MENU_IAT_PRINT          2
#define MENU_IAT_PRINT_INVALID  3
#define MENU_IAT_REPLACE        4
#define MENU_IAT_REPLACE_ALL    5
#define MENU_IAT_FIX            6
#define MENU_IAT_LOG            7
#define MENU_IAT_NOTES          8
#define MENU_IAT_FREE           9
#define MENU_IAT_CFG            10



#define plugin_name   "xx_vm_iat_x64"

#define plugin_version  127

struct T_VM_IAT64
{
	ulong64 ip;     //vmcall ip
	int fix_type;  //vmcall type 1.call [iat]  2.mov reg,[iat] 3.jmp [iat]
	ulong64 fix_ip;  //vmcall fix ip
	ulong64 iat;     //api addr
	int iat_flag;  //0-invalid  1-valid
	//ulong ret;     //
	int ret_flag;  //1.retn  2.retn 4  3.retn 8
	char iat_sym[0x100]; //
	int reg_index;  //mov reg,[iat]  reg index
	ulong64 iat_addr; //
};

/*全局iat结构存储在数据链中*/
struct ST_SLIST *g_iat_slist = 0;
//struct T_VM_IAT *vm_iat = 0;
int iat_num = 0;

ulong64 iat_base = 0;

int pluginHandle;
HWND hwndDlg;
int hMenu;
int hMenuDisasm;
int hMenuDump;
int hMenuStack;
int g_inst_count = 0;





char *g_pstack = 0; /*虚拟栈*/
////////////////////////////////////////////////////////////////////////////////

extern "C" {

	extern unsigned long long code_start;
	extern unsigned long long code_end;
	extern unsigned long long vmp0_start;
	extern unsigned long long vmp0_end;

	extern int pause_flag;
	//extern struct XX_CONTEXT start_context;
	extern int xdbg_gen_fmem();
	//extern int get_inst_context_test(struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context);
	extern int xx_start_execute(struct XX_CONTEXT64 *init_context, struct XX_CONTEXT64 *out_context, int *out_type);
	///////////////////////////////////////////////////////////////////////////

	////////////////
	extern HINSTANCE hinst;
	//extern int xx_start_saly();
	//extern void xx_saly_stop();
	extern struct XX_MOD xx_mod;

	extern char log_file[];
	extern char opmz_log[];
	extern char opmz_file[];
	extern char opmz_dfile[];
	extern char exp_file[];

	extern int init_mem_file(char *);
	extern int xx_init64(char* mainname);
	extern char xdbg_path[0x400];
	extern int g_inst_num;

	extern int init_mem_file(char *);
	extern int free_mem_file();

	extern int xx_read_fmem(unsigned long long  addr, char* pdata, unsigned int datasize);

	extern int   cfg_win_create(HWND hWnd);

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int xx_vm_iat();
int judge_vmcall_type(struct XX_CONTEXT64 *old_reg, struct XX_CONTEXT64 *curt_reg, struct T_VM_IAT64* pvm_iat);
int search_vmcall(ulong64 codestart, ulong64 codeend, ulong64 vmp0start, ulong64 vmp0end, struct ST_SLIST* piat_slist);
void print_iat();
void print_invalid_iat();
void iat_free();
int log_vmcall();
void notes_vmcall();
int fix_vmcall();
void *iat_base_alloc();
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hinst = (HINSTANCE)hModule;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}



/*====================================================================================
  pluginit - Called by debugger when plugin.dp32 is loaded - needs to be EXPORTED

  Arguments: initStruct - a pointer to a PLUG_INITSTRUCT structure

  Notes:     you must fill in the pluginVersion, sdkVersion and pluginName members.
			 The pluginHandle is obtained from the same structure - it may be needed in
			 other function calls.

			 you can call your own setup routine from within this function to setup
			 menus and commands, and pass the initStruct parameter to this function.

--------------------------------------------------------------------------------------*/
DLL_EXPORT bool pluginit(PLUG_INITSTRUCT* initStruct)
{
	int iret = 0;
	char *p = 0;


	initStruct->pluginVersion = plugin_version;
	initStruct->sdkVersion = PLUG_SDKVERSION;
	strcpy(initStruct->pluginName, plugin_name);
	pluginHandle = initStruct->pluginHandle;

	// place any additional initialization code here

	//memset(log_file, 0, sizeof(log_file));
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

	p = strstr(xdbg_path, "plugins");
	if (p)
	{
		memset(p, 0, strlen(p));
	}


	/*初始化分析配置文件*/
	xx_cfg_init(xdbg_path);

	/*授权*/

	return true;
}


/*====================================================================================
  plugstop - Called by debugger when the plugin.dp32 is unloaded - needs to be EXPORTED

  Arguments: none

  Notes:     perform cleanup operations here, clearing menus and other housekeeping

--------------------------------------------------------------------------------------*/
DLL_EXPORT bool plugstop()
{
	_plugin_menuclear(hMenu);

	// place any cleanup code here
	//MessageBox(0, "stop", "info", MB_OK);

	return true;
}


/*====================================================================================
  plugsetup - Called by debugger to initialize your plugins setup - needs to be EXPORTED

  Arguments: setupStruct - a pointer to a PLUG_SETUPSTRUCT structure

  Notes:     setupStruct contains useful handles for use within x64_dbg, mainly Qt
			 menu handles (which are not supported with win32 api) and the main window
			 handle with this information you can add your own menus and menu items
			 to an existing menu, or one of the predefined supported right click
			 context menus: hMenuDisam, hMenuDump & hMenuStack

			 plugsetup is called after pluginit.
--------------------------------------------------------------------------------------*/
DLL_EXPORT void plugsetup(PLUG_SETUPSTRUCT* setupStruct)
{
	hwndDlg = setupStruct->hwndDlg;
	hMenu = setupStruct->hMenu;
	hMenuDisasm = setupStruct->hMenuDisasm;
	hMenuDump = setupStruct->hMenuDump;
	hMenuStack = setupStruct->hMenuStack;

	//GuiAddLogMessage, sz$safeprojectname$Info;
	// place any additional setup code here

	_plugin_menuaddentry(hMenu, MENU_IAT_SEARCH, "search vmiat");
	_plugin_menuaddentry(hMenu, MENU_IAT_PRINT, "print");
	_plugin_menuaddentry(hMenu, MENU_IAT_PRINT_INVALID, "print invalid iat");
	//_plugin_menuaddentry(hMenu, MENU_IAT_REPLACE, "test");
	//_plugin_menuaddentry(hMenu, MENU_IAT_REPLACE, "replace invalid iat");
	//_plugin_menuaddentry(hMenu, MENU_IAT_REPLACE_ALL, "replace all invalid iat");
	_plugin_menuaddentry(hMenu, MENU_IAT_FIX, "fix vmiat");
	_plugin_menuaddentry(hMenu, MENU_IAT_LOG, "log vmiat");
	_plugin_menuaddentry(hMenu, MENU_IAT_NOTES, "notes vmiat");
	_plugin_menuaddentry(hMenu, MENU_IAT_FREE, "free");
	_plugin_menuaddentry(hMenu, MENU_IAT_CFG, "cfg");
}


DLL_EXPORT void CBDEBUGEVENT(CBTYPE cbType, PLUG_CB_DEBUGEVENT* info)
{
	/*虚拟栈在调试进程，不用释放*/
	if (info->DebugEvent->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
	{
		g_pstack = 0;
		//g_pstack = (char*)VirtualAllocEx((HANDLE)DbgGetProcessHandle(), 0, 0x1000, MEM_COMMIT, PAGE_READWRITE);
		g_pstack = (char*)VirtualAllocEx((HANDLE)info->DebugEvent->u.CreateProcessInfo.hProcess, \
			0, 0x1000, MEM_COMMIT, PAGE_READWRITE);
		if (g_pstack == 0)
		{
			MessageBox(0, "VAL_HPROCESS error", "error", MB_OK);
		}
	}
	else if (info->DebugEvent->dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
	{
		if (g_iat_slist)
		{
			xx_slist_free(g_iat_slist);
			iat_num = 0;
			g_iat_slist = 0;
		}
	}

}

/*====================================================================================
  CBMENUENTRY - Called by debugger when a menu item is clicked - needs to be EXPORTED

  Arguments: cbType
			 cbInfo - a pointer to a PLUG_CB_MENUENTRY structure. The hEntry contains
			 the resource id of menu item identifiers

  Notes:     hEntry can be used to determine if the user has clicked on your plugins
			 menu item(s) and to do something in response to it.

--------------------------------------------------------------------------------------*/
DLL_EXPORT void  CBMENUENTRY(CBTYPE cbType, PLUG_CB_MENUENTRY* info)
{
	char tmp[0x200];
	int iret = 0;
	SELECTIONDATA sel;


	switch (info->hEntry)
	{

	case MENU_IAT_SEARCH:
		/*search vmiat*/
		//xx_vm_iat();
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)xx_vm_iat, 0, 0, 0);
		break;
	case MENU_IAT_PRINT:
		/*print*/
		print_iat();
		break;
	case MENU_IAT_PRINT_INVALID:
		/*print invalid iat*/
		print_invalid_iat();
		break;
	case MENU_IAT_REPLACE:
		/*replace invalid iat*/
		//iat_base_alloc();
		MessageBox(0, "Developping", "info", MB_OK);
		break;
	case MENU_IAT_REPLACE_ALL:
		/*replace all invalid iat*/
		MessageBox(0, "Developping", "info", MB_OK);
		break;
	case MENU_IAT_FIX:
		/*fix vmiat*/
		//fix_vmcall();
		MessageBox(0, "Developping", "info", MB_OK);
		break;
	case MENU_IAT_LOG:
		/*log vmiat*/
		//log_vmcall();
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)log_vmcall, 0, 0, 0);
		break;
	case MENU_IAT_NOTES:
		/*notes vmiat*/
		//notes_vmcall();
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)notes_vmcall, 0, 0, 0);
		break;
	case MENU_IAT_FREE:
		/*free*/
		iat_free();
		break;
	case MENU_IAT_CFG:
		/*cfg*/
		cfg_win_create(hwndDlg);
		break;

	}
}



/*
功能
搜索vm_call，并建立iat结构
*/
__declspec(noinline)  int xx_vm_iat()
{
	int threadid;
	int iret = 0;
	struct XX_CONTEXT64 in_context;
	struct XX_CONTEXT64 out_context;
	struct T_VM_IAT64 *pvm_iat = 0;
	char tmp[200];
	struct ST_SLIST_NODE *pnode = 0;
	int n = 0;
	Script::Module::ModuleInfo modinfo;


	/*获取当前模块*/
	iret = DbgIsRunning();
	if (iret == 0)
	{
		memset(&modinfo, 0, sizeof(modinfo));
		iret = Script::Module::InfoFromAddr(Script::Register::GetRIP(), &modinfo);
		if (iret == 0)
		{
			/*如果没有模块，则取内存范围*/
			modinfo.base = Script::Memory::GetBase(Script::Register::GetRIP(), 0, 1);
			if (modinfo.base == 0)
			{
				_plugin_logputs("GetBase");
			}

			modinfo.size = Script::Memory::GetSize(Script::Register::GetRIP(), 0, 1);
			if (modinfo.size == 0)
			{
				_plugin_logputs("GetSize");
			}

		}

		if (strlen(modinfo.name) == 0)
		{
			/*无模块或者模块无名称时，取内存基地址作为名称*/
			sprintf(modinfo.name, "m%llx", modinfo.base);
		}
	}
	else
	{
		sprintf(modinfo.name, "xx_iat");
	}

	memset(&xx_mod, 0, sizeof(xx_mod));
	memcpy(xx_mod.name, modinfo.name, strlen(modinfo.name));
	xx_mod.base = modinfo.base;
	xx_mod.size = modinfo.size;



	/*暂停状态，防止内存发生变化*/
	_plugin_logprintf("code_start:[%llx],code_end:[%llx],vmp0_start:[%llx],vmp0_end:[%llx]\r\n", \
		code_start, code_end, vmp0_start, vmp0_end);

	/*检查iat数据是否已经存在*/
	if (g_iat_slist)
	{
		xx_slist_free(g_iat_slist);
		iat_num = 0;
		g_iat_slist = 0;
	}

	_plugin_logprintf("xx_iat  start\r\n");

	/*初始化内存*/
	iret = init_mem_file(xdbg_path);
	if (iret == 0)
	{
		MessageBox(0, "init error", "error", MB_OK);
		return 0;
	}
	_plugin_logprintf("init success\r\n");


	/*申请g_iat_slist*/
	g_iat_slist = (struct ST_SLIST*)xx_slist_new();
	if (g_iat_slist == 0)
	{
		MessageBox(0, "g_iat_slist", "error", MB_OK);
		return 0;
	}


	iat_num = search_vmcall(code_start, code_end, vmp0_start, vmp0_end, g_iat_slist);
	if (iat_num <= 0)
	{
		_plugin_logprintf("search_vmcall 0 error\r\n");
		return -1;
	}
	
	_plugin_logprintf("=====search complete=====\r\n");

	memset(&in_context, 0, sizeof(struct XX_CONTEXT64));
	in_context.r[REG_RSP] = (ulong64)(g_pstack + 0x500);
	in_context.r[REG_RBP] = (ulong64)(g_pstack + 0x500);

	_plugin_logprintf("start----iat  total num [%x] [%x]----\r\n", iat_num, g_iat_slist->total);

	//return 1;

	pnode = g_iat_slist->pstart;
	if (pnode != 0)
	{
		while (1)
		{
			pvm_iat = (struct T_VM_IAT64 *)pnode->pdata;

			/*循环获取iat结构数据*/
			in_context.ip = pvm_iat->ip;

			memset(&out_context, 0, sizeof(out_context));
			iret = xx_start_execute(&in_context, &out_context, &pvm_iat->ret_flag);
			if (iret == 0)
			{
				/*执行出错，下一个iat*/
				_plugin_logprintf("execute error ip:[%016llx]  ip:[%016llx]\r\n", in_context.ip, out_context.ip);
				//return 0;
				pvm_iat->iat_flag = 0;
				pvm_iat->fix_type = 0;
				//continue;
			}
			else
			{


				iret = judge_vmcall_type(&in_context, &out_context, pvm_iat);
				if (iret == 0)
				{
					_plugin_logprintf("vmcall_type error ip:[%016llx]  ip:[%016llx]\r\n", in_context.ip, out_context.ip);
					pvm_iat->iat_flag = 0;
					pvm_iat->fix_type = 0;
					//return 0;
				}
				else
				{
					_plugin_logprintf("[%016llx] ip:[%016llx]--get iat data\r\n", n, in_context.ip);
				}

			}


#if 0
			_plugin_logprintf("seq:[%08x]  in_context.ip:[%llx] out_context.ip:[%llx] ret_flag:[%08x]\r\n", pnode->seq, in_context.ip, out_context.ip, pvm_iat->ret_flag);

			_plugin_logprintf("in rax:[%llx] rcx:[%llx] rdx:[%llx] rbx:[%llx] rbp:[%llx] rsp:[%llx] rsi:[%llx] rdi:[%llx]\r\n", \
				in_context.r[REG_RAX], in_context.r[REG_RCX], in_context.r[REG_RDX], in_context.r[REG_RBX], \
				in_context.r[REG_RBP], in_context.r[REG_RSP], in_context.r[REG_RSI], in_context.r[REG_RDI]);
			_plugin_logprintf("in r8:[%llx] r9:[%llx] r10:[%llx] r11:[%llx] r12:[%llx] r13:[%x] r14:[%llx] r15:[%llx]\r\n", \
				in_context.r[REG_R8], in_context.r[REG_R9], in_context.r[REG_R10], in_context.r[REG_R11], \
				in_context.r[REG_R12], in_context.r[REG_R13], in_context.r[REG_R14], in_context.r[REG_R15]);


			_plugin_logprintf("in rax:[%llx] rcx:[%llx] rdx:[%llx] rbx:[%llx] rbp:[%llx] rsp:[%llx] rsi:[%llx] rdi:[%llx]\r\n", \
				out_context.r[REG_RAX], out_context.r[REG_RCX], out_context.r[REG_RDX], out_context.r[REG_RBX], \
				out_context.r[REG_RBP], out_context.r[REG_RSP], out_context.r[REG_RSI], out_context.r[REG_RDI]);
			_plugin_logprintf("in r8:[%llx] r9:[%llx] r10:[%llx] r11:[%llx] r12:[%llx] r13:[%x] r14:[%llx] r15:[%llx]\r\n", \
				out_context.r[REG_R8], out_context.r[REG_R9], out_context.r[REG_R10], out_context.r[REG_R11], \
				out_context.r[REG_R12], out_context.r[REG_R13], out_context.r[REG_R14], out_context.r[REG_R15]);
#endif


			if (pnode->pnext == 0)
			{
				break;
			}
			pnode = (struct ST_SLIST_NODE *)pnode->pnext;

		}
	}

	_plugin_logprintf("finish----iat  total num [%08x][%d]----\r\n", iat_num, iat_num);
	MessageBox(0, "iat finish", "success", MB_OK);
	return 1;
}






__declspec(noinline)  int search_vmcall(ulong64 codestart, ulong64 codeend, ulong64 vmp0start, ulong64 vmp0end, struct ST_SLIST* piat_slist)
{
	/*搜索所有的vmcall*/
	unsigned char tmp[200];
	unsigned char cmdbuf[200];

	int iret = 0;
	ulong64 ip = 0;
	struct XX_INST xx_inst;
	int num = 0;
	int cmdlen = 0;       //cmd长度
	ulong64 base = 0;       //模块基地址
	ulong64 des_addr = 0;  //vmcall目的地址
	struct T_VM_IAT64 t_vm_iat;

	/*base暂时为0*/
	base = 0;

	ip = codestart - 1;
	num = 0;
	while (ip < codeend)
	{
		ip = ip + 1;

		//_plugin_logprintf("search_vmcall ip:[%llx]\r\n", ip);

		//xx_inst=malloc(sizeof(struct XX_INST)*100);
		memset(&xx_inst, 0, sizeof(struct XX_INST));

		//_Addtolist(0,1,"ip:[%08x]",ip);
		memset(cmdbuf, 0, sizeof(cmdbuf));
		iret = DbgMemRead(ip, cmdbuf, INST_MAX_LEN);
		if (iret == 0)
		{
			_plugin_logprintf("DbgMemRead 1 [%llx] error\r\n", ip);
			return 0;
		}

		if (*cmdbuf != 0xe8)
		{
			continue;
		}

		xx_inst.xx_inst_code.inst_system = INST_SYSTEM_CURT;
		memcpy(xx_inst.xx_inst_code.base, (char*)&base, xx_inst.xx_inst_code.inst_system);
		memcpy(xx_inst.xx_inst_code.current_addr, (char*)&ip, xx_inst.xx_inst_code.inst_system);

		xx_disasm(&xx_inst, cmdbuf);
		//_Addtolist(0,1,"finish_flag:[%d]",xx_inst[n].xx_inst_exist.finish_flag);

		if (xx_inst.xx_inst_exist.finish_flag != INST_FLAG_EXIST)
		{
			_plugin_logprintf("xx_disasm error ip:[%llx]\r\n", ip);
			continue;
		}

		if (memcmp(xx_inst.xx_inst_code.opcode_type, sop_call, sop_len) != 0 ||
			xx_inst.xx_inst_items.xx_inst_items_flag[0].item_type != INST_ITEM_TYPE_DIS)
		{
			continue;
		}

		des_addr = *(ulong64*)xx_inst.xx_inst_items.xx_inst_items_var[0].item_const;
		/*目的地址在vmp0段,也有可能在text段*/
		if (des_addr < codestart || des_addr > vmp0end)
		{
			continue;
		}

		memset(tmp, 0, sizeof(tmp));
		cmdlen = DbgMemRead(des_addr, (char*)tmp, INST_MAX_LEN);
		if (cmdlen == 0)
		{
			_plugin_logprintf("DbgMemRead 2 [%016llx] error\r\n", des_addr);
			return -1;
		}

		if (tmp[0] == 0x90 && tmp[1] != 0x90)
		{
			/*确定是vmcall*/
			_plugin_logprintf("=======vmcall [%016llx][%s]\r\n", ip, xx_inst.xx_inst_mic.disasm);

			memset(&t_vm_iat, 0, sizeof(t_vm_iat));
			t_vm_iat.ip = ip;
			iret = xx_slist_insert_data(piat_slist, &t_vm_iat, sizeof(t_vm_iat));
			if (iret == 0)
			{
				_plugin_logprintf("insert iat error\r\n");
				return -1;
			}
			num++;

		}


	}

	_plugin_logprintf("search complete num:[%08x][%0d]\r\n", num, num);
	return num;
}




/*
判断vmcall类型
*/
__declspec(noinline)  int judge_vmcall_type(struct XX_CONTEXT64 *old_reg, struct XX_CONTEXT64 *curt_reg, struct T_VM_IAT64* pvm_iat)
{
	ulong iret = 0;
	ulong64 ret_var = 0;
	ulong64 callret = 0;
	int n = 0;
	int m = 0;
	int chgnum = 0;
	


	callret = pvm_iat->ip + 5;

	pvm_iat->fix_ip = 0;


	/*2.mov reg,[iat] pop;mov reg,[iat]   [sp]=callret curt_sp=sp-8   retn
					  push;mov reg,[iat]  [sp]=callret curt_sp=sp    retn
					  mov reg,[iat];ret   [sp]=callret+1  curt_sp=sp-4  retn
	*/
	if (pvm_iat->ret_flag == 1)
	{
		//iret=get_mem_value(curt_reg->r[REG_RSP]);
		ret_var = 0;
		iret = xx_read_fmem(curt_reg->r[REG_RSP], (char*)&ret_var, sizeof(ret_var));
		if (iret == 0)
		{
			_plugin_logprintf("error:[%016llx] mem data\r\n", curt_reg->r[REG_RSP]);
			return 0;
		}
		//_plugin_logprintf("ret_var:[%016llx]\r\n", ret_var);

		//if (ret_var == callret && curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x8))
		if (ret_var == callret && curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x10))
		{
			pvm_iat->fix_ip = pvm_iat->ip - 1;
		}
		else if (ret_var == callret && curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP]))
		{
			pvm_iat->fix_ip = pvm_iat->ip - 1;
		}
		//else if (ret_var == (callret + 1) && curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x04))
		else if (ret_var == (callret + 1) && curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x08))
		{
			pvm_iat->fix_ip = pvm_iat->ip;
		}
		//else if (ret_var == (callret) && curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x04) && old_reg->r[REG_RAX] != curt_reg->r[REG_RAX])
		else if (ret_var == (callret) && curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x08) && old_reg->r[REG_RAX] != curt_reg->r[REG_RAX])
		{
			pvm_iat->iat_flag = DbgGetLabelAt(curt_reg->r[REG_RAX], SEG_CS, pvm_iat->iat_sym);
			if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
			{
				pvm_iat->fix_ip = pvm_iat->ip;
				pvm_iat->fix_type = 2;
				pvm_iat->iat = curt_reg->r[REG_RAX];
				pvm_iat->reg_index = REG_RAX;
				return 1;
			}
			memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			pvm_iat->iat_flag = 0;
		}

		if (pvm_iat->fix_ip)
		{
			pvm_iat->fix_type = 2;
			if (old_reg->r[REG_RAX] != curt_reg->r[REG_RAX])
			{
				pvm_iat->iat = curt_reg->r[REG_RAX];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_RAX;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_RBX] != curt_reg->r[REG_RBX])
			{
				pvm_iat->iat = curt_reg->r[REG_RBX];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_RBX;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_RCX] != curt_reg->r[REG_RCX])
			{
				pvm_iat->iat = curt_reg->r[REG_RCX];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_RCX;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_RDX] != curt_reg->r[REG_RDX])
			{
				pvm_iat->iat = curt_reg->r[REG_RDX];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_RDX;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_RSI] != curt_reg->r[REG_RSI])
			{
				pvm_iat->iat = curt_reg->r[REG_RSI];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_RSI;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_RDI] != curt_reg->r[REG_RDI])
			{
				pvm_iat->iat = curt_reg->r[REG_RDI];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_RDI;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_RBP] != curt_reg->r[REG_RBP])
			{
				pvm_iat->iat = curt_reg->r[REG_RBP];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_RBP;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_R8] != curt_reg->r[REG_R8])
			{
				pvm_iat->iat = curt_reg->r[REG_R8];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_R8;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_R9] != curt_reg->r[REG_R9])
			{
				pvm_iat->iat = curt_reg->r[REG_R9];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_R9;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_R10] != curt_reg->r[REG_R10])
			{
				pvm_iat->iat = curt_reg->r[REG_R10];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_R10;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_R11] != curt_reg->r[REG_R11])
			{
				pvm_iat->iat = curt_reg->r[REG_R11];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_R11;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_R12] != curt_reg->r[REG_R12])
			{
				pvm_iat->iat = curt_reg->r[REG_R12];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_R12;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_R13] != curt_reg->r[REG_R13])
			{
				pvm_iat->iat = curt_reg->r[REG_R13];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_R13;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_R14] != curt_reg->r[REG_R14])
			{
				pvm_iat->iat = curt_reg->r[REG_R14];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_R14;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			if (old_reg->r[REG_R15] != curt_reg->r[REG_R15])
			{
				pvm_iat->iat = curt_reg->r[REG_R15];
				pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
				if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
				{
					pvm_iat->reg_index = REG_R15;
					return 1;
				}
				memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
			}
			pvm_iat->iat = 0;
		}
	}

	/*1.call [iat]  pop;call    [sp]=iat,[sp+4]=callret,curt_sp=sp-4  retn
					push;call   [sp]=iat,[sp+4]=callret,curt_sp=sp-4  retn
					call;ret    [sp]=iat,[sp+4]=callret+1,curt_sp=sp-8 retn
	*/
	if (pvm_iat->ret_flag == 1)
	{
		//iret=get_mem_value(curt_reg->r[REG_RSP]+4);xx_read_fmem
		ret_var = 0;
		//iret = xx_read_fmem(curt_reg->r[REG_RSP] + 4, (char*)&ret_var, sizeof(ret_var));
		iret = xx_read_fmem(curt_reg->r[REG_RSP] + 8, (char*)&ret_var, sizeof(ret_var));
		if (iret == 0)
		{
			_plugin_logprintf("error:[%016llx] mem data\r\n", curt_reg->r[REG_RSP] + 8);
			return 0;
		}
		//_plugin_logprintf("ret_var:[%016llx]\r\n", ret_var);

		//if (ret_var == callret && curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x04))
		if (ret_var == callret && curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x08))
		{
			pvm_iat->fix_ip = pvm_iat->ip - 1;
		}
		//else if (ret_var == (callret + 1) && curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x08))
		else if (ret_var == (callret + 1) && curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x10))
		{
			pvm_iat->fix_ip = pvm_iat->ip;
		}

		if (pvm_iat->fix_ip)
		{
			pvm_iat->fix_type = 1;
			//pvm_iat->iat=get_mem_value(curt_reg->r[REG_RSP]);
			iret = xx_read_fmem(curt_reg->r[REG_RSP], (char*)&(pvm_iat->iat), sizeof(pvm_iat->iat));
			if (iret == 0)
			{
				_plugin_logprintf("error:[%016llx] mem data\r\n", curt_reg->r[REG_RSP]);
				return 0;
			}
			pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
			if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
			{
				return 1;
			}
			memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
		}

	}

	/*3.jmp [iat]    pop; jmp [iat]   [sp]=iat  [sp+4]=callret  curt_sp=sp-4  retn 4
					 push;jmp [iat]   [sp]=iat  [sp+4]=callret  curt_sp=sp-4  retn 4
					 jmp [iat];ret    [sp]=iat  [sp+4]=callret  curt_sp=sp-8  retn 4
	*/
	if (pvm_iat->ret_flag == 2)
	{
		//if (curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x04))
		if (curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x08))
		{
			pvm_iat->fix_ip = pvm_iat->ip - 1;
		}
		//else if (curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x08))
		else if (curt_reg->r[REG_RSP] == (old_reg->r[REG_RSP] - 0x10))
		{
			pvm_iat->fix_ip = pvm_iat->ip;
		}

		if (pvm_iat->fix_ip)
		{
			pvm_iat->fix_type = 3;
			//pvm_iat->iat=get_mem_value(curt_reg->r[REG_RSP]);
			iret = xx_read_fmem(curt_reg->r[REG_RSP], (char*)&(pvm_iat->iat), sizeof(pvm_iat->iat));
			if (iret == 0)
			{
				_plugin_logprintf("error:[%016llx] mem data\r\n", curt_reg->r[REG_RSP]);
				return 0;
			}
			//_plugin_logprintf("ret_var:[%016llx]\r\n", ret_var);
			pvm_iat->iat_flag = DbgGetLabelAt(pvm_iat->iat, SEG_CS, pvm_iat->iat_sym);
			if (pvm_iat->iat_flag != 0 && strcmp(pvm_iat->iat_sym, "EntryPoint") != 0)
			{
				return 1;
			}
			memset(pvm_iat->iat_sym, 0, sizeof(pvm_iat->iat_sym));
		}
	}

	_plugin_logprintf("vm_iat type error [%016llx] \r\n", pvm_iat->ip);

	return 0;
}





/*
功能
释放iat数据
*/
__declspec(noinline) void iat_free()
{
	if (g_iat_slist)
	{
		xx_slist_free(g_iat_slist);
		iat_num = 0;
		g_iat_slist = 0;
	}

	MessageBox(0, "iat free", "success", MB_OK);

	return;
}




/*
功能
打印iat数据结构
*/
__declspec(noinline) void print_iat()
{
	int n = 0;
	struct T_VM_IAT64 *pvm_iat = 0;
	struct ST_SLIST_NODE *pnode = 0;

	if (g_iat_slist == 0)
	{
		MessageBox(0, "iat is null", "error", MB_OK);
		return;
	}

	pnode = g_iat_slist->pstart;
	if (pnode != 0)
	{
		while (1)
		{
			pvm_iat = (struct T_VM_IAT64 *)pnode->pdata;
			n = pnode->seq;

			/*循环获取iat结构数据*/
			_plugin_logprintf("vm_iat[%x].ip [%016llx]\r\n", n, pvm_iat->ip);
			_plugin_logprintf("vm_iat[%08x].ret_flag [%x]\r\n", n, pvm_iat->ret_flag);
			_plugin_logprintf("vm_iat[%08x].fix_ip [%016llx]\r\n", n, pvm_iat->fix_ip);
			_plugin_logprintf("vm_iat[%08x].fix_type [%08x]\r\n", n, pvm_iat->fix_type);
			_plugin_logprintf("vm_iat[%08x].iat [%016llx]\r\n", n, pvm_iat->iat);
			_plugin_logprintf("vm_iat[%08x].iat_flag [%08x]\r\n", n, pvm_iat->iat_flag);
			_plugin_logprintf("vm_iat[%08x].iat_sym [%s]\r\n", n, pvm_iat->iat_sym);
			_plugin_logprintf("vm_iat[%08x].reg_index [%08x]\r\n", n, pvm_iat->reg_index);
			_plugin_logprintf("                        \r\n");

			if (pnode->pnext == 0)
			{
				break;
			}
			pnode = (struct ST_SLIST_NODE *)pnode->pnext;

		}
	}
	else
	{
		MessageBox(0, "iat is null", "error", MB_OK);
	}
	return;
}





/*
功能
打印无效的iat
*/
__declspec(noinline) void print_invalid_iat()
{
	int n = 0;
	struct T_VM_IAT64 *pvm_iat = 0;
	struct ST_SLIST_NODE *pnode = 0;

	if (g_iat_slist == 0)
	{
		MessageBox(0, "iat is null", "error", MB_OK);
		return;
	}

	pnode = g_iat_slist->pstart;
	if (pnode != 0)
	{
		while (1)
		{
			pvm_iat = (struct T_VM_IAT64 *)pnode->pdata;
			n = pnode->seq;

			/*循环获取iat结构数据*/
			if (pvm_iat->iat_flag == 0)
			{
				_plugin_logprintf("vm_iat[%x].ip [%016llx]\r\n", n, pvm_iat->ip);
				_plugin_logprintf("vm_iat[%08x].ret_flag [%x]\r\n", n, pvm_iat->ret_flag);
				_plugin_logprintf("vm_iat[%08x].fix_ip [%016llx]\r\n", n, pvm_iat->fix_ip);
				_plugin_logprintf("vm_iat[%08x].fix_type [%08x]\r\n", n, pvm_iat->fix_type);
				_plugin_logprintf("vm_iat[%08x].iat [%016llx]\r\n", n, pvm_iat->iat);
				_plugin_logprintf("vm_iat[%08x].iat_flag [%08x]\r\n", n, pvm_iat->iat_flag);
				_plugin_logprintf("vm_iat[%08x].iat_sym [%s]\r\n", n, pvm_iat->iat_sym);
				_plugin_logprintf("vm_iat[%08x].reg_index [%08x]\r\n", n, pvm_iat->reg_index);
				_plugin_logprintf("                        \r\n");

			}

			if (pnode->pnext == 0)
			{
				break;
			}
			pnode = (struct ST_SLIST_NODE *)pnode->pnext;

		}
	}
	else
	{
		MessageBox(0, "iat is null", "error", MB_OK);
	}
	return;
}









/*
功能
日志
*/
__declspec(noinline) int log_vmcall()
{
	int n = 0;

	char cmdbuf[200];
	ulong64 iat_base = 0;
	ulong64 offset = 0;
	int iret = 0;
	struct T_VM_IAT64 *pvm_iat = 0;
	struct ST_SLIST_NODE *pnode = 0;
	char tmp[400];

	if (g_iat_slist == 0)
	{
		MessageBox(0, "iat is null", "error", MB_OK);
		return 0;
	}


	xx_init64(xx_mod.name);
	

	if (strlen(log_file) == 0)
	{
		MessageBox(0, "Init Log error", "error", MB_OK);
		return 0;
	}

	iat_base = 0x10000000;

	xx_cover_file(log_file, (char*)"websit: www.microcode.com\r\n", strlen("websit: www.microcode.com\r\n"));

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "iat num:%08x  \r\n", iat_num);
	xx_append_file(log_file, tmp, strlen(tmp));

	/*log vmcall*/

	pnode = g_iat_slist->pstart;
	if (pnode != 0)
	{
		while (1)
		{
			pvm_iat = (struct T_VM_IAT64 *)pnode->pdata;
			n = pnode->seq;

			pvm_iat->iat_addr = iat_base + offset;
			offset = offset + sizeof(pvm_iat->iat_addr);

			if (pvm_iat->fix_type == 1)
			{
				/*call [iat]*/
				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf + strlen(cmdbuf), "call qword ptr ");
				sprintf(cmdbuf + strlen(cmdbuf), "[");
				sprintf(cmdbuf + strlen(cmdbuf), "0x%016llx", pvm_iat->iat_addr);
				sprintf(cmdbuf + strlen(cmdbuf), "]");
			}
			else if (pvm_iat->fix_type == 2)
			{
				/*mov reg,[iat]*/
				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf + strlen(cmdbuf), "mov ");
				switch (pvm_iat->reg_index)
				{
				case REG_RAX:
					sprintf(cmdbuf + strlen(cmdbuf), "rax,");
					break;
				case REG_RBX:
					sprintf(cmdbuf + strlen(cmdbuf), "rbx,");
					break;
				case REG_RCX:
					sprintf(cmdbuf + strlen(cmdbuf), "rcx,");
					break;
				case REG_RDX:
					sprintf(cmdbuf + strlen(cmdbuf), "rdx,");
					break;
				case REG_RBP:
					sprintf(cmdbuf + strlen(cmdbuf), "rbp,");
					break;
				case REG_RSI:
					sprintf(cmdbuf + strlen(cmdbuf), "rsi,");
					break;
				case REG_RDI:
					sprintf(cmdbuf + strlen(cmdbuf), "rdi,");
					break;
				case REG_R8:
					sprintf(cmdbuf + strlen(cmdbuf), "r8,");
					break;
				case REG_R9:
					sprintf(cmdbuf + strlen(cmdbuf), "r9,");
					break;
				case REG_R10:
					sprintf(cmdbuf + strlen(cmdbuf), "r10,");
					break;
				case REG_R11:
					sprintf(cmdbuf + strlen(cmdbuf), "r11,");
					break;
				case REG_R12:
					sprintf(cmdbuf + strlen(cmdbuf), "r12,");
					break;
				case REG_R13:
					sprintf(cmdbuf + strlen(cmdbuf), "r13,");
					break;
				case REG_R14:
					sprintf(cmdbuf + strlen(cmdbuf), "r14,");
					break;
				case REG_R15:
					sprintf(cmdbuf + strlen(cmdbuf), "r15,");
					break;

				default:
					_plugin_logprintf("fix_type 2 error index:[%08x] ip:[%016llx]", n, pvm_iat->ip);
					//continue;
				}
				sprintf(cmdbuf + strlen(cmdbuf), "qword ptr ");
				sprintf(cmdbuf + strlen(cmdbuf), "[");
				sprintf(cmdbuf + strlen(cmdbuf), "0x%016llx", pvm_iat->iat_addr);
				sprintf(cmdbuf + strlen(cmdbuf), "]");
			}
			else if (pvm_iat->fix_type == 3)
			{
				/*jmp [iat]*/
				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf + strlen(cmdbuf), "jmp qword ptr ");
				sprintf(cmdbuf + strlen(cmdbuf), "[");
				sprintf(cmdbuf + strlen(cmdbuf), "0x%016llx", pvm_iat->iat_addr);
				sprintf(cmdbuf + strlen(cmdbuf), "]");
			}
			else
			{
				_plugin_logprintf("fix_type  error index:[%08x] ip:[%016llx]", n, pvm_iat->ip);
				//return 0;
				//continue;
			}

			/*写入日志*/
			if (strlen(cmdbuf) != 0)
			{
				memset(tmp, 0, sizeof(tmp));
				sprintf(tmp, "%016llx  < %s >   [ %s ]\r\n", pvm_iat->ip, cmdbuf, pvm_iat->iat_sym);
				xx_append_file(log_file, tmp, strlen(tmp));
			}


			if (pnode->pnext == 0)
			{
				break;
			}
			pnode = (struct ST_SLIST_NODE *)pnode->pnext;

		}
	}
	else
	{
		MessageBox(0, "iat is null", "error", MB_OK);
	}

	MessageBox(0, "log finish", "success", MB_OK);

	return 1;
}







/*
功能
只注释api
*/
__declspec(noinline) void notes_vmcall()
{
	int n = 0;
	int iret = 0;
	char snote[] = "API: ";
	char tmp[200];
	struct T_VM_IAT64 *pvm_iat = 0;
	struct ST_SLIST_NODE *pnode = 0;

	if (g_iat_slist == 0)
	{
		MessageBox(0, "iat is null", "error", MB_OK);
		return;
	}


	pnode = g_iat_slist->pstart;
	if (pnode != 0)
	{
		while (1)
		{
			pvm_iat = (struct T_VM_IAT64 *)pnode->pdata;
			n = pnode->seq;

			/*循环获取iat结构数据*/
			sprintf(tmp, "%s%s", snote, pvm_iat->iat_sym);

			DbgSetCommentAt(pvm_iat->ip, tmp);

			if (pnode->pnext == 0)
			{
				break;
			}
			pnode = (struct ST_SLIST_NODE *)pnode->pnext;
		}
	}
	else
	{
		MessageBox(0, "iat is null", "error", MB_OK);
	}

	MessageBox(0, "notes finish", "success", MB_OK);

	return;
}






/*
功能
修复vmcall
*/
__declspec(noinline) int fix_vmcall()
{
	int n = 0;
	char cmdbuf[200];
	HANDLE hp = 0;
	ulong64 offset = 0;
	int iret = 0;
	char tmp[400];
	struct T_VM_IAT64 *pvm_iat = 0;
	struct ST_SLIST_NODE *pnode = 0;

	if (g_iat_slist == 0)
	{
		MessageBox(0, "iat is null", "error", MB_OK);
		return 0;
	}


	hp = (HANDLE)DbgGetProcessHandle();

	if (iat_base != 0)
	{
		VirtualFreeEx(hp, (void*)iat_base, 0, MEM_DECOMMIT);
		iat_base = 0;

	}

	/*根据输入的地址范围获取模块地址或者内存块地址*/

	/*申请一个内存,存放iat的地址，64位call32位偏移地址，在模块或者内部块尾部申请空间*/
	iat_base = (ulong64)VirtualAllocEx(hp, 0, iat_num*(sizeof(ulong64)), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (iat_base == 0)
	{
		_plugin_logprintf("VirtualAllocEx error\r\n");
		return 0;
	}

	offset = 0;

	pnode = g_iat_slist->pstart;
	if (pnode != 0)
	{
		while (1)
		{
			pvm_iat = (struct T_VM_IAT64 *)pnode->pdata;
			n = pnode->seq;

			/*循环获取iat结构数据*/
			iret = DbgMemWrite(iat_base + offset, &pvm_iat->iat, sizeof(pvm_iat->iat));
			if (iret == 0)
			{
				//VirtualFreeEx(hp, (void*)iat_base, 0, MEM_DECOMMIT);
				//iat_base = 0;
				//_plugin_logprintf("DbgMemWrite error\r\n");
				_plugin_logprintf("DbgMemWrite error index:[%08x] addr:[%016llx] iat:[%016llx]\r\n", n, iat_base + offset, pvm_iat->iat);
				return 0;
			}
			pvm_iat->iat_addr = iat_base + offset;
			offset = offset + sizeof(pvm_iat->iat);


			if (pvm_iat->fix_type == 1)
			{
				/*call [iat]*/
				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf + strlen(cmdbuf), "call qword ptr ");
				sprintf(cmdbuf + strlen(cmdbuf), "[");
				sprintf(cmdbuf + strlen(cmdbuf), "0x%016llx", pvm_iat->iat_addr);
				sprintf(cmdbuf + strlen(cmdbuf), "]");
			}
			else if (pvm_iat->fix_type == 2)
			{
				/*mov reg,[iat]*/
				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf + strlen(cmdbuf), "mov ");
				switch (pvm_iat->reg_index)
				{
				case REG_RAX:
					sprintf(cmdbuf + strlen(cmdbuf), "rax,");
					break;
				case REG_RBX:
					sprintf(cmdbuf + strlen(cmdbuf), "rbx,");
					break;
				case REG_RCX:
					sprintf(cmdbuf + strlen(cmdbuf), "rcx,");
					break;
				case REG_RDX:
					sprintf(cmdbuf + strlen(cmdbuf), "rdx,");
					break;
				case REG_RBP:
					sprintf(cmdbuf + strlen(cmdbuf), "rbp,");
					break;
				case REG_RSI:
					sprintf(cmdbuf + strlen(cmdbuf), "rsi,");
					break;
				case REG_RDI:
					sprintf(cmdbuf + strlen(cmdbuf), "rdi,");
					break;
				case REG_R8:
					sprintf(cmdbuf + strlen(cmdbuf), "r8,");
					break;
				case REG_R9:
					sprintf(cmdbuf + strlen(cmdbuf), "r9,");
					break;
				case REG_R10:
					sprintf(cmdbuf + strlen(cmdbuf), "r10,");
					break;
				case REG_R11:
					sprintf(cmdbuf + strlen(cmdbuf), "r11,");
					break;
				case REG_R12:
					sprintf(cmdbuf + strlen(cmdbuf), "r12,");
					break;
				case REG_R13:
					sprintf(cmdbuf + strlen(cmdbuf), "r13,");
					break;
				case REG_R14:
					sprintf(cmdbuf + strlen(cmdbuf), "r14,");
					break;
				case REG_R15:
					sprintf(cmdbuf + strlen(cmdbuf), "r15,");
					break;

				default:
					_plugin_logprintf("fix_type 2 error index:[%08x] ip:[%016llx]", n, pvm_iat->ip);
					//continue;
				}
				sprintf(cmdbuf + strlen(cmdbuf), "qword ptr ");
				sprintf(cmdbuf + strlen(cmdbuf), "[");
				sprintf(cmdbuf + strlen(cmdbuf), "0x%016llx", pvm_iat->iat_addr);
				sprintf(cmdbuf + strlen(cmdbuf), "]");
			}
			else if (pvm_iat->fix_type == 3)
			{
				/*jmp [iat]*/
				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf + strlen(cmdbuf), "jmp qword ptr ");
				sprintf(cmdbuf + strlen(cmdbuf), "[");
				sprintf(cmdbuf + strlen(cmdbuf), "0x%016llx", pvm_iat->iat_addr);
				sprintf(cmdbuf + strlen(cmdbuf), "]");
			}
			else
			{
				_plugin_logprintf("fix_type  error index:[%08x] ip:[%016llx]", n, pvm_iat->ip);
				//return 0;
				//continue;
			}


			if (strlen(cmdbuf) != 0)
			{
				iret = DbgAssembleAt(pvm_iat->fix_ip, cmdbuf);
				if (iret == 0)
				{
					//VirtualFreeEx(hp, (void*)iat_base, 0, MEM_DECOMMIT);
					//iat_base = 0;
					_plugin_logprintf("DbgAssembleAt [%016llx][%s] error\r\n", pvm_iat->fix_ip, cmdbuf);
					//continue;
				}
			}


			if (pnode->pnext == 0)
			{
				break;
			}
			pnode = (struct ST_SLIST_NODE *)pnode->pnext;
		}
	}
	else
	{
		MessageBox(0, "iat is null", "error", MB_OK);
	}

	//VirtualFreeEx(hp,(void*)iat_base,0,MEM_DECOMMIT);
	MessageBox(0, "fix finish", "success", MB_OK);
	return 1;
}





/*
申请iat空间
*/
void *iat_base_alloc()
{
	int iret = 0;
	void* piat_base = 0;
	HANDLE hp = 0;
	ulong64 base_mem = 0;
	ulong64 addr_mem = 0;
	ulong64 size_mem = 0;
	MEMORY_BASIC_INFORMATION mi;

	hp = (HANDLE)DbgGetProcessHandle();
	if (hp == 0)
	{
		_plugin_logprintf("DbgGetProcessHandle error\r\n");
		return 0;
	}

	/*获取目标模块或者内存块的base，size*/
	addr_mem = DbgMemFindBaseAddr(code_start, &size_mem);

	_plugin_logprintf("iat_base_alloc addr_mem:[%016llx] size_mem:[%016llx]\r\n", addr_mem, size_mem);
	while (1)
	{
		memset(&mi, 0, sizeof(mi));
		iret = VirtualQueryEx(hp, (LPVOID)addr_mem, &mi, sizeof(mi));
		if (iret == 0)
		{
			break;
		}
		_plugin_logprintf("VirtualQueryEx AllocationBase:[%016llx] BaseAddress:[%016llx] RegionSize:[%016llx] Protect:[%016llx] State:[%016llx] Type:[%016llx]\r\n",\
			mi.AllocationBase, mi.BaseAddress, mi.RegionSize, mi.Protect, mi.State, mi.Type);
		
		if (mi.AllocationBase==0)
		{
			_plugin_logprintf("BaseAddress:[%016llx] \r\n", mi.BaseAddress);
			break;
		}

		addr_mem = (ulong64)mi.BaseAddress + (ulong64)mi.RegionSize;
	}

	_plugin_logprintf("addr_mem:[%016llx] \r\n", addr_mem);

	//addr_mem = (addr_mem & 0xffffffffffff0000) + 0x10000;

	//_plugin_logprintf("addr_mem:[%016llx] \r\n", addr_mem);

	/*在尾部申请空间*/
	piat_base = VirtualAllocEx(hp, (LPVOID)addr_mem, iat_num*(sizeof(ulong64)), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (piat_base == 0)
	{
		_plugin_logprintf("VirtualAllocEx error\r\n");
		 
		//iret = VirtualFreeEx(hp, (LPVOID)addr_mem, 0, MEM_RELEASE);

		//_plugin_logprintf("VirtualFreeEx addr_mem:[%016llx] iret:[%x]\r\n", addr_mem, iret);

		return 0;
	}

	_plugin_logprintf("piat_base:[%016llx] size:[%x]\r\n", piat_base, iat_num*(sizeof(ulong64)));

	return piat_base;
}

