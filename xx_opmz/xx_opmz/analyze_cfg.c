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
//int InitInstance_cfg();
int xx_cfg_get_int(int ikey, int *out_var);
int xx_cfg_set_int(int ikey, int in_var);
///////////////////////////////////////////////////////////////////////////////////////////////

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





