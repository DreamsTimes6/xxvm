#include <windows.h>
#include <stdio.h>
#include "resource.h"

#include <winuser.h>
#include "xx_comm32.h"

extern HINSTANCE hinst;
extern	unsigned int code_start;
extern	unsigned int code_end;
extern	unsigned int vmp0_start;
extern	unsigned int vmp0_end;
///////////////////////////////////////////////////////////////////////////

int cfg_win_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int   cfg_win_create(HWND hWnd);

/////////////////////////////////////////////////////////////////////////////



int cfg_win_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int iret=0;
	int wmId,wmEvent;
	int length=0;
	int var=0;
	char tmp[200];
	char tmp2[200];
	
	switch (message) 
    { 
	case WM_INITDIALOG:
		if(code_start)
		{
			memset(tmp,0,sizeof(tmp));
			sprintf(tmp,"%08x",code_start);
			SetDlgItemText(hWnd,IDC_EDITCS,tmp);
		}
		if(code_end)
		{
			memset(tmp,0,sizeof(tmp));
			sprintf(tmp,"%08x",code_end);
			SetDlgItemText(hWnd,IDC_EDITCE,tmp);
		}
		if(vmp0_start)
		{
			memset(tmp,0,sizeof(tmp));
			sprintf(tmp,"%08x",vmp0_start);
			SetDlgItemText(hWnd,IDC_EDITVS,tmp);
		}
		if(vmp0_end)
		{
			memset(tmp,0,sizeof(tmp));
			sprintf(tmp,"%08x",vmp0_end);
			SetDlgItemText(hWnd,IDC_EDITVE,tmp);
		}
		return TRUE;
		break;
	case WM_COMMAND: 
		wmId=LOWORD(wParam);
		wmEvent=HIWORD(wParam);
		switch(wmId)
		{
		case IDC_BUTTON1:
			length=0;
			memset(tmp,0,sizeof(tmp));
			length=GetDlgItemText(hWnd,IDC_EDITCS,tmp,sizeof(tmp));
			if(length>=sizeof(tmp) || length>8)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}
			memset(tmp2,0,sizeof(tmp2));
			iret=nstr_hex(tmp,length,tmp2);
			if(iret)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}
			iret=byte_opposite(tmp2,length/2,(char*)&code_start);
			if(iret)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}
			//////////////////////
			length=0;
			memset(tmp,0,sizeof(tmp));
			length=GetDlgItemText(hWnd,IDC_EDITCE,tmp,sizeof(tmp));
			if(length>=sizeof(tmp) || length>8)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}
			memset(tmp2,0,sizeof(tmp2));
			iret=nstr_hex(tmp,length,tmp2);
			if(iret)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}
			iret=byte_opposite(tmp2,length/2,(char*)&code_end);
			if(iret)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}
			/////////////////
			length=0;
			memset(tmp,0,sizeof(tmp));
			length=GetDlgItemText(hWnd,IDC_EDITVS,tmp,sizeof(tmp));
			if(length>=sizeof(tmp) || length>8)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}
			memset(tmp2,0,sizeof(tmp2));
			iret=nstr_hex(tmp,length,tmp2);
			if(iret)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}
			iret=byte_opposite(tmp2,length/2,(char*)&vmp0_start);
			if(iret)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}
			///////////////
			length=0;
			memset(tmp,0,sizeof(tmp));
			length=GetDlgItemText(hWnd,IDC_EDITVE,tmp,sizeof(tmp));
			if(length>=sizeof(tmp) || length>8)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}
			memset(tmp2,0,sizeof(tmp2));
			iret=nstr_hex(tmp,length,tmp2);
			if(iret)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}
			iret=byte_opposite(tmp2,length/2,(char*)&vmp0_end);
			if(iret)
			{
				MessageBox(0,"data error","error",MB_OK);
				return 1;
			}

			EndDialog(hWnd,1);
			return 1;
			break;
		case IDC_BUTTON2:
			EndDialog(hWnd,1);
			return 1;
		}
		break;
	case WM_CLOSE:
		EndDialog(hWnd,1);
		break;
    } 
    return FALSE; 
}


int   cfg_win_create(HWND hWnd)
{
	HINSTANCE hInstance;
	int iret=0;
	
	hInstance=hinst;
	//register
	return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG4), hWnd, (DLGPROC)cfg_win_proc,0) ;

	//return 1;
}
