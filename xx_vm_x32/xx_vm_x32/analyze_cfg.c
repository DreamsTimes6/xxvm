#include <windows.h>
//#include "res.h"
#include <stdio.h>
#include "resource.h"
#include "xx_comm32.h"



#define XX_ANA_COUNT   1    //分析次数
#define XX_ANA_RULE    2	//分析规则，正常和按次数分析，循环
#define XX_OPMZ_DATA   3    //优化数据
#define XX_LOG_FLAG    4    //是否关联程序打开
#define XX_LOGIN_AUTO    5    //是否自动登陆



/*配置文件路径不变*/
char *xx_cfg_file = 0;
char xx_cfg_name[] = "xx_cfg.ini";

///////////////////////////////////////////////////////////////////////////////////////////////
extern  HINSTANCE hinst;

///////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK proc_cfg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
int InitInstance_cfg();
int xx_cfg_get_int(int ikey, int *out_var);
int xx_cfg_set_int(int ikey, int in_var);
///////////////////////////////////////////////////////////////////////////////////////////////
/*打开配置*/
int  xx_cfg()
{
	int iret = 0;

	/*初始化配置目录*/
	if (xx_get_file_size(xx_cfg_file) == 0)
	{
		return 0;
	}

	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InitInstance_cfg, 0, 0, 0);

	return 1;
}


int  xx_cfg_init(char *xdbg_path)
{
	int iret = 0;
	int var = 0;
	int path_len = 0;
	int xx_cfg_size = 0;


	if (xdbg_path == 0)
	{
		return 0;
	}

	path_len = strlen(xdbg_path);
	if (path_len == 0)
	{
		return 0;
	}

	if (xx_cfg_file == 0)
	{
		xx_cfg_size = path_len + sizeof(xx_cfg_name) + 10;
		xx_cfg_file = malloc(xx_cfg_size);
		if (xx_cfg_file == 0)
		{
			return 0;
		}

		memset(xx_cfg_file, 0, xx_cfg_size);
		sprintf(xx_cfg_file, "%s%s", xdbg_path, xx_cfg_name);
	}

	/*检查配置文件*/
	if (xx_get_file_size(xx_cfg_file) == 0)
	{


		/*没有则生成*/
		iret = xx_cover_file(xx_cfg_file, "[XX_ANA]\r\n", strlen("[XX_ANA]\r\n"));
		if (iret == 0)
		{
			goto err_ret;
		}

		iret = xx_append_file(xx_cfg_file, "XX_ANA_COUNT = 0\r\n", strlen("XX_ANA_COUNT = 0\r\n"));
		if (iret == 0)
		{
			goto err_ret;
		}
		iret = xx_append_file(xx_cfg_file, "XX_ANA_RULE = 0\r\n", strlen("XX_ANA_RULE = 0\r\n"));
		if (iret == 0)
		{
			goto err_ret;
		}
		iret = xx_append_file(xx_cfg_file, "XX_OPMZ_DATA = 1\r\n", strlen("XX_OPMZ_DATA = 1\r\n"));
		if (iret == 0)
		{
			goto err_ret;
		}
		iret = xx_append_file(xx_cfg_file, "XX_LOG_FLAG = 1\r\n", strlen("XX_LOG_FLAG = 1\r\n"));
		if (iret == 0)
		{
			goto err_ret;
		}
		iret = xx_append_file(xx_cfg_file, "XX_LOGIN_AUTO = 0\r\n", strlen("XX_LOGIN_AUTO = 0\r\n"));
		if (iret == 0)
		{
			goto err_ret;
		}
	}

	/*检查是否有值*/
	var = 0;
	iret = xx_cfg_get_int(XX_ANA_COUNT, &var);
	if (iret == 0)
	{
		/*没有值，则设置默认值*/
		iret = xx_cfg_set_int(XX_ANA_COUNT, 0);
		if (iret == 0)
		{
			/*设置出错则返回错误*/
			goto err_ret;
		}
	}

	var = 0;
	iret = xx_cfg_get_int(XX_ANA_RULE, &var);
	if (iret == 0)
	{
		/*没有值，则设置默认值*/
		iret = xx_cfg_set_int(XX_ANA_RULE, 0);
		if (iret == 0)
		{
			/*设置出错则返回错误*/
			goto err_ret;
		}
	}

	var = 0;
	iret = xx_cfg_get_int(XX_OPMZ_DATA, &var);
	if (iret == 0)
	{
		/*没有值，则设置默认值*/
		iret = xx_cfg_set_int(XX_OPMZ_DATA, 1);
		if (iret == 0)
		{
			/*设置出错则返回错误*/
			goto err_ret;
		}
	}

	var = 0;
	iret = xx_cfg_get_int(XX_LOG_FLAG, &var);
	if (iret == 0)
	{
		/*没有值，则设置默认值*/
		iret = xx_cfg_set_int(XX_LOG_FLAG, 0);
		if (iret == 0)
		{
			/*设置出错则返回错误*/
			goto err_ret;
		}
	}

	var = 0;
	iret = xx_cfg_get_int(XX_LOGIN_AUTO, &var);
	if (iret == 0)
	{
		/*没有值，则设置默认值*/
		iret = xx_cfg_set_int(XX_LOGIN_AUTO, 0);
		if (iret == 0)
		{
			/*设置出错则返回错误*/
			goto err_ret;
		}
	}

	/*再次检查默认值*/
	iret = xx_cfg_get_int(XX_ANA_COUNT, &var);
	if (iret == 0)
	{
		goto err_ret;
	}
	iret = xx_cfg_get_int(XX_ANA_RULE, &var);
	if (iret == 0)
	{
		goto err_ret;
	}

	iret = xx_cfg_get_int(XX_OPMZ_DATA, &var);
	if (iret == 0)
	{
		goto err_ret;
	}

	iret = xx_cfg_get_int(XX_LOG_FLAG, &var);
	if (iret == 0)
	{
		goto err_ret;
	}

	iret = xx_cfg_get_int(XX_LOGIN_AUTO, &var);
	if (iret == 0)
	{
		goto err_ret;
	}

	return 1;

err_ret:
	DeleteFile(xx_cfg_file);
	memset(xx_cfg_file, 0, sizeof(xx_cfg_file));
	return 0;
}


