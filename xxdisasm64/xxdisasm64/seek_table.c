#include <stdio.h>
#include <string.h>

#include "g_sql.h"
#include "xx_def.h"


extern int nstr_hex(char* str, int len, char* out);



 int seek_one_opcode_tbl(struct XX_TBL_ONE_OPCODE *tables,int nrows,uchar *first_opcode,int inst_system)
{
	int n;
	uchar tmp[200];

	if(first_opcode==0 || strlen(first_opcode)!=2)
	{
		return -1;
	}

	memset(tmp,0,sizeof(tmp));

	if(strlen(first_opcode))
	{
		strcat(tmp,first_opcode);
	}
	else
	{
		strcat(tmp,"xx");
	}

	for (n = 0; n < nrows; n++)
	{
		if (strcmp(tables[n].search_index, tmp) == 0)
		{
			if (inst_system == 8)
			{

				if (strcmp(tables[n].rex_flag, "1") == 0)
				{
					return n;
				}
				else
				{
					if (memcmp(tables[n].superscripts, "i64", strlen("i64")) != 0)
					{
						return n;
					}
				}
			}
			else
			{
				if (memcmp(tables[n].superscripts, "o64", strlen("o64")) != 0)
				{
					return n;
				}
			}
		}

	}
	return -1;
}



 int seek_two_opcode_tbl(struct XX_TBL_TWO_OPCODE *tables,int nrows,uchar *second_opcode,uchar *prefix)
{
	int n;
	uchar tmp[200];

	if(second_opcode==0 || strlen(second_opcode)!=2 || prefix==0)
	{
		return -1;
	}

	memset(tmp,0,sizeof(tmp));

	if(strlen(second_opcode))
	{
		strcat(tmp,second_opcode);
	}
	else
	{
		strcat(tmp,"xx");
	}

	for(n=0;n<nrows;n++)
	{
		if(strcmp(tables[n].search_index,tmp)==0)
		{
			if(strlen(prefix))
			{
				if(strstr(tables[n].prefix_group,prefix))
				{
					return n;
				}
			}
			else
			{
				if(strstr(tables[n].prefix_group,"xx"))
				{
					return n;
				}
			}
		}
	}
	return -1;

}


 int seek_three_opcode_tbl(struct XX_TBL_THREE_OPCODE *tables,int nrows,uchar *second_opcode,uchar *third_opcode,\
		int nprefix,uchar *prefix)
{
	int n;
	uchar tmp[200];


	if(second_opcode==0 || strlen(second_opcode)!=2 )
	{
		return -1;
	}
	if(third_opcode==0 || strlen(third_opcode)!=2 )
	{
		return -1;
	}

	memset(tmp,0,sizeof(tmp));

	if(strlen(second_opcode))
	{
		strcat(tmp,second_opcode);
	}
	else
	{
		strcat(tmp,"xx");
	}
	if(strlen(third_opcode))
	{
		strcat(tmp,third_opcode);
	}
	else
	{
		strcat(tmp,"xx");
	}
	for(n=0;n<nprefix;n++)
	{

		if(strlen(prefix+n*2))
		{
			memcpy(tmp,prefix+n*2,2);
		}
		else
		{
			strcat(tmp,"xx");
		}
	}


	for(n=0;n<nrows;n++)
	{
		if (memcmp(tables[n].search_index, tmp, strlen(tmp)) == 0)
		{
			return n;
		}
	}
	return -1;
}


 int seek_ext_one_opcode_tbl(struct XX_TBL_EXT_ONE_OPCODE *tables,int nrows,uchar *opcode,\
		uchar *mod,uchar *reg_opcode,uchar *rm)
{
	int n;
	uchar tmp[200];

	if(opcode==0 || strlen(opcode)!=2)
	{
		return -1;
	}

	if(mod==0 || strlen(mod)!=2)
	{
		return -1;
	}

	memset(tmp,0,sizeof(tmp));

	
	if(strlen(opcode))
	{
		strcat(tmp,opcode);
	}
	else
	{
		strcat(tmp,"xx");
	}
	if(strlen(reg_opcode))
	{
		strcat(tmp,reg_opcode);
	}
	else
	{
		strcat(tmp,"xx");
	}


	for(n=0;n<nrows;n++)
	{

		if(strcmp(tables[n].search_index,tmp)==0)
		{
			if(strstr(tables[n].mod_group,mod))
			{
				if(strstr(tables[n].rm_group,rm))
				{
					return n;
				}
			}
		}
	}

	return -1;
}



 int seek_ext_two_opcode_tbl(struct XX_TBL_EXT_TWO_OPCODE *tables,int nrows,uchar *second_opcode,\
		uchar *prefix,uchar *mod,uchar *reg_opcode,uchar *rm)
{
	int n;
	uchar tmp[200];

	if(second_opcode==0 || strlen(second_opcode)!=2)
	{
		return -1;
	}

	if(mod==0 || strlen(mod)!=2)
	{
		return -1;
	}

	memset(tmp,0,sizeof(tmp));

	if(strlen(second_opcode))
	{
		strcat(tmp,second_opcode);
	}
	else
	{
		strcat(tmp,"xx");
	}
	if(strlen(reg_opcode))
	{
		strcat(tmp,reg_opcode);
	}
	else
	{
		strcat(tmp,"xx");
	}


	for(n=0;n<nrows;n++)
	{
		if(strcmp(tables[n].search_index,tmp)==0)
		{
			if(strstr(tables[n].mod_group,mod))
			{
				if(strstr(tables[n].rm_group,rm))
				{
					if(strlen(prefix))
					{
						if(strstr(tables[n].prefix_group,prefix))
						{
							return n;
						}
					}
					else
					{
						return n;
					}
				}
			}
		}
	}

	return -1;
}


 int seek_ext_three_opcode_tbl(struct XX_TBL_EXT_THREE_OPCODE *tables,int nrows,uchar *second_opcode,\
		uchar *third_opcode,uchar *prefix,uchar *mod,uchar *reg_opcode,uchar *rm)
{
	int n;
	uchar tmp[200];

	if(second_opcode==0 || strlen(second_opcode)!=2)
	{
		return -1;
	}

	if(third_opcode==0 || strlen(third_opcode)!=2)
	{
		return -1;
	}

	if(mod==0 || strlen(mod)!=2)
	{
		return -1;
	}

	memset(tmp,0,sizeof(tmp));


	if(strlen(second_opcode))
	{
		strcat(tmp,second_opcode);
	}
	else
	{
		strcat(tmp,"xx");
	}
	if(strlen(third_opcode))
	{
		strcat(tmp,third_opcode);
	}
	else
	{
		strcat(tmp,"xx");
	}
	if(strlen(prefix))
	{
		strcat(tmp,prefix);
	}
	else
	{
		strcat(tmp,"xx");
	}
	if(strlen(reg_opcode))
	{
		strcat(tmp,reg_opcode);
	}
	else
	{
		strcat(tmp,"xx");
	}
	if(strlen(rm))
	{
		strcat(tmp,rm);
	}
	else
	{
		strcat(tmp,"xx");
	}


	for(n=0;n<nrows;n++)
	{
		if(strcmp(tables[n].search_index,tmp)==0)
		{
			if(strstr(tables[n].mod_group,mod))
			{
				return n;
			}
		}
	}


	return -1;
}


 int seek_item_type_tbl(struct XX_TBL_ITEM_TYPE* tbl_item_type,int rows,uchar* type)
{
	int n=0;
	for(n=0;n<rows;n++)
	{
		if(strcmp(type,tbl_item_type[n].type_mic)==0)
		{
			return n;
		}
	}
	return -1;

}

 int seek_item_ltype_tbl(struct XX_TBL_ITEM_LTYPE* tbl_item_ltype,int rows,uchar* ltype)
{
	int n=0;
	for(n=0;n<rows;n++)
	{
		if(strcmp(ltype,tbl_item_ltype[n].ltype_mic)==0)
		{
			return n;
		}
	}
	return -1;

}


 int seek_fpu_opcode_tbl(struct XX_TBL_FPU_OPCODE* tbl_fpu_opcode,int rows,uchar* sbyte,uchar* smodrm)
{
	int n=0;

	uchar buf2[20];
	uchar bmodrm[20];

	if(strlen(sbyte)<1 || strlen(smodrm)<1)
	{
		return -1;
	}

	memset(bmodrm,0,sizeof(bmodrm));
	nstr_hex(smodrm,strlen(smodrm),bmodrm);

	if(*bmodrm>=0x00 && *bmodrm<=0xbf)
	{
		for(n=0;n<rows;n++)
		{
			if(strcmp(tbl_fpu_opcode[n].opcode,sbyte)==0 && \
					strcmp(tbl_fpu_opcode[n].modrm_flag,"1")==0)
			{
				if(strlen(tbl_fpu_opcode[n].modrm)==0)
				{
					return -1;
				}
				memset(buf2,0,sizeof(buf2));
				nstr_hex(tbl_fpu_opcode[n].modrm,strlen(tbl_fpu_opcode[n].modrm),buf2);
				if(*buf2==(*bmodrm&0x38))
				{
					return n;
				}
			}
		}
	}
	else if(*bmodrm>=0xc0 && *bmodrm<=0xff)
	{
		for(n=0;n<rows;n++)
		{
			if(strcmp(tbl_fpu_opcode[n].opcode,sbyte)==0 && \
					strcmp(tbl_fpu_opcode[n].modrm_flag,"2")==0)
			{
				if(strcmp(tbl_fpu_opcode[n].modrm,smodrm)==0)
				{
					return n;
				}
			}
		}
	}
	return -1;

}
