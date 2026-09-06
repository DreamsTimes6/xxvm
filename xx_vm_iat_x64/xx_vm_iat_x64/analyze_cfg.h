#include <windows.h>





#ifdef __cplusplus
extern "C" {
#endif

	int  xx_cfg_init(char *xdbg_path);

	/*ÉèÖÃÅäÖÃ*/
	int xx_cfg_set_int(int ikey, int in_var);

	/*¶ÁÈ¡ÅäÖÃÖ»ÓĞÒ»¸ösection*/
	int xx_cfg_get_int(int ikey, int *out_var);

	/*ÉèÖÃÅäÖÃ×Ö·û´®*/
	int xx_cfg_set_str(char* pkey, char* pvar);

	/*¶ÁÈ¡ÅäÖÃ×Ö·û´®*/
	int xx_cfg_get_str(char* pkey, char* out_var, int out_size);

#ifdef __cplusplus
}
#endif