int InitInstance_cfg()
{
	HWND hWnd;
	int iret = 0;
	MSG msg;

	hWnd = CreateDialogParam(hinst, MAKEINTRESOURCE(IDD_DIALOG4), 0, (DLGPROC)proc_cfg, 0);
	if (!hWnd)
	{
		return 0;
	}

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 1;
}

LRESULT CALLBACK proc_cfg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int iret = 0;
	int wmId, wmEvent;
	HWND hcombo1 = 0;
	HWND hcombo2 = 0;
	int sel_index = 0;
	char tmp[100];
	HWND hedit = 0;
	HWND hcheck = 0;
	int rule = 0;
	int count = 0;
	int opmz_flag = 0;
	int log_flag = 0;
	


	switch (message)
	{
	case WM_INITDIALOG:

		/*设置分析规则选择框*/
		rule = 0;
		iret = xx_cfg_get_int(XX_ANA_RULE, &rule);
		if (iret == 0)
		{
			MessageBox(0, "error:xx_cfg.ini ", "error", MB_OK);
			break;
		}
		count = 0;
		iret = xx_cfg_get_int(XX_ANA_COUNT, &count);
		if (iret == 0)
		{
			MessageBox(0, "error:xx_cfg.ini ", "error", MB_OK);
			break;
		}

		hcombo1 = GetDlgItem(hDlg, IDC_COMBO1);
		if (hcombo1 == 0)
		{
			break;
		}

		SendMessage(hcombo1, CB_INSERTSTRING, 0, (LPARAM)"Normal");

		SendMessage(hcombo1, CB_INSERTSTRING, 1, (LPARAM)"Cycle limited");

		SendMessage(hcombo1, CB_INSERTSTRING, 2, (LPARAM)"Cycle");

		SendMessage(hcombo1, CB_SETCURSEL, rule, 0);

		if (rule == 1)
		{
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "%d", count);
			SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, 0, tmp);
			if (strlen(tmp) == 0)
			{
				MessageBox(0, "error:cfg init ", "error", MB_OK);
				break;
			}
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), 1);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), 0);
		}


		/*设置优化数据开关选择框*/
		opmz_flag = 0;
		iret = xx_cfg_get_int(XX_OPMZ_DATA, &opmz_flag);
		if (iret == 0)
		{
			MessageBox(0, "error:xx_cfg.ini ", "error", MB_OK);
			break;
		}

		hcombo2 = GetDlgItem(hDlg, IDC_COMBO2);
		if (hcombo2 == 0)
		{
			break;
		}

		SendMessage(hcombo2, CB_INSERTSTRING, 0, (LPARAM)"Open");

		SendMessage(hcombo2, CB_INSERTSTRING, 1, (LPARAM)"Close");

		SendMessage(hcombo2, CB_SETCURSEL, opmz_flag - 1, 0);

		/*设置日志打开方式*/
		log_flag = 0;
		iret = xx_cfg_get_int(XX_LOG_FLAG, &log_flag);
		if (iret == 0)
		{
			MessageBox(0, "error:xx_cfg.ini ", "error", MB_OK);
			break;
		}
		hcheck = 0;
		hcheck = GetDlgItem(hDlg, IDC_CHECK1);
		if (hcheck == 0)
		{
			break;
		}
		SendMessage(hcheck, BM_SETCHECK, log_flag, 0);

		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDC_BUTTON1:
			/*save*/

			/*分析规则*/
			sel_index = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
			if (sel_index == CB_ERR)
			{
				break;
			}
			switch (sel_index)
			{
			case 0:
				/*Normal*/
				iret = xx_cfg_set_int(XX_ANA_COUNT, 0);
				if (iret == 0)
				{
					MessageBox(0, "error:save ", "error", MB_OK);
					break;
				}
				iret = xx_cfg_set_int(XX_ANA_RULE, 0);
				if (iret == 0)
				{
					MessageBox(0, "error:save ", "error", MB_OK);
					break;
				}
				break;
			case 1:
				/*Cycle limited*/
				memset(tmp, 0, sizeof(tmp));
				SendDlgItemMessage(hDlg, IDC_EDIT1, WM_GETTEXT, sizeof(tmp), tmp);
				if (strlen(tmp) == 0)
				{
					MessageBox(0, "error:save ", "error", MB_OK);
					break;
				}
				count = 0;
				count = atoi(tmp);
				if (count >= 0x10000000)
				{
					MessageBox(0, "error:count is too larger ", "error", MB_OK);
					break;
				}
				iret = xx_cfg_set_int(XX_ANA_COUNT, count);
				if (iret == 0)
				{
					MessageBox(0, "error:save ", "error", MB_OK);
					break;
				}
				iret = xx_cfg_set_int(XX_ANA_RULE, 1);
				if (iret == 0)
				{
					MessageBox(0, "error:save ", "error", MB_OK);
					break;
				}
				break;
			case 2:
				/*Cycle*/
				iret = xx_cfg_set_int(XX_ANA_COUNT, 0);
				if (iret == 0)
				{
					MessageBox(0, "error:save ", "error", MB_OK);
					break;
				}
				iret = xx_cfg_set_int(XX_ANA_RULE, 2);
				if (iret == 0)
				{
					MessageBox(0, "error:save ", "error", MB_OK);
					break;
				}
				break;
			}


			/*优化数据开关*/
			sel_index = SendDlgItemMessage(hDlg, IDC_COMBO2, CB_GETCURSEL, 0, 0);
			if (sel_index == CB_ERR)
			{
				break;
			}
			switch (sel_index)
			{
			case 0:
				iret = xx_cfg_set_int(XX_OPMZ_DATA, 1);
				if (iret == 0)
				{
					MessageBox(0, "error:save ", "error", MB_OK);
					break;
				}
				break;
			case 1:
				iret = xx_cfg_set_int(XX_OPMZ_DATA, 2);
				if (iret == 0)
				{
					MessageBox(0, "error:save ", "error", MB_OK);
					break;
				}
				break;
			}

			/*日志打开方式*/
			hcheck = 0;
			hcheck = GetDlgItem(hDlg, IDC_CHECK1);
			if (hcheck == 0)
			{
				break;
			}
			log_flag = 0;
			log_flag = SendMessage(hcheck, BM_GETCHECK, 0, 0);
			iret = xx_cfg_set_int(XX_LOG_FLAG, log_flag);
			if (iret == 0)
			{
				MessageBox(0, "error:save ", "error", MB_OK);
				break;
			}

			PostQuitMessage(0);
			break;
		case IDC_BUTTON2:
			/*cancle*/
			PostQuitMessage(0);
			break;
		case IDC_COMBO1:
			if (wmEvent == CBN_SELENDOK)
			{

				sel_index = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
				if (sel_index == CB_ERR)
				{
					break;
				}
				switch (sel_index)
				{
				case 0:
					/*Normal*/
					//MessageBox(0, "Normal", "info", MB_OK);
					EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), 0);
					break;
				case 1:
					/*Cycle limited*/
					//MessageBox(0, "Cycle limited", "info", MB_OK);
					EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), 1);
					break;
				case 2:
					/*Cycle*/
					//MessageBox(0, "Cycle", "info", MB_OK);
					EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), 0);
					break;
				}
			}
			break;
		default:
			return 0;
		}
		break;
	default:
		return 0;
	}
	return 1;
}


