#include "stdio.h"
#include <windows.h>
#include "xx_vm.h"
#include "xx_comm32.h"
#include "xxdisasm32.h"
#include "xx_opdef3.h"



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



/*push,pop记录在临时记录中*/
/*连续11个以上push,pop记录，且遇到jmp,就更新记录*/

/*entry，重新初始化记录*/

/*ret暂时不记录*/

void vm_reg_status(int func)
{
	switch(func)
	{
	case 0x0100:
		/*push*/
		break;
	case 0x0300:
		/*pop*/
		break;
	case 0000:
		/*entry*/
		break;
	case 0x0f01:
		/*jmp*/
		break;
	case 0x1200:
		/*ret*/
		break;
	default:
		break;
	}

}