// xx_vm64.cpp : 定义 DLL 应用程序的导出函数。
//

//#include "stdafx.h"
//#include "x64dbg.h"

#include "xx_plugin.h"
#include "xx_comm32.h"
#include "xx_vm.h"
#include "analyze_cfg.h"
#include "vmp38_restore.h"
#pragma comment(lib, "comdlg32.lib")


#ifndef DLL_EXPORT
#define DLL_EXPORT extern "C" __declspec(dllexport)
#endif 


#pragma comment(lib,"x32dbg.lib")
#pragma comment(lib,"x32bridge.lib")

// menu identifiers


#define MENU_VMP38_RESTORE     16
#define MENU_VMP38_DYN         18
#define MENU_VMP38_SCAN       19
#define MENU_VMP38_STOP       21
#define MENU_VMP38_ANALYZE    22
#define MENU_VMP38_APPENDS   23


#define plugin_name   "xx_vm"

#define plugin_version  127




int pluginHandle;
int g_menu_dyn = 0;
int g_menu_appends = 0;
int g_dyn_running = 0;
volatile int g_dyn_stop = 0;
int g_scan_running = 0;
unsigned long long g_dyn_vm_entry = 0;
static unsigned long long g_ft_entry = 0;
static unsigned long long g_ft_esp = 0;   /* full-trace entry ESP (param injection) */
DWORD WINAPI vmp38_dyn_thread(LPVOID lpParam);

/* Headless command callbacks: the menu titles are Chinese (UTF-8), which
   x64dbg rewrites to "menu______________N" in headless - unusable. Register
   stable ASCII commands (vmp38_restore / _dyn / _scan / _stop / _analyze)
   that forward to the same CBMENUENTRY switch so headless .cf scripts work. */
static bool cb_vmp38_restore(int argc, char** argv);
static bool cb_vmp38_dyn(int argc, char** argv);
static bool cb_vmp38_scan(int argc, char** argv);
static bool cb_vmp38_stop(int argc, char** argv);
static bool cb_vmp38_analyze(int argc, char** argv);
static bool cb_vmp38_revm(int argc, char** argv);

DWORD WINAPI vmp38_fulltrace_thread(LPVOID lpParam)
{
	(void)lpParam;
	unsigned long long ft_last = 0;
	if (g_ft_entry)
	{
		/* direct-to-file extraction: the trace writes the analyzer data file
		   itself - no post-trace restore pass (the standalone analyzer
		   derives handler semantics from the data) */
		vmp38_unicorn_full_trace(g_ft_entry, g_ft_esp, &ft_last);
	}
	g_dyn_running = 0;
	return 0;
}

DWORD WINAPI vmp38_scan_thread(LPVOID lpParam);
int hMenu;

/* analyze runs on a background thread so the GUI stays responsive
   while a huge .data.txt (10M+ lines) is being analyzed */
static int g_an_running = 0;
static char g_an_file[MAX_PATH] = "";
static char g_an_outpath[MAX_PATH] = "";
DWORD WINAPI vmp38_analyze_thread(LPVOID lpParam);


////////////////////////////////////////////////////////////////////////////////