////////////////////////////////////////////////////////////////
/*设置配置*/
int xx_cfg_set_int(int ikey, int in_var)
{
	char app_name[] = "";
	int iret = 0;
	char  default_str[] = "xxxxx";
	char tmp[0x100];


	if (xx_cfg_file == 0)
	{
		return 0;
	}


	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%d", in_var);
	switch (ikey)
	{
	case XX_ANA_COUNT:
		iret = WritePrivateProfileString("XX_ANA", "XX_ANA_COUNT", tmp, xx_cfg_file);
		break;
	case XX_ANA_RULE:
		iret = WritePrivateProfileString("XX_ANA", "XX_ANA_RULE", tmp, xx_cfg_file);
		break;
	case XX_OPMZ_DATA:
		iret = WritePrivateProfileString("XX_ANA", "XX_OPMZ_DATA", tmp, xx_cfg_file);
		break;
	case XX_LOG_FLAG:
		iret = WritePrivateProfileString("XX_ANA", "XX_LOG_FLAG", tmp, xx_cfg_file);
		break;
	case XX_LOGIN_AUTO:
		iret = WritePrivateProfileString("XX_ANA", "XX_LOGIN_AUTO", tmp, xx_cfg_file);
		break;
	default:
		return 0;
	}

	if (iret == 0)
	{
		return 0;
	}


	return 1;
}


