#include <string.h>
#include <stdlib.h>

#include "xx_comm32.h"
#include "xx_inst.h"
#include "xx_err.h"




extern struct XX_INST *g_inst;
extern int g_count;
extern ulong err_code;

  DLL_PUBLIC int xx_get_item_count(ulong seq,ulong *pitem_count)
{
	struct XX_INST *xx_inst=0;
	if(pitem_count==0)
	{
		err_code=INST_ERR_ARGUMENT;
		return false;
	}

	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return false;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return false;
	}
	*pitem_count=(ulong)xx_inst->xx_inst_items.nitem;
	err_code=INST_ERR_SUCCESS;
	return true;
}


  DLL_PUBLIC int xx_get_item_type(ulong seq,ulong item_seq)
{
	struct XX_INST *xx_inst=0;
	int item_type=0;

	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	if(item_seq>=xx_inst->xx_inst_items.nitem)
	{
		err_code=INST_ERR_ITEM_SEQ;
		return 0;
	}

	err_code=INST_ERR_SUCCESS;
	return xx_inst->xx_inst_items.xx_inst_items_flag[item_seq].item_type;
}


  DLL_PUBLIC int xx_get_item_mic(ulong seq,ulong item_seq,uchar *buf,int bufsize)
{
	struct XX_INST *xx_inst=0;
	if(buf==0 || bufsize==0)
	{
		err_code=INST_ERR_ARGUMENT;
		return false;
	}

	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return false;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return false;
	}
	if(strlen(xx_inst->xx_inst_items.xx_inst_items_mic[item_seq].item_mic)>bufsize)
	{
		err_code=INST_ERR_BUFSIZE;
		return 0;
	}
	memset(buf,0,bufsize);
	memcpy(buf,xx_inst->xx_inst_items.xx_inst_items_mic[item_seq].item_mic,\
			strlen(xx_inst->xx_inst_items.xx_inst_items_mic[item_seq].item_mic));
	err_code=INST_ERR_SUCCESS;
	return true;
}



  DLL_PUBLIC int xx_get_item_data_size(ulong seq,ulong item_seq)
{
	struct XX_INST *xx_inst=0;


	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	if(item_seq>=xx_inst->xx_inst_items.nitem)
	{
		err_code=INST_ERR_ITEM_SEQ;
		return 0;
	}
	err_code=INST_ERR_SUCCESS;
	return xx_inst->xx_inst_items.xx_inst_items_var[item_seq].item_data_size;
}


  DLL_PUBLIC int xx_get_item_addr_size(ulong seq,ulong item_seq)
{
	struct XX_INST *xx_inst=0;


	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	if(item_seq>=xx_inst->xx_inst_items.nitem)
	{
		err_code=INST_ERR_ITEM_SEQ;
		return 0;
	}
	err_code=INST_ERR_SUCCESS;
	return xx_inst->xx_inst_items.xx_inst_items_var[item_seq].item_addr_size;
}



