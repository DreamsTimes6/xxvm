#include <stdio.h>
#include <Windows.h>
#include "xx_vm.h"
#include <_plugins.h>
#include <bridgemain.h>



#define XX_ANA_COUNT   1    //分析次数
#define XX_ANA_RULE    2	//分析规则，正常和按次数分析，循环

////////////////////////////////////////////////////////////////////////////////////





char xdbg_path[0x400];
extern struct XX_CONTEXT start_context;
//////////////////////////////////////////////////////////////////////////////

extern int xx_start_saly();
extern void xx_saly_stop();
extern void xx_saly_semaphore(HANDLE sem);
extern int is_vm_entry();

/*设置配置*/
extern int xx_cfg_set_int(int ikey, int in_var);

/*读取配置*/
extern int xx_cfg_get_int(int ikey, int *out_var);

/*设置优化数据开关*/
extern int xx_opmz_set_flag();

extern int init_mem_file(char *);
extern int free_mem_file();
///////////////////////////////////////////////////////////////////////////////////
/*
命令函数单独线程处理，根据配置选择是否需要等待
分析函数也使用单独线程处理

对暂停功能有影响，分析模块提供暂停接口，外表调用，分析内部退出

分析模块提供信号句柄传递接口，


循环分析，，判断入口，时入口则分析，分析模块提供入口模块


内存数据处理，内存数据考虑由分析模块外部释放

*/
///////////////////////////////////////////////////////////////////////////////////
/*
动态分析命令
*/