////////////////////////////////////////////////////////////////////
/*
读取配置
只有一个section
*/
int xx_cfg_get_int(int ikey, int *out_var)
{
	char app_name[] = "";
	int iret = 0;
	char  default_str[] = "xxxxx";
	char tmp[0x100];


	if (out_var == 0 || xx_cfg_file == 0)
	{
		return 0;
	}

	memset(tmp, 0, sizeof(tmp));
	switch (ikey)
	{
	case XX_ANA_COUNT:
		iret = GetPrivateProfileString("XX_ANA", "XX_ANA_COUNT", default_str, tmp, sizeof(tmp), xx_cfg_file);
		break;
	case XX_ANA_RULE:
		iret = GetPrivateProfileString("XX_ANA", "XX_ANA_RULE", default_str, tmp, sizeof(tmp), xx_cfg_file);
		break;
	case XX_OPMZ_DATA:
		iret = GetPrivateProfileString("XX_ANA", "XX_OPMZ_DATA", default_str, tmp, sizeof(tmp), xx_cfg_file);
		break;
	case XX_LOG_FLAG:
		iret = GetPrivateProfileString("XX_ANA", "XX_LOG_FLAG", default_str, tmp, sizeof(tmp), xx_cfg_file);
		break;
	case XX_LOGIN_AUTO:
		iret = GetPrivateProfileString("XX_ANA", "XX_LOGIN_AUTO", default_str, tmp, sizeof(tmp), xx_cfg_file);
		break;
	default:
		return 0;
	}

	if (iret == (sizeof(tmp) - 1) || iret == (sizeof(tmp) - 2) || strlen(tmp) == 0 || strcmp(tmp, default_str) == 0)
	{
		return 0;
	}

	*out_var = atoi(tmp);

	return 1;
}




/*
设置配置字符串
*/
int xx_cfg_set_str(char* pkey, char* pvar)
{
	char app_name[] = "";
	int iret = 0;
	char  default_str[] = "xxxxx";


	if (xx_cfg_file == 0)
	{
		return 0;
	}

	iret = WritePrivateProfileString("XX_ANA", pkey, pvar, xx_cfg_file);
	if (iret == 0)
	{
		return 0;
	}


	return 1;
}



/*
读取配置字符串
*/
int xx_cfg_get_str(char* pkey, char* out_var,int out_size)
{
	char app_name[] = "";
	int iret = 0;
	char  default_str[] = "xxxxx";
	char tmp[0x100];


	if (out_var == 0 || xx_cfg_file == 0 || out_size == 0)
	{
		return 0;
	}

	iret = GetPrivateProfileString("XX_ANA", pkey, default_str, out_var, out_size, xx_cfg_file);
	if (iret == (sizeof(out_var) - 1) || iret == (sizeof(out_var) - 2) || strlen(out_var) == 0 || \
		strcmp(tmp, default_str) == 0 || strlen(out_var)>= out_size)
	{
		return 0;
	}

	return 1;
}





