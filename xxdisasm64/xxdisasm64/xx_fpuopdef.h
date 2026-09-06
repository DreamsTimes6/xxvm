#include <stdio.h>
#include "xx_def.h"


//d8
#define    op_fadd        "011001"
#define    op_fmul        "011002"
#define    op_fcom        "011003"
#define    op_fcomp       "011004"
#define    op_fsub        "011005"
#define    op_fsubr       "011006"
#define    op_fdiv        "011007"
#define    op_fdivr       "011008"

//d9
#define    op_fld        "011009"
#define    op_fst        "01100a"
#define    op_fstp       "01100b"
#define    op_fldenv     "01100c"
#define    op_fldcw      "01100d"
#define    op_fstenv     "01100e"
#define    op_fstcw      "01100f"

//#define    op_fld        "011010"
#define    op_fnop       "011011"
#define    op_fchs       "011012"
#define    op_fabs       "011013"
#define    op_ftst       "011014"
#define    op_fxam       "011015"
#define    op_f2xm1      "011016"
#define    op_fyl2x      "011017"
#define    op_fptan      "011018"
#define    op_fpatan     "011019"
#define    op_fxtract    "01101a"
#define    op_fprem1     "01101b"
#define    op_fdecstp    "01101c"
#define    op_fincstp    "01101d"
#define    op_fxch       "01101e"
#define    op_fld1       "01101f"
#define    op_fldl2t     "011020"
#define    op_fldl2e     "011021"
#define    op_fldpi      "011022"
#define    op_fldlg2     "011023"
#define    op_fldln2     "011024"
#define    op_fldz       "011025"
#define    op_fprem      "011026"
#define    op_fyl2xp1    "011027"
#define    op_fsqrt      "011028"
#define    op_fsincos    "011029"
#define    op_frndint    "01102a"
#define    op_fscale     "01102b"
#define    op_fsin       "01102c"
#define    op_fcos       "01102d"


//da
#define    op_fiadd        "01102e"
#define    op_fimul        "01102f"
#define    op_ficom        "011030"
#define    op_ficomp       "011031"
#define    op_fisub        "011032"
#define    op_fisubr       "011033"
#define    op_fidiv        "011034"
#define    op_fidivr       "011035"
#define    op_fcmovb       "011036"
#define    op_fcmovbe      "011037"
#define    op_fcmove       "011038"
#define    op_fcmovu       "011039"
#define    op_fucompp      "01103a"


//db
#define    op_fild        "01103b"
#define    op_fisttp      "01103c"
#define    op_fist        "01103d"
#define    op_fistp       "01103e"
#define    op_fld         "01103f"
#define    op_fstp        "011040"
#define    op_fcmovnb     "011041"
#define    op_fcmovnbe    "011042"
#define    op_fclex       "011043"
#define    op_finit       "011044"
#define    op_fcomi       "011045"
#define    op_fcmovne     "011046"
#define    op_fcmovnu     "011047"
#define    op_fucomi      "011048"

//dc dd de df
#define    op_frstor      "011049"
#define    op_fsave       "01104a"
#define    op_fstsw       "01104b"



















