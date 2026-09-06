#include <windows.h>




HINSTANCE hinst;



int __stdcall DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	void *dllbase=0;
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			hinst=(HINSTANCE)hModule;
			//dllhinst=GetModuleHandle(DLL_NAME);
			dllbase=LoadLibrary("wininet.dll");

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
};




/*
2019.3.18
测试基本完成

优化选项，运行状态，完成


2019.3.19
32指令信息修改

指令信息增加32位，64位标志，通用一个xx_exp.x，完成


读内存使用调试器接口，出错显示信息




*/




