extern "C" {
	extern int pause_flag;
	extern struct XX_CONTEXT start_context;
	extern int xdbg_gen_fmem();
	extern int get_inst_context_test(struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context);
	///////////////////////////////////////////////////////////////////////////
	extern void dbg_cmd_ana_static();

	////////////////
	extern HINSTANCE hinst;
	extern int xx_start_saly();
	extern void xx_saly_stop();
	extern struct XX_MOD xx_mod;

	extern char log_file[];
	extern char opmz_cmd[];
	extern char opmz_log[];
	extern char opmz_file[];
	extern char opmz_dfile[];
	extern char exp_file[];

	extern int init_mem_file(char *);
	extern int xx_init32(char* mainname);
	extern char xdbg_path[0x400];
	extern int g_inst_num;




	//extern int xx_cfg();
	//extern int  xx_cfg_init(char *xdbg_path);

	extern int func_op_vmem();

}
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////


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




	iret = func_op_vmem();
	if (iret == 0)
	{
		MessageBox(0, "func_op_vmem", "error", MB_OK);
	}

	/* stable ASCII commands for headless (.cf) automation - the Chinese menu
	   titles get rewritten to unusable "menu________" names in headless */
	_plugin_registercommand(pluginHandle, "vmp38_restore", cb_vmp38_restore, true);
	_plugin_registercommand(pluginHandle, "vmp38_dyn", cb_vmp38_dyn, true);
	_plugin_registercommand(pluginHandle, "vmp38_scan", cb_vmp38_scan, true);
	_plugin_registercommand(pluginHandle, "vmp38_stop", cb_vmp38_stop, true);
	_plugin_registercommand(pluginHandle, "vmp38_analyze", cb_vmp38_analyze, true);
	_plugin_registercommand(pluginHandle, "vmp38_revm", cb_vmp38_revm, true);

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
	HANDLE hsubmenu = 0;
	unsigned int iyear = 0;
	unsigned int imonth = 0;
	unsigned int iday = 0;
	char tmp[200];

	hMenu = setupStruct->hMenu;

	int menuid = 0;
	int iret = 0;

	//GuiAddLogMessage, sz$safeprojectname$Info;
	// place any additional setup code here

		//_plugin_menuadd(hMenu, L"xx_vm1");

	/* Grouped submenus: VMP38 (current focus), OPMZ. The legacy xxvm
	   analyzer (Log-Init / Analyze-*) is kept in code for headless
	   command compatibility but its menu entries are dropped - VMP38
	   does not depend on it. */
	/* VMP38 top-level menu: Restore / Dyn / Stop / Scan / Analyze */
		g_menu_dyn = _plugin_menuaddentry(hMenu, MENU_VMP38_DYN, "\xE5\x8A\xA8\xE6\x80\x81\xE8\xBF\x98\xE5\x8E\x9F");
	_plugin_menuaddentry(hMenu, MENU_VMP38_RESTORE, "\xE9\x9D\x99\xE6\x80\x81\xE8\xBF\x98\xE5\x8E\x9F");
	_plugin_menuaddentry(hMenu, MENU_VMP38_STOP, "\xE5\x8A\xA8/\xE9\x9D\x99\xE5\x81\x9C\xE6\xAD\xA2");
	_plugin_menuaddentry(hMenu, MENU_VMP38_SCAN, "\xE5\x85\xA5\xE5\x8F\xA3\xE6\x89\xAB\xE6\x8F\x8F");
	_plugin_menuaddentry(hMenu, MENU_VMP38_ANALYZE, "\xE7\x8B\xAC\xE7\xAB\x8B\xE5\x88\x86\xE6\x9E\x90");
	g_menu_appends = _plugin_menuaddentry(hMenu, MENU_VMP38_APPENDS,
		g_vm_continue ? "\xE8\x87\xAA\xE5\x8A\xA8\xE8\xBF\xBDVM: \xE5\xBC\x80" : "\xE8\x87\xAA\xE5\x8A\xA8\xE8\xBF\xBDVM: \xE5\x85\xB3");

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
	char tmp[0x400];
	unsigned long long ip_var;
	unsigned long long base_var;
	uchar pcode[200];
	int n = 0;
	Script::Module::ModuleInfo modinfo;
	int iret = 0;
	unsigned long long  var1 = 0;
	XX_CONTEXT curt_context;
	XX_CONTEXT out_context;
	unsigned long long inst_num = 0;
	int log_flag = 0;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;



	switch (info->hEntry)
	{
	case MENU_VMP38_RESTORE:
		if (DbgIsDebugging())
		{
			VMP38_CTX vctx;
			/* FILL DIRECTLY with 64-bit zero-extended registers. Do NOT use
			   XX_CONTEXT (unsigned long r[8], 4-byte fields) - copying it
			   field-by-field into VMP38_CTX (unsigned long long r[16],
			   8-byte fields) misaligns every register (r[5] read garbage
			   like b76c2900000064 = ip leaked in). Script::Register::GetXxx
			   returns duint (32-bit) which zero-extends cleanly into the
			   64-bit slots. */
			memset(&vctx, 0, sizeof(vctx));
			/* CRITICAL: Script::Register::GetXxx returns duint (64-bit even in
			   32-bit x64dbg) and its HIGH 32 BITS ARE GARBAGE in 32-bit hosts
			   (observed ip=0x34400b76c29, eflag=9bb3dc00000000). Cast to
			   unsigned long (32-bit) FIRST, then zero-extend, so the 64-bit
			   slots never carry junk high bits. */
			vctx.r[0] = (unsigned long long)(unsigned long)Script::Register::GetEAX();
			vctx.r[1] = (unsigned long long)(unsigned long)Script::Register::GetECX();
			vctx.r[2] = (unsigned long long)(unsigned long)Script::Register::GetEDX();
			vctx.r[3] = (unsigned long long)(unsigned long)Script::Register::GetEBX();
			vctx.r[4] = (unsigned long long)(unsigned long)Script::Register::GetESP();
			vctx.r[5] = (unsigned long long)(unsigned long)Script::Register::GetEBP();
			vctx.r[6] = (unsigned long long)(unsigned long)Script::Register::GetESI();
			vctx.r[7] = (unsigned long long)(unsigned long)Script::Register::GetEDI();
			vctx.eflag = (unsigned long long)(unsigned long)Script::Register::Get(Script::Register::RegisterEnum::CFLAGS);
			vctx.ip = (unsigned long long)(unsigned long)Script::Register::GetEIP();
			unsigned long vm_entry;
			{
				Script::Module::ModuleInfo mi;
				if (Script::Module::GetMainModuleInfo(&mi))
					vmp38_set_module(mi.path, mi.base);
			}
			/* continue-chain: if EIP is already inside the VM section, restore
			   from EIP (real registers captured at a run+bp hit) instead of
			   re-locating the entry — lets a run+bp pass feed real dispatch
			   registers into the static sim to continue past a jmp reg break.
			   vm_entry is a RUNTIME address in this case (skip VA conversion). */
			{
				Script::Module::ModuleInfo mi3;
				unsigned long long mbase3 = 0;
				if (Script::Module::GetMainModuleInfo(&mi3)) mbase3 = mi3.base;
				unsigned long long cur = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EIP);
				/* locate first so g_vm_start/g_vm_end are set (dynamic VM
				   section bounds - the old hardcoded mbase+0x1f000..0xe8200
				   range was sample-specific and rejected larger VM sections) */
				unsigned long long vmloc = vmp38_auto_locate();
				unsigned long long vmst = 0, vmen = 0;
				vmp38_get_vm_bounds(&vmst, &vmen);
				if (vmst && vmen && cur >= vmst && cur < vmen)
				{
					vm_entry = (unsigned long)cur;
					_plugin_logprintf("menu: continue-chain from EIP %08x (in VM section)", vm_entry);
				}
				else if (cur >= 0x01000000ULL && cur < 0x02000000ULL)
				{
					/* fallback: EIP inside the 0x01xxxxxx VM section but the
					   auto-located bounds missed it (multi-function VM) - use
					   current EIP as the entry so a user bp at ANY VM function
					   (e.g. the key-gen function) feeds real regs + memory into
					   the static sim. Generic: 0x01xxxxxx = VMP 3.8 VM section. */
					vm_entry = (unsigned long)cur;
					_plugin_logprintf("menu: entry from EIP %08x (VM 0x01xxxxxx)", vm_entry);
				}
				else
				{
					vm_entry = (unsigned long)vmloc;
					/* auto_locate returns preferred-base VA; convert below */
				}
			}
			/* auto_locate returns preferred-base VA; convert to runtime addr */
			{
				Script::Module::ModuleInfo mi2;
				unsigned long long mbase = 0;
				unsigned long long mimg = 0x400000ULL;
				if (Script::Module::GetMainModuleInfo(&mi2)) mbase = mi2.base;
				mimg = vmp38_get_imgbase();
				if (!mimg) mimg = 0x400000ULL;
				if (vm_entry && vm_entry < 0x100000000ULL && mbase)
				{
					/* only convert preferred-base VA -> runtime when the entry
					   came from auto_locate; continue-chain already passed a
					   runtime address (VM-section EIP). */
					unsigned long long cur = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EIP);
					unsigned long long vmst2 = 0, vmen2 = 0;
					vmp38_get_vm_bounds(&vmst2, &vmen2);
					if (!(vmst2 && vmen2 && cur >= vmst2 && cur < vmen2))
						vm_entry = (unsigned long)(mbase + (vm_entry - mimg));
				}
				_plugin_logprintf("menu: vm_entry(runtime)=%08x mbase=%08llx mimg=%08llx", vm_entry, mbase, mimg);
			}
			/* NOTE: VMP 3.8 anti-debug triggers "File corrupted" on:
			   - sti single-step (Trap Flag detection)
			   - SetBreakpoint+Run (memory INT3 -> integrity CRC)
			   => GUI and headless BOTH use pure-static restore: read the
			      file + current registers, no single-step, no breakpoint. */
			{
				/* Always restore: if EIP happens to be at the VM entry we get
				   real registers (incl. ESI) and the xx_execute sim resolves
				   the dispatch table via DbgMemRead; otherwise we use the
				   current context and static mini-exec. No sti, no bp. */
				VMP38_CTX fb; memset(&fb, 0, sizeof(fb));
				/* continue-chain (EIP already inside VM section): do NOT call
				   vmp38_capture_vm_regs — its SetBreakpoint+Run deadlocks in
				   headless menu callbacks. The RPC/bp pass already paused us
				   at the real dispatch point with live registers; use vctx.
				   FIX: match on "EIP inside the VM section" (cur >= vmst &&
				   cur < vmen), NOT "EIP == vm_entry" - the user's breakpoint
				   (e.g. 00B76C29) is inside the VM section but NOT the
				   auto_locate entry (00bff000); the old == test silently fell
				   back to capture/entry and restored from the WRONG address. */
				{
					unsigned long long cur_eip = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EIP);
					unsigned long long vmst = 0, vmen = 0;
					int in_vm = 0;
					vmp38_get_vm_bounds(&vmst, &vmen);
					if (cur_eip && vmst && vmen && cur_eip >= vmst && cur_eip < vmen)
						in_vm = 1;
					else if (cur_eip && vmst && vmen && cur_eip < vmst)
					{
						/* EIP below the VM section (e.g. a .text call stub like
						   00B76C29 "call 0xBEE454" that jumps INTO the VM section):
						   treat as VM-entry too - the user breakpoint hit it. */
						DISASM_INSTR di;
						memset(&di, 0, sizeof(di));
						DbgDisasmAt((duint)cur_eip, &di);
						if (di.instr_size >= 1 && di.instruction[0])
						{
							_plugin_logprintf("[VMP38] continue-chain probe @ %08llx: '%s'",
								cur_eip, di.instruction);
							if (strncmp(di.instruction, "call", 4) == 0 ||
								strncmp(di.instruction, "jmp", 3) == 0)
								in_vm = 1;
						}
						else
							_plugin_logprintf("[VMP38] continue-chain probe @ %08llx: no disasm (instr_size=%d)",
								cur_eip, di.instr_size);
					}

					if (in_vm)
					{
						fb.ip = vctx.ip;
						fb.r[4] = vctx.r[4];
						fb.r[0]=vctx.r[0]; fb.r[1]=vctx.r[1]; fb.r[2]=vctx.r[2];
						fb.r[3]=vctx.r[3]; fb.r[5]=vctx.r[5]; fb.r[6]=vctx.r[6]; fb.r[7]=vctx.r[7];
						fb.eflag = vctx.eflag;
						/* 2026-08-22 实证：auto_locate 的 .text per-function call stub 扫描
						   因 VMProtect 加密 .text stub 而失败(读文件字节解不出 call)，
						   改用 per-fn 入口=fallback vmsect_begin 会让 trace 从段头只跑 40 余条即停
						   (参数不在)，远差于从断点 EIP(引擎中间态)的 20 万条完整 trace。
						   故 continue-chain 仍用断点 EIP 进入(完整 trace)，接受其参数捕获偏。
						   正确参数需从正确 entry 或符号，本自动路径无法可靠获得。 */
						vm_entry = (unsigned long)cur_eip;
					}
					else if (vmp38_capture_vm_regs(vm_entry, &fb))
					{
					/* Real registers captured. The memory breakpoint may hit a
					   JUNK execution point deep in the VM section (not a real
					   handler) — continuing from there disassembles garbage.
					   ALWAYS restore from the VM entry (auto_located). Keep the
					   real EBP/ESP/ESI so the xx_execute sim reads the real VM
					   stack (pop/[ebp]) and dispatch resolves. */
					fb.ip = (unsigned long)vm_entry;
					_plugin_logprintf("[VMP38] restore from entry %08x (ebp=%08x esp=%08x esi=%08x)",
						fb.ip, fb.r[5], fb.r[4], fb.r[6]);
				}
				else
				{
					if (vm_entry)
						vctx.ip = vm_entry;
					fb.ip = vctx.ip;
					fb.r[4] = vctx.r[4];
					fb.r[0]=vctx.r[0]; fb.r[1]=vctx.r[1]; fb.r[2]=vctx.r[2];
					fb.r[3]=vctx.r[3]; fb.r[5]=vctx.r[5]; fb.r[6]=vctx.r[6]; fb.r[7]=vctx.r[7];
					fb.eflag = vctx.eflag;
				}
				/* UNICORN FULL-TRACE static restore: the virtual CPU runs the
				   VM code continuously (call/ret/jmp handled by Unicorn itself),
				   records every executed instruction, stops on vm/module-exit.
				   This replaces the old stepwise restore (which manually chased
				   call/ret/jmp and drifted). */
				if (vm_entry && !g_dyn_running)
				{
					g_ft_entry = vm_entry;
					g_ft_esp = (unsigned long long)(unsigned long)Script::Register::GetESP();
					g_dyn_running = 1;
					g_dyn_stop = 0;
					/* headless (x32dbg script mode) does not reliably schedule
					   a CreateThread'd worker from a menu callback - the script
					   stays blocked on sleep while the trace thread never runs.
					   Detect the host and run synchronously there; GUI keeps the
					   background thread so the window stays responsive. */
					char ft_host[MAX_PATH] = "";
					GetModuleFileNameA(NULL, ft_host, MAX_PATH);
					int ft_headless = (strstr(ft_host, "headless") != NULL);
					if (ft_headless)
					{
						unsigned long long ft_last = 0;
						vmp38_unicorn_full_trace(vm_entry, vctx.r[4], &ft_last);
						g_dyn_running = 0;
					}
					else
					{
						CreateThread(NULL, 0, vmp38_fulltrace_thread, NULL, 0, NULL);
					}
				}
				}   /* close the in_vm scope block opened above */
			}
		}
		else
		{
			MessageBox(0, "NO PROCESS", "ERROR", MB_OK);
		}
		break;
	case MENU_VMP38_APPENDS:
		g_vm_continue = !g_vm_continue;
		_plugin_logprintf("[VMP38] auto-continue-VM toggled -> %d", g_vm_continue);
		_plugin_menuentrysetname(pluginHandle, MENU_VMP38_APPENDS,
				g_vm_continue ? "\xE8\x87\xAA\xE5\x8A\xA8\xE8\xBF\xBDVM: \xE5\xBC\x80" : "\xE8\x87\xAA\xE5\x8A\xA8\xE8\xBF\xBDVM: \xE5\x85\xB3");
		break;
	case MENU_VMP38_ANALYZE:
	{
		char exedir[MAX_PATH] = "";
		char procpath[MAX_PATH] = "";
		GetModuleFileNameA(NULL, procpath, sizeof(procpath));
		_snprintf(exedir, sizeof(exedir), "%s", procpath);
		{
			char *sl = strrchr(exedir, '\\');
			if (sl) *sl = 0;
		}
		if (strstr(procpath, "headless"))
		{
			/* headless test path: run inline analyzer on the newest .data.txt */
			_plugin_logputs("[VMP38-Analyze] headless branch entered");
			char pat[MAX_PATH];
			_snprintf(pat, sizeof(pat), "%s\\xxvm\\*.data.txt", exedir);
			WIN32_FIND_DATAA wfd;
			HANDLE hfind = FindFirstFileA(pat, &wfd);
			char newest[MAX_PATH] = "";
			FILETIME newest_ft = { 0, 0 };
			if (hfind != INVALID_HANDLE_VALUE) {
				do {
					if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
					if (!newest[0] || CompareFileTime(&wfd.ftLastWriteTime, &newest_ft) > 0) {
						newest_ft = wfd.ftLastWriteTime;
						_snprintf(newest, sizeof(newest), "%s\\xxvm\\%s", exedir, wfd.cFileName);
					}
				} while (FindNextFileA(hfind, &wfd));
				FindClose(hfind);
			}
			if (newest[0]) {
				char outpath[MAX_PATH];
				_snprintf(outpath, sizeof(outpath), "%s\\headless_analyze_out.txt", exedir);
				_plugin_logprintf("[VMP38-Analyze] headless data=%s -> calling inline", newest);
				vmp38_analyze_inline(newest, outpath);
				_plugin_logprintf("[VMP38-Analyze] headless done: %s -> %s", newest, outpath);
			} else {
				_plugin_logputs("[VMP38-Analyze] no .data.txt found");
			}
			break;
		}
		{
			char initdir[MAX_PATH];
			_snprintf(initdir, sizeof(initdir), "%s\\xxvm", exedir);
			char file[MAX_PATH] = "";
			OPENFILENAMEA ofn;
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = GuiGetWindowHandle();
			ofn.lpstrInitialDir = initdir;
			ofn.lpstrFilter = "VMP38 data (*.data.txt)\0*.data.txt\0All files\0*.*\0";
			ofn.lpstrFile = file;
			ofn.nMaxFile = sizeof(file);
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
			if (GetOpenFileNameA(&ofn))
			{
				/* run the embedded C++ analyzer on a background thread so the
				   GUI stays responsive (10M-line data files take minutes) */
				if (g_an_running)
				{
					_plugin_logputs("[VMP38-Analyze] already analyzing - wait for it");
					break;
				}
				char outpath[MAX_PATH];
				{
					SYSTEMTIME st;
					GetLocalTime(&st);
					_snprintf(outpath, sizeof(outpath),
						"%s\\xxvm\\vmp38_analyze_%04d%02d%02d-%02d%02d%02d-%03d.txt",
						exedir, st.wYear, st.wMonth, st.wDay,
						st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
				}
				_snprintf(g_an_file, sizeof(g_an_file), "%s", file);
				_snprintf(g_an_outpath, sizeof(g_an_outpath), "%s", outpath);
				g_an_running = 1;
				CreateThread(NULL, 0, vmp38_analyze_thread, NULL, 0, NULL);
				_plugin_logprintf("[VMP38-Analyze] started in background -> %s", outpath);
			}
		}
		break;
	}
	case MENU_VMP38_SCAN:
		if (DbgIsDebugging())
		{
			/* enumerate VM-section large blocks (candidate virtualized
			   function bodies) and restore each - on a background thread
			   so the GUI stays responsive (scan can take a while). */
			if (g_scan_running)
			{
				_plugin_logputs("[VMP38] scan already running on background thread - wait");
				break;
			}
			g_scan_running = 1;
			CreateThread(NULL, 0, vmp38_scan_thread, NULL, 0, NULL);
		}
		else
		{
			MessageBox(0, "NO PROCESS", "ERROR", MB_OK);
		}
		break;

	case MENU_VMP38_DYN:
		if (DbgIsDebugging())
		{
			/* Dynamic single-step trace (sti). Works when the sample has
			   anti-debug disabled (low VMComplexity / options off) — the
			   authors' method (阿强/LingMo0412). Locate VM entry first. */
			Script::Module::ModuleInfo mi;
			unsigned long long mbase = 0, mimg = 0;
			unsigned long long vm_entry = 0;
			if (Script::Module::GetMainModuleInfo(&mi))
			{
				mbase = mi.base;
				vmp38_set_module(mi.path, mi.base);
			}
			mimg = vmp38_get_imgbase();
			if (!mimg) mimg = 0x400000ULL;
			/* Prefer the CURRENT EIP as the trace start: when the user set a
			   breakpoint at a VM call stub (e.g. 00B76C29 call BEE454) and it
			   hit, EIP is exactly that stub - tracing from there restores the
			   SPECIFIC VM function the user targeted (not the whole main-level
			   VM). Only fall back to auto_locate when EIP is not in the VM
			   section. auto_locate runs FIRST so g_vm_start/g_vm_end are set
			   (vmp38_get_vm_bounds reads those globals). */
			{
				unsigned long long cur = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EIP);
				unsigned long long vmst = 0, vmen = 0;
				vm_entry = vmp38_auto_locate();
				if (vm_entry && vm_entry < 0x100000000ULL && mbase)
					vm_entry = mbase + (vm_entry - mimg);
				vmp38_get_vm_bounds(&vmst, &vmen);
				if (cur && vmst && vmen && cur >= vmst && cur < vmen)
				{
					vm_entry = cur;
					_plugin_logprintf("[VMP38] dyn trace from EIP %08llx (VM-section breakpoint)", cur);
				}
				else
					_plugin_logprintf("[VMP38] dyn trace entry=%08llx (auto_locate) mbase=%08llx", vm_entry, mbase);
			}
			if (!vm_entry)
				vm_entry = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EIP);
			/* Run the trace on a background thread so the GUI stays
			   responsive (menu progress label + log render live). The
			   trace loop itself only queues sti + sleeps; x64dbg's command
			   thread executes each sti. */
			if (g_dyn_running)
			{
				_plugin_logprintf("[VMP38] trace already running on a background thread - wait for it");
				break;
			}
			g_dyn_vm_entry = vm_entry;
			g_dyn_running = 1;
			g_dyn_stop = 0;
			CreateThread(NULL, 0, vmp38_dyn_thread, NULL, 0, NULL);
		}
		else
		{
			MessageBox(0, "NO PROCESS", "ERROR", MB_OK);
		}
		break;
	case MENU_VMP38_STOP:
		if (g_dyn_running)
		{
			g_dyn_stop = 1;
			_plugin_logputs("[VMP38] stop requested - trace loop exits at next step");
		}
		else
			_plugin_logputs("[VMP38] no trace running");
		break;


	}

}