/*
静态分析命令
*/
void dbg_cmd_ana_static()
{
	int iret = 0;
	int rule = 0;
	int count = 0;
	HANDLE hSemaphore = 0;
	int cmax = 1;
	int n = 0;
	char tmp[200];

	/*获取配置信息*/
	rule = 0;
	iret = xx_cfg_get_int(XX_ANA_RULE, &rule);
	if (iret == 0)
	{
		MessageBox(0, "error:xx_cfg.ini ", "error", MB_OK);
		return;
	}
	count = 0;
	iret = xx_cfg_get_int(XX_ANA_COUNT, &count);
	if (iret == 0)
	{
		MessageBox(0, "error:xx_cfg.ini ", "error", MB_OK);
		return;
	}

	/*设置优化数据开关*/
	iret = xx_opmz_set_flag();
	if (iret == 0)
	{
		MessageBox(0, "error:xx_cfg.ini ", "error", MB_OK);
		return;
	}

	/*初始化*/
	iret = init_mem_file(xdbg_path);
	if (iret == 0)
	{
		MessageBox(0, "init error", "error", MB_OK);
		return;
	}
	_plugin_logputs("init success");



	switch (rule)
	{
	case 0:
		/*Normal*/
		hSemaphore = CreateSemaphore(NULL, 0, cmax, NULL);
		if (hSemaphore == NULL)
		{
			MessageBox(0, "error:cmd_ana_static ", "error", MB_OK);
			goto clean_ret;
		}

		xx_saly_semaphore(hSemaphore);

		_plugin_logputs("Normal----start ...please wait\r\n");
		if (start_context.ip == 0)
		{
			MessageBox(0, "error:ip ", "error", MB_OK);
			goto clean_ret;
		}
		else
		{
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)xx_start_saly, 0, 0, 0);
			iret = WaitForSingleObject(hSemaphore, 0x10000000);
			switch (iret)
			{
				// The semaphore object was signaled.
			case WAIT_OBJECT_0:
				MessageBox(0, "Finish", "success(Normal)\r\n", MB_OK);
				// OK to open another window.
				//memset(tmp, 0, sizeof(tmp));
				//sprintf(tmp, "next ip:[%llx]", start_context.ip);
				//_plugin_logputs(tmp);
				break;
			default:
				/*其它原因则暂停分析线程*/
				xx_saly_stop();
				MessageBox(0, "error:sem ", "error(Normal)\r\n", MB_OK);
				goto clean_ret;

			}
		}
		break;
	case 1:
		/*Cycle limited*/

		for (n = 0; n < count; n++)
		{
			hSemaphore = CreateSemaphore(NULL, 0, cmax, NULL);
			if (hSemaphore == NULL)
			{
				MessageBox(0, "error:cmd_ana_static ", "error", MB_OK);
				goto clean_ret;
			}

			xx_saly_semaphore(hSemaphore);

			if (start_context.ip == 0)
			{
				MessageBox(0, "error:ip ", "error", MB_OK);
				_plugin_logprintf("error:ip");
				goto clean_ret;
			}
			else
			{
				_plugin_logprintf("Cycle limited[%x][%x]----start ...please wait\r\n", count, n + 1);
				if (n != 0)
				{
					/*检查入口*/
					iret = is_vm_entry();
					if (iret == 0)
					{
						/*不是入口*/
						_plugin_logprintf("next ip[%llx] is not 'vm_entry',Cycle break,[%x]-[%x]\r\n", \
							start_context.ip, count, n + 1);
						break;
					}
				}

				CreateThread(0, 0, (LPTHREAD_START_ROUTINE)xx_start_saly, 0, 0, 0);
				iret = WaitForSingleObject(hSemaphore, 0x10000000);
				switch (iret)
				{
					// The semaphore object was signaled.
				case WAIT_OBJECT_0:
					_plugin_logprintf("Cycle limited[%x][%x]----finish\r\n", count, n + 1);

					// OK to open another window.
					break;
				default:
					/*其它原因则暂停分析线程*/
					xx_saly_stop();
					MessageBox(0, "error:sem ", "error(Cycle limited)\r\n", MB_OK);
					goto clean_ret;

				}

			}
		}
		break;
	case 2:
		/*Cycle*/
		_plugin_logputs("Cycle----start ...please wait\r\n");
		n = 0;
		while (1)
		{
			hSemaphore = CreateSemaphore(NULL, 0, cmax, NULL);
			if (hSemaphore == NULL)
			{
				MessageBox(0, "error:cmd_ana_static ", "error", MB_OK);
				goto clean_ret;
			}

			xx_saly_semaphore(hSemaphore);

			if (start_context.ip == 0)
			{
				MessageBox(0, "error:ip ", "error", MB_OK);
				_plugin_logprintf("error:ip");
				goto clean_ret;
			}
			else
			{
				_plugin_logprintf("Cycle [%x]----start ...please wait\r\n", n + 1);
				if (n != 0)
				{
					/*检查入口*/
					iret = is_vm_entry();
					if (iret == 0)
					{
						/*不是入口*/
						_plugin_logprintf("next ip[%llx] is not 'vm_entry',Cycle break,[%x]\r\n", \
							start_context.ip, n + 1);
						break;
					}
				}

				CreateThread(0, 0, (LPTHREAD_START_ROUTINE)xx_start_saly, 0, 0, 0);
				iret = WaitForSingleObject(hSemaphore, 0x10000000);
				switch (iret)
				{
					// The semaphore object was signaled.
				case WAIT_OBJECT_0:
					_plugin_logprintf("Cycle [%x]----finish\r\n", n + 1);

					// OK to open another window.
					break;
				default:
					/*其它原因则暂停分析线程*/
					xx_saly_stop();
					MessageBox(0, "error:sem ", "error(Cycle )\r\n", MB_OK);
					goto clean_ret;

				}

			}
			n++;
		}
		break;
	default:

		MessageBox(0, "error:xx_cfg  rule ", "error", MB_OK);
		goto clean_ret;
	}


clean_ret:
	/*结束清理*/
	ReleaseSemaphore(hSemaphore, 1, 0);
	hSemaphore = 0;

	iret = free_mem_file();
	if (iret == 0)
	{
		MessageBox(0, "data clean error", "error", MB_OK);
		//return 0;
	}

	MessageBox(0, "Analyze Finish", "info", MB_OK);
	return;
}










