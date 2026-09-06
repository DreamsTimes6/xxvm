#include <stdio.h>
//#include <windows.h>
#include <string.h>
#include <time.h>


//#define XX_VM_LOG_PATH    "C:\\vm_log\\"


int init_file_path(char *ppath, char *pname, char *file)
{
	//char filename[500];
	char filepath[0x400];

	//int *p = (int *)&filename;
	time_t ttime;
	struct tm *pt;
	char tmp[200];

	char *ptmp_name=0;


	if (file == 0)
	{
		return -1;
	}

	if (pname==0 || strlen(pname) == 0)
	{
		ptmp_name="xx";
	}
	else
	{
		ptmp_name=pname;
	}

	memset(filepath, 0, sizeof(filepath));
	memset(tmp, 0, sizeof(tmp));
	memset(&ttime, 0, sizeof(ttime));
	//memcpy(filepath,XX_VM_LOG_PATH,sizeof(XX_VM_LOG_PATH));
	strcat(filepath, ppath);
	strcat(filepath, ptmp_name);
	strcat(filepath, "_");
	time(&ttime);
	pt = localtime(&ttime);
	sprintf(filepath + strlen(filepath), "%04d%02d%02d%02d%02d%02d", (1900 + pt->tm_year), (1 + pt->tm_mon), pt->tm_mday, pt->tm_hour, pt->tm_min,pt->tm_sec);
	strcat(filepath, "\\");

	memcpy(file, filepath, strlen(filepath));

	return 0;
}








