#include <stdio.h>
#include "xx_comm32.h"
#include "xx_opdef3.h"
#include "xxdisasm32.h"

/*
eax=010104010402
ebx=010204010402
ecx=010304010402
edx=010404010402
esi=010504010402
edi=010604010402
ebp=010704010402
esp=010804010402
*/


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




#if 0
int get_reg_index(char *reg_def)
{
	int n=0;
	n=reg_def[1];
	switch(n)
	{
	case 1:
		return 0;
		break;
	case 2:
		return 3;
		break;
	case 3:
		return 1;
		break;
	case 4:
		return 2;
		break;
	case 5:
		return 6;
		break;
	case 6:
		return 7;
		break;
	case 7:
		return 5;
		break;
	case 8:
		return 4;
		break;
	}
	return 0;
}

int get_reg_def_mic(char *reg_mic,char *reg_def)
{
	int n=0;
	if(memcmp(reg_mic,"eax",strlen(reg_mic))==0)
	{
		memcpy(reg_def,xreg_eax,xreg_len);
	}
	else if(memcmp(reg_mic,"ebx",strlen(reg_mic))==0)
	{
		memcpy(reg_def,xreg_ebx,xreg_len);
	}
	else if(memcmp(reg_mic,"ecx",strlen(reg_mic))==0)
	{
		memcpy(reg_def,xreg_ecx,xreg_len);
	}
	else if(memcmp(reg_mic,"edx",strlen(reg_mic))==0)
	{
		memcpy(reg_def,xreg_edx,xreg_len);
	}
	else if(memcmp(reg_mic,"esi",strlen(reg_mic))==0)
	{
		memcpy(reg_def,xreg_esi,xreg_len);
	}
	else if(memcmp(reg_mic,"edi",strlen(reg_mic))==0)
	{
		memcpy(reg_def,xreg_edi,xreg_len);
	}
	else if(memcmp(reg_mic,"ebp",strlen(reg_mic))==0)
	{
		memcpy(reg_def,xreg_ebp,xreg_len);
	}
	else if(memcmp(reg_mic,"esp",strlen(reg_mic))==0)
	{
		memcpy(reg_def,xreg_esp,xreg_len);
	}
	else
	{
		return 0;
	}

	return xreg_len;
}




#else

int get_reg_index(char *reg_def)
{
	int n = 0;
	int m = 0;
	char tmp_def[100];

	memset(tmp_def, 0, sizeof(tmp_def));
	nstr_hex(reg_def, strlen(reg_def), tmp_def);

	n = tmp_def[0];
	
	m = tmp_def[1];

	
	return n*8+m;
}


#endif







