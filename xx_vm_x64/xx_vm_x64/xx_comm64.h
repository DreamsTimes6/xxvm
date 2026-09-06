#include <stdio.h>
#include "xx_def.h"

#pragma pack(push)
#pragma pack(1)

#pragma comment(lib,"xx_comm64.lib")

#ifdef __cplusplus
extern "C" {
#endif


int nstr_hex(char* str, int len, unsigned char* out);
int nhex_str(unsigned char *phex, int nhex, char *str_buff, char *other_char);
int  nhex_char(unsigned char *phex, int nchar, char *str_buff);
int dump_hex(unsigned char *phex, int hex_size, char *str_buff, unsigned int str_size, char *other_char);
int byte_opposite(unsigned char* src_hex, int src_len, uchar* des_hex);

//char *file_open(char *filename, int *out_size);

#if 0
int seek_one_opcode_tbl(struct XX_TBL_ONE_OPCODE *tables, int nrows, unsigned char *first_opcode,int);
int seek_two_opcode_tbl(struct XX_TBL_TWO_OPCODE *tables, int nrows, unsigned char *second_opcode, unsigned char *prefix);
int seek_three_opcode_tbl(struct XX_TBL_THREE_OPCODE *tables, int nrows, uchar *second_opcode, uchar *third_opcode, \
	int nprefix, uchar *prefix);
int seek_ext_one_opcode_tbl(struct XX_TBL_EXT_ONE_OPCODE *tables, int nrows, uchar *opcode, \
	uchar *mod, uchar *reg_opcode, uchar *rm);
int seek_ext_two_opcode_tbl(struct XX_TBL_EXT_TWO_OPCODE *tables, int nrows, uchar *second_opcode, \
	uchar *prefix, uchar *mod, uchar *reg_opcode, uchar *rm);
int seek_ext_three_opcode_tbl(struct XX_TBL_EXT_THREE_OPCODE *tables, int nrows, uchar *second_opcode, \
	uchar *third_opcode, uchar *prefix, uchar *mod, uchar *reg_opcode, uchar *rm);
int seek_item_type_tbl(struct XX_TBL_ITEM_TYPE* tbl_item_type, int rows, uchar* type);
int seek_item_ltype_tbl(struct XX_TBL_ITEM_LTYPE* tbl_item_ltype, int rows, uchar* ltype);

int init_sqlfile();
int free_sqlfile();

void* get_sql_struct(int tbl);
void* get_sql_lstruct(int tbl);
int get_sql_rows_num(int tbl);
int get_sql_lrows_num(int tbl);
int get_sql_item_size(int tbl);
int get_sql_litem_size(int tbl);
int get_sql_list_num();

struct XX_INIT_TBL_LIST* get_sql_list_st();
#endif

//file
int xx_append_file(char *file, char* data, int dsize);
int xx_cover_file(char *file, char* data, int dsize);
int xx_get_file_size(char* filename);
int xx_read_file(char *filename, char* data, unsigned int fileoffset, unsigned int size);
int xx_append_ff(char *file1, char *file2);
int xx_scover_file(char *file, unsigned char *pdata, unsigned int begin, unsigned int size);



#ifdef __cplusplus
}
#endif



#pragma pack(pop)
