#include <stdio.h>
#include "xx_comm64.h"




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






