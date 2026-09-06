#include <stdio.h>
#include <string.h>
#include "xx_comm32.h"

#include "xx_reg2.h"




//int xx_get_reg_mic(unsigned char *reg_def,char* reg_mic);



int reg_def(uchar* src_def,uchar* des_def,int rexflag,int dsize)
{
	uchar tmp[200];
	int length;

	memset(tmp,0,sizeof(tmp));
	
	if(!src_def || !des_def || !dsize || dsize>0x000000ff || !strlen(src_def))
	{

		return 0;
	}
	

	length=strlen(src_def);
	nstr_hex(src_def,length,tmp);


	if(tmp[5]==0 && tmp[4] == 0)
	{
		if(dsize==8)
		{
			tmp[5]=6+tmp[6]-1;
		}
		else if(dsize==4)
		{
			tmp[5]=4+tmp[6]-1;
		}
		else if(dsize==2)
		{
			tmp[5]=2+tmp[6]-1;
		}
		else if(dsize==1)
		{
			tmp[5]=0+tmp[6]-1;
		}
	}

	if ((tmp[4] & 0xff) == 0)
	{
		tmp[4] = dsize;
	}

	if(tmp[4]>tmp[2] || tmp[4]<tmp[3])
	{

		return 0;
	}

	if (rexflag == 1 && dsize == 1)
	{
		if (tmp[0] == 0 && tmp[1] >= 0 && tmp[1] < 4 && tmp[5] == 1)
		{
			tmp[1] = tmp[1] + 4;
			tmp[5] = 0;
		}
	}

	nhex_str(tmp,length/2,des_def,"");

	return length;
}


DLL_PUBLIC int xx_get_reg_mic(unsigned char *reg_def,char* reg_mic)
{
	char tmp[200];
	char *sub_reg;

	memset(tmp,0,sizeof(tmp));
	nstr_hex(reg_def,strlen(reg_def),tmp);

	sub_reg=reg_def_data[(tmp[1]+tmp[0]*0x10)][tmp[5]];

	memcpy(reg_mic,sub_reg,strlen(sub_reg));

	return 1;
}


int xx_get_reg_mic2(unsigned long ireg_def,char* reg_mic,unsigned long mic_size)
{
	char tmp[200];
	char *sub_reg;
	unsigned long index1=0;
	unsigned long index2=0;
	unsigned long index3=0;

	index1=0xff000000 & ireg_def;
	index2=0x00ff0000 & ireg_def;
	index3=0x000000ff & ireg_def;


	sub_reg=reg_def_data[(index2+index1*0x10)][index3];

	if(strlen(sub_reg)>=mic_size)
	{
		return 0;
	}
	memcpy(reg_mic,sub_reg,strlen(sub_reg));

	return 1;
}