/*运行状态检查*/


/*
测试单条指令
*/








/* Live progress on the plugin menu entry while vmp38_dynamic_trace runs.
   step==0 resets the label to the idle name. Called from the trace loop
   (command thread); _plugin_menuentrysetname posts to the GUI thread, so
   it is safe to call every few hundred steps. */
void vmp38_dyn_menu(int step, int handlers)
{
	char buf[96];
	if (step > 0)
		sprintf(buf, "\xE5\x8A\xA8\xE6\x80\x81\xE8\xBF\x98\xE5\x8E\x9F \xE8\xBF\x90\xE8\xA1\x8C\xE4\xB8\xAD: %d \xE6\xAD\xA5 / %d \xE5\xA4\x84", step, handlers);
	else
		strcpy(buf, "\xE5\x8A\xA8\xE6\x80\x81\xE8\xBF\x98\xE5\x8E\x9F");
	_plugin_menuentrysetname(pluginHandle, MENU_VMP38_DYN, buf);
}

/* Background trace thread: vmp38_dynamic_trace keeps the command thread
   busy only with queued sti; the GUI thread stays free to render logs and
   refresh the VMP38-Dyn menu progress label. */
DWORD WINAPI vmp38_dyn_thread(LPVOID lpParam)
{
	(void)lpParam;
	vmp38_dynamic_trace(g_dyn_vm_entry);  /* no step limit: trace until VM return / module escape / user stop */
	g_dyn_running = 0;
	_plugin_logprintf("[VMP38] background trace finished");
	g_dyn_stop = 0;
	return 0;
}