DLL_PUBLIC int xx_cmp_item(ulong seq1,ulong item_seq1,ulong seq2,ulong item_seq2)
{
	struct XX_INST *xx_inst1=0;
	struct XX_INST *xx_inst2=0;

	if(seq1>=g_count || seq2>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst1=&g_inst[seq1];
	xx_inst2=&g_inst[seq2];
	if(xx_inst1==0 || xx_inst2==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	if(item_seq1>=xx_inst1->xx_inst_items.nitem || item_seq2>=xx_inst2->xx_inst_items.nitem)
	{
		err_code=INST_ERR_ITEM_SEQ;
		return 0;
	}

	err_code=INST_ERR_SUCCESS;
	if(xx_inst1->xx_inst_items.xx_inst_items_flag[item_seq1].item_type==xx_inst2->xx_inst_items.xx_inst_items_flag[item_seq2].item_type && \
			memcmp(&xx_inst1->xx_inst_items.xx_inst_items_var[item_seq1],\
				&xx_inst2->xx_inst_items.xx_inst_items_var[item_seq2],\
				sizeof(xx_inst1->xx_inst_items.xx_inst_items_var[item_seq1]))==0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
	return 1;
}



DLL_PUBLIC int xx_ecmp_item(ulong seq1,ulong item_seq1,ulong seq2,ulong item_seq2)
{
	struct XX_INST *xx_inst1=0;
	struct XX_INST *xx_inst2=0;

	if(seq1>=g_count || seq2>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst1=&g_inst[seq1];
	xx_inst2=&g_inst[seq2];
	if(xx_inst1==0 || xx_inst2==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	if(item_seq1>=xx_inst1->xx_inst_items.nitem || item_seq2>=xx_inst2->xx_inst_items.nitem)
	{
		err_code=INST_ERR_ITEM_SEQ;
		return 0;
	}

	err_code=INST_ERR_SUCCESS;
	return 1;
}



  DLL_PUBLIC int xx_get_item_regdef(ulong seq,ulong item_seq,ulong *preg_def)
{
	struct XX_INST *xx_inst=0;
	uchar tmp[200];
	ulong var=0;

	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	if(item_seq>=xx_inst->xx_inst_items.nitem)
	{
		err_code=INST_ERR_ITEM_SEQ;
		return 0;
	}

	if(xx_inst->xx_inst_items.xx_inst_items_var[item_seq].reg_def_len==0)
	{
		err_code=INST_ERR_UNDEFINE;
		return 0;
	}

	memset(tmp,0,sizeof(tmp));
	nstr_hex(xx_inst->xx_inst_items.xx_inst_items_var[item_seq].item_reg_def,\
			xx_inst->xx_inst_items.xx_inst_items_var[item_seq].reg_def_len,\
			tmp);	
	*preg_def=0;

	var=0xff000000 & (tmp[0]<<24);
	*preg_def=*preg_def | var;

	var=0x00ff0000 & (tmp[1]<<16);
	*preg_def=*preg_def | var;

	var=0x0000ff00 & (tmp[4]<<8);
	*preg_def=*preg_def | var;

	var=0x000000ff & (tmp[5]<<0);
	*preg_def=*preg_def | var;

	err_code=INST_ERR_SUCCESS;
	return 1;
}



  DLL_PUBLIC int xx_get_item_basedef(ulong seq,ulong item_seq,ulong *pbase_def)
{
	struct XX_INST *xx_inst=0;
	uchar tmp[200];
	ulong var=0;

	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	if(item_seq>=xx_inst->xx_inst_items.nitem)
	{
		err_code=INST_ERR_ITEM_SEQ;
		return 0;
	}

	if(xx_inst->xx_inst_items.xx_inst_items_var[item_seq].base_def_len==0)
	{
		err_code=INST_ERR_UNDEFINE;
		return 0;
	}

	memset(tmp,0,sizeof(tmp));
	nstr_hex(xx_inst->xx_inst_items.xx_inst_items_var[item_seq].item_base_def,\
			xx_inst->xx_inst_items.xx_inst_items_var[item_seq].base_def_len,\
			tmp);	
	*pbase_def=0;

	var=0xff000000 & (tmp[0]<<24);
	*pbase_def=*pbase_def | var;

	var=0x00ff0000 & (tmp[1]<<16);
	*pbase_def=*pbase_def | var;

	var=0x0000ff00 & (tmp[4]<<8);
	*pbase_def=*pbase_def | var;

	var=0x000000ff & (tmp[5]<<0);
	*pbase_def=*pbase_def | var;

	err_code=INST_ERR_SUCCESS;
	return 1;
}



  DLL_PUBLIC int xx_get_item_indexdef(ulong seq,ulong item_seq,ulong *pindex_def)
{
	struct XX_INST *xx_inst=0;
	uchar tmp[200];
	ulong var=0;

	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	if(item_seq>=xx_inst->xx_inst_items.nitem)
	{
		err_code=INST_ERR_ITEM_SEQ;
		return 0;
	}

	if(xx_inst->xx_inst_items.xx_inst_items_var[item_seq].index_def_len==0)
	{
		err_code=INST_ERR_UNDEFINE;
		return 0;
	}

	memset(tmp,0,sizeof(tmp));
	nstr_hex(xx_inst->xx_inst_items.xx_inst_items_var[item_seq].item_index_def,\
			xx_inst->xx_inst_items.xx_inst_items_var[item_seq].index_def_len,\
			tmp);	
	*pindex_def=0;

	var=0xff000000 & (tmp[0]<<24);
	*pindex_def=*pindex_def | var;

	var=0x00ff0000 & (tmp[1]<<16);
	*pindex_def=*pindex_def | var;

	var=0x0000ff00 & (tmp[4]<<8);
	*pindex_def=*pindex_def | var;

	var=0x000000ff & (tmp[5]<<0);
	*pindex_def=*pindex_def | var;

	err_code=INST_ERR_SUCCESS;
	return 1;
}



  DLL_PUBLIC int xx_get_item_scale(ulong seq,ulong item_seq,ulong *pscale)
{
	struct XX_INST *xx_inst=0;
	uchar tmp[200];
	ulong var=0;

	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	if(item_seq>=xx_inst->xx_inst_items.nitem)
	{
		err_code=INST_ERR_ITEM_SEQ;
		return 0;
	}

	if(xx_inst->xx_inst_items.xx_inst_items_var[item_seq].index_scale==0)
	{
		err_code=INST_ERR_UNDEFINE;
		return 0;
	}

	*pscale=xx_inst->xx_inst_items.xx_inst_items_var[item_seq].index_scale;
	err_code=INST_ERR_SUCCESS;
	return 1;
}



  DLL_PUBLIC int xx_get_item_const(ulong seq,ulong item_seq,uchar *pconst)
{
	struct XX_INST *xx_inst=0;
	uchar tmp[200];
	ulong var=0;

	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	if(item_seq>=xx_inst->xx_inst_items.nitem)
	{
		err_code=INST_ERR_ITEM_SEQ;
		return 0;
	}

	if(xx_inst->xx_inst_items.xx_inst_items_var[item_seq].const_size==0)
	{
		err_code=INST_ERR_UNDEFINE;
		return 0;
	}

	memcpy(pconst,xx_inst->xx_inst_items.xx_inst_items_var[item_seq].item_const,\
			xx_inst->xx_inst_items.xx_inst_items_var[item_seq].const_size);
	err_code=INST_ERR_SUCCESS;
	return 1;
}



















