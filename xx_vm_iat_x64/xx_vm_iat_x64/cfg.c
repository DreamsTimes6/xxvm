#include <windows.h>
#include <stdio.h>
#include "resource.h"

#include <winuser.h>
#include "xx_comm64.h"

extern HINSTANCE hinst;
unsigned long long code_start;
unsigned long long code_end;
unsigned long long vmp0_start;
unsigned long long vmp0_end;
///////////////////////////////////////////////////////////////////////////

int cfg_win_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int   cfg_win_create(HWND hWnd);
int func_str_data(char* pstr, char *pdata);
/////////////////////////////////////////////////////////////////////////////



int cfg_win_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int iret = 0;
	int wmId, wmEvent;
	int length = 0;
	int var = 0;
	char tmp[200];
	char tmp2[200];

	switch (message)
	{
	case WM_INITDIALOG:
		if (code_start)
		{
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "%llx", code_start);
			SetDlgItemText(hWnd, IDC_EDITCS, tmp);
		}
		if (code_end)
		{
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "%llx", code_end);
			SetDlgItemText(hWnd, IDC_EDITCE, tmp);
		}
		if (vmp0_start)
		{
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "%llx", vmp0_start);
			SetDlgItemText(hWnd, IDC_EDITVS, tmp);
		}
		if (vmp0_end)
		{
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "%llx", vmp0_end);
			SetDlgItemText(hWnd, IDC_EDITVE, tmp);
		}
		return TRUE;
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDC_BUTTON1:
			length = 0;
			memset(tmp, 0, sizeof(tmp));
			length = GetDlgItemText(hWnd, IDC_EDITCS, tmp, sizeof(tmp));
			if (length >= sizeof(tmp) || length > 16)
			{
				MessageBox(0, "code_start error", "error", MB_OK);
				return 1;
			}
			iret = func_str_data(tmp, (char*)&code_start);
			if (iret == 0)
			{
				MessageBox(0, "code_start error", "error", MB_OK);
				return 1;
			}
			//////////////////////
			length = 0;
			memset(tmp, 0, sizeof(tmp));
			length = GetDlgItemText(hWnd, IDC_EDITCE, tmp, sizeof(tmp));
			if (length >= sizeof(tmp) || length > 16)
			{
				MessageBox(0, "code_end error", "error", MB_OK);
				return 1;
			}
			iret = func_str_data(tmp, (char*)&code_end);
			if (iret == 0)
			{
				MessageBox(0, "code_end error", "error", MB_OK);
				return 1;
			}
			/////////////////
			length = 0;
			memset(tmp, 0, sizeof(tmp));
			length = GetDlgItemText(hWnd, IDC_EDITVS, tmp, sizeof(tmp));
			if (length >= sizeof(tmp) || length > 16)
			{
				MessageBox(0, "vmp0_start error", "error", MB_OK);
				return 1;
			}
			iret = func_str_data(tmp, (char*)&vmp0_start);
			if (iret == 0)
			{
				MessageBox(0, "vmp0_start error", "error", MB_OK);
				return 1;
			}
			///////////////
			length = 0;
			memset(tmp, 0, sizeof(tmp));
			length = GetDlgItemText(hWnd, IDC_EDITVE, tmp, sizeof(tmp));
			if (length >= sizeof(tmp) || length > 16)
			{
				MessageBox(0, "vmp0_end error", "error", MB_OK);
				return 1;
			}

			iret = func_str_data(tmp, (char*)&vmp0_end);
			if (iret == 0)
			{
				MessageBox(0, "vmp0_end error", "error", MB_OK);
				return 1;
			}

			EndDialog(hWnd, 1);
			return 1;
			break;
		case IDC_BUTTON2:
			EndDialog(hWnd, 1);
			return 1;
		}
		break;
	case WM_CLOSE:
		EndDialog(hWnd, 1);
		break;
	}
	return FALSE;
}


__declspec(noinline) int   cfg_win_create(HWND hWnd)
{
	int iret = 0;

	return DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_DIALOG4), hWnd, (DLGPROC)cfg_win_proc, 0);

	//return 1;
}





__declspec(noinline) int func_str_data(char* pstr, char *pdata)
{
	char tmp[100];
	int iret = 0;
	int str_len = 0;
	char str_tmp[100];

	/*²ÎÊý¼ì²é*/
	if (pstr == 0 || pdata == 0)
	{
		return 0;
	}

	str_len = strlen(pstr);
	if (str_len == 0 || str_len > 16)
	{
		return 0;
	}

	memset(str_tmp, 0x30, sizeof(str_tmp));

	if (str_len % 2)
	{
		memcpy(str_tmp + 1, pstr, str_len);
		str_len = str_len + 1;
	}
	else
	{
		memcpy(str_tmp, pstr, str_len);
	}

	memset(tmp, 0, sizeof(tmp));
	iret = nstr_hex(str_tmp, str_len, tmp);
	if (iret)
	{
		return 0;
	}

	iret = byte_opposite(tmp, str_len / 2, pdata);
	if (iret)
	{
		return 0;
	}


	return 1;
}