/* Background scan thread: same reason as vmp38_dyn_thread - keep the
   GUI responsive while vmp38_scan_all_entries auto-tests all blocks. */
DWORD WINAPI vmp38_scan_thread(LPVOID lpParam)
{
	(void)lpParam;
	vmp38_scan_all_entries();
	g_scan_running = 0;
	_plugin_logputs("[VMP38] scan finished");
	return 0;
}

/* Background analyze thread: same reason - a 10M-line .data.txt would
   otherwise freeze the GUI for minutes. Files are copied to globals by
   the menu callback (single instance, g_an_running guards re-entry). */
DWORD WINAPI vmp38_analyze_thread(LPVOID lpParam)
{
	(void)lpParam;
	vmp38_analyze_inline(g_an_file, g_an_outpath);
	g_an_running = 0;
	_plugin_logprintf("[VMP38-Analyze] background done - see %s", g_an_outpath);
	return 0;
}

/* ============================================================================
   Headless command callbacks: forward to the CBMENUENTRY switch so the .cf
   automation can trigger VMP38 Restore/Dyn/Scan/Stop/Analyze with stable
   ASCII command names (the Chinese menu titles get rewritten by headless).
   ========================================================================== */
static void vmp38_fire_menu(int entry)
{
	PLUG_CB_MENUENTRY info;
	info.hEntry = entry;
	CBMENUENTRY(CB_MENUENTRY, &info);
}
static bool cb_vmp38_restore(int argc, char** argv) { (void)argc; (void)argv; vmp38_fire_menu(MENU_VMP38_RESTORE); return true; }
static bool cb_vmp38_dyn(int argc, char** argv) { (void)argc; (void)argv; vmp38_fire_menu(MENU_VMP38_DYN); return true; }
static bool cb_vmp38_scan(int argc, char** argv) { (void)argc; (void)argv; vmp38_fire_menu(MENU_VMP38_SCAN); return true; }
static bool cb_vmp38_stop(int argc, char** argv) { (void)argc; (void)argv; vmp38_fire_menu(MENU_VMP38_STOP); return true; }
static bool cb_vmp38_analyze(int argc, char** argv) { (void)argc; (void)argv; vmp38_fire_menu(MENU_VMP38_ANALYZE); return true; }
static bool cb_vmp38_revm(int argc, char** argv) { (void)argc; (void)argv; vmp38_revm_exec(); return true; }
