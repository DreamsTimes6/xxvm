

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xx_def.h"

////////////////////////////////////

//////////////BP51/////////////////////////
#if 0
int xx_append_file(char *file, char* data, int dsize);
int xx_cover_file(char *file, char* data, int dsize);
int xx_get_file_size(char* filename);
int xx_read_file(char *filename, char* data, unsigned int fileoffset, unsigned int size);
int xx_append_ff(char *file1, char *file2);
int xx_scover_file(char *file, unsigned char *pdata, unsigned int begin, unsigned int size);
#endif

////////////////////////////////////

/*write file*/
DLL_PUBLIC int xx_append_file(char *file, char* data, int dsize)
{
	FILE *file_stream = 0;
	int write_size = 0;

	if (file == 0 || data == 0 || dsize == 0)
	{
		return 0;
	}

	if (strlen(file) == 0)
	{
		return 0;
	}

	file_stream = fopen(file, "ab+");
	if (file_stream == 0)
	{
		//printf("fopen err\n");
		return 0;
	}

	write_size = fwrite(data, 1, dsize, file_stream);
	if (write_size == 0 && write_size != dsize)
	{
		//printf("fread err  [%d]\n",ferror(file_stream));
		fclose(file_stream);
		return 0;
	}

	fclose(file_stream);

	return 1;
}

/*get file size*/
DLL_PUBLIC int xx_get_file_size(char* filename)
{
	int iret = 0;
	FILE *file_stream = 0;
	int  file_size = 0;

	if (filename == 0 || strlen(filename) == 0)
	{
		return 0;
	}


	file_stream = fopen(filename, "rb");
	if (file_stream == 0)
	{
		//printf("fopen err\n");
		return 0;
	}
	iret = fseek(file_stream, 0, SEEK_END);
	if (iret)
	{
		//printf("fseek err\n");
		fclose(file_stream);
		return 0;
	}

	file_size = ftell(file_stream);
	if (file_size == 0)
	{
		//printf("ftell err\n");
		fclose(file_stream);
		return 0;
	}
	fclose(file_stream);
	return file_size;
}


/*read file
arg1   filename
arg2   buff
arg3   offset
arg4   size
*/
DLL_PUBLIC int xx_read_file(char *filename, char* data, unsigned int fileoffset, unsigned int size)
{
	int iret = 0;
	FILE *file_stream = 0;
	unsigned int read_size = 0;
	unsigned int file_size = 0;

	if (filename == 0 || data == 0 || size == 0 || strlen(filename) == 0)
	{
		return 0;
	}
	file_size = xx_get_file_size(filename);
	if ((fileoffset + size) > file_size)
	{
		return 0;
	}

	file_stream = fopen(filename, "rb");
	if (file_stream == 0)
	{
		//printf("fopen err\n");
		return 0;
	}

	iret = fseek(file_stream, fileoffset, SEEK_SET);
	if (iret)
	{
		//printf("fseek err\n");
		fclose(file_stream);
		return 0;
	}

	read_size = fread(data, 1, size, file_stream);
	if (read_size == 0 && read_size != size)
	{
		//printf("fread err  [%d]\n",ferror(file_stream));
		fclose(file_stream);
		return 0;
	}
	fclose(file_stream);
	return 1;
}


/*cover over the file*/
DLL_PUBLIC int xx_cover_file(char *file, char* data, int dsize)
{
	FILE *file_stream = 0;
	int write_size = 0;

	if (file == 0 || data == 0 || dsize == 0)
	{
		return 0;
	}

	if (strlen(file) == 0)
	{
		return 0;
	}

	file_stream = fopen(file, "wb+");
	if (file_stream == 0)
	{
		//printf("fopen err\n");
		return 0;
	}

	write_size = fwrite(data, 1, dsize, file_stream);
	if (write_size == 0 && write_size != dsize)
	{
		//printf("fread err  [%d]\n",ferror(file_stream));
		fclose(file_stream);
		return 0;
	}

	fclose(file_stream);

	return 1;
}



/*
附加文件2到文件1
*/
DLL_PUBLIC int xx_append_ff(char *file1, char *file2)
{
	int iret = 0;
	int filesize = 0;
	unsigned char *filedata = 0;

	filesize = xx_get_file_size(file2);
	if (filesize == 0)
	{
		return 0;
	}
	filedata = (unsigned char *)malloc(filesize + 1);
	if (filedata == 0)
	{
		return 0;
	}
	memset(filedata, 0, filesize + 1);
	iret = xx_read_file((char*)file2, (char*)filedata, 0, filesize);
	if (iret == 0)
	{
		free(filedata);
		return 0;
	}

	iret = xx_append_file(file1, (char*)filedata, filesize);
	if (iret == 0)
	{
		free(filedata);
		return 0;
	}

	free(filedata);
	return 1;
}


/*
在文件指定位置写入数据
*/

DLL_PUBLIC int xx_scover_file(char *file, unsigned char *pdata, unsigned int begin, unsigned int size)
{
	int iret = 0;
	unsigned int filesize = 0;
	FILE *file_stream = 0;
	unsigned int write_size = 0;


	/*判断开始位置+数据大小是否超出文件大小*/
	filesize = xx_get_file_size(file);
	if (filesize == 0)
	{
		return 0;
	}
	if ((begin + size) > filesize)
	{
		return 0;
	}

	file_stream = fopen(file, "rb+");
	if (file_stream == 0)
	{
		return 0;
	}

	iret = fseek(file_stream, begin, SEEK_SET);
	if (iret)
	{
		fclose(file_stream);
		return 0;
	}

	write_size = fwrite(pdata, 1, size, file_stream);
	if (write_size == 0 && write_size != size)
	{
		fclose(file_stream);
		return 0;
	}

	fclose(file_stream);

	return 1;
}