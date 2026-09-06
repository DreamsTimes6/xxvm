#include <stdio.h>





int get_reg_index(char *reg_def)
{
	int n = 0;
	int m = 0;
	char tmp_def[100];

	memset(tmp_def, 0, sizeof(tmp_def));
	nstr_hex(reg_def, strlen(reg_def), tmp_def);

	n = tmp_def[0];
	
	m = tmp_def[1];

	
	return n*8+m;
}





int xreg_cmp(char* def1, char* def2)
{
	int n = 0;
	char buf1[100];
	char buf2[100];

	memset(buf1, 0, sizeof(buf1));
	memset(buf2, 0, sizeof(buf2));

	nstr_hex(def1, strlen(def1), buf1);
	nstr_hex(def2, strlen(def2), buf2);

	if (buf1[0] == buf2[0] && buf1[1] == buf2[1])
	{
		if (buf1[4] == buf2[4] && buf1[5] == buf2[5])
		{
			return 0;
		}
	}
	return -1;
}

int xreg_incmp(char* def1, char* def2)
{
	int n = 0;
	char reg[100];
	char sub[100];

	memset(reg, 0, sizeof(reg));
	memset(sub, 0, sizeof(sub));

	nstr_hex(def1, strlen(def1), reg);
	nstr_hex(def2, strlen(def2), sub);

	if (reg[0] == sub[0] && reg[1] == sub[1])
	{
		if (reg[4] >= sub[4])
		{
			return 0;
		}
	}
	return -1;
}




