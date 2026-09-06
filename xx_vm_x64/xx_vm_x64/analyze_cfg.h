#include <windows.h>





#ifdef __cplusplus
extern "C" {
#endif

#define XX_ANA_COUNT   1    //分析次数
#define XX_ANA_RULE    2	//分析规则，正常和按次数分析，循环
#define XX_OPMZ_DATA   3    //优化数据
#define XX_LOG_FLAG    4    //是否关联程序打开
#define XX_LOGIN_AUTO    5    //是否自动登陆



	int xx_cfg();

	int  xx_cfg_init(char *xdbg_path);

	/*设置配置*/
	int xx_cfg_set_int(int ikey, int in_var);

	/*读取配置只有一个section*/
	int xx_cfg_get_int(int ikey, int *out_var);

	/*设置配置字符串*/
	int xx_cfg_set_str(char* pkey, char* pvar);

	/*读取配置字符串*/
	int xx_cfg_get_str(char* pkey, char* out_var, int out_size);

#ifdef __cplusplus
}
#endif


