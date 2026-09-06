#include <stdio.h>
#include "xx_def.h"


//d8
#define    op_fadd        0x011001
#define    op_fmul        0x011002
#define    op_fcom        0x011003
#define    op_fcomp       0x011004
#define    op_fsub        0x011005
#define    op_fsubr       0x011006
#define    op_fdiv        0x011007
#define    op_fdivr       0x011008

//d9
#define    op_fld        0x011009
#define    op_fst        0x01100a
#define    op_fstp       0x01100b
#define    op_fldenv     0x01100c
#define    op_fldcw      0x01100d
#define    op_fstenv     0x01100e
#define    op_fstcw      0x01100f

//#define    op_fld        0x011010
#define    op_fnop       0x011011
#define    op_fchs       0x011012
#define    op_fabs       0x011013
#define    op_ftst       0x011014
#define    op_fxam       0x011015
#define    op_f2xm1      0x011016
#define    op_fyl2x      0x011017
#define    op_fptan      0x011018
#define    op_fpatan     0x011019
#define    op_fxtract    0x01101a
#define    op_fprem1     0x01101b
#define    op_fdecstp    0x01101c
#define    op_fincstp    0x01101d
#define    op_fxch       0x01101e
#define    op_fld1       0x01101f
#define    op_fldl2t     0x011020
#define    op_fldl2e     0x011021
#define    op_fldpi      0x011022
#define    op_fldlg2     0x011023
#define    op_fldln2     0x011024
#define    op_fldz       0x011025
#define    op_fprem      0x011026
#define    op_fyl2xp1    0x011027
#define    op_fsqrt      0x011028
#define    op_fsincos    0x011029
#define    op_frndint    0x01102a
#define    op_fscale     0x01102b
#define    op_fsin       0x01102c
#define    op_fcos       0x01102d


//da
#define    op_fiadd        0x01102e
#define    op_fimul        0x01102f
#define    op_ficom        0x011030
#define    op_ficomp       0x011031
#define    op_fisub        0x011032
#define    op_fisubr       0x011033
#define    op_fidiv        0x011034
#define    op_fidivr       0x011035
#define    op_fcmovb       0x011036
#define    op_fcmovbe      0x011037
#define    op_fcmove       0x011038
#define    op_fcmovu       0x011039
#define    op_fucompp      0x01103a


//db
#define    op_fild        0x01103b
#define    op_fisttp      0x01103c
#define    op_fist        0x01103d
#define    op_fistp       0x01103e
//#define    op_fld         0x01103f
//#define    op_fstp        0x011040
#define    op_fcmovnb     0x011041
#define    op_fcmovnbe    0x011042
#define    op_fclex       0x011043
#define    op_finit       0x011044
#define    op_fcomi       0x011045
#define    op_fcmovne     0x011046
#define    op_fcmovnu     0x011047
#define    op_fucomi      0x011048

//dc dd de df
#define    op_frstor      0x011049
#define    op_fsave       0x01104a
#define    op_fstsw       0x01104b





/////////////////////////////////////////////




//d8
#define    sop_fadd        "011001"
#define    sop_fmul        "011002"
#define    sop_fcom        "011003"
#define    sop_fcomp       "011004"
#define    sop_fsub        "011005"
#define    sop_fsubr       "011006"
#define    sop_fdiv        "011007"
#define    sop_fdivr       "011008"

//d9
#define    sop_fld        "011009"
#define    sop_fst        "01100a"
#define    sop_fstp       "01100b"
#define    sop_fldenv     "01100c"
#define    sop_fldcw      "01100d"
#define    sop_fstenv     "01100e"
#define    sop_fstcw      "01100f"

//#define    sop_fld        "011010"
#define    sop_fnop       "011011"
#define    sop_fchs       "011012"
#define    sop_fabs       "011013"
#define    sop_ftst       "011014"
#define    sop_fxam       "011015"
#define    sop_f2xm1      "011016"
#define    sop_fyl2x      "011017"
#define    sop_fptan      "011018"
#define    sop_fpatan     "011019"
#define    sop_fxtract    "01101a"
#define    sop_fprem1     "01101b"
#define    sop_fdecstp    "01101c"
#define    sop_fincstp    "01101d"
#define    sop_fxch       "01101e"
#define    sop_fld1       "01101f"
#define    sop_fldl2t     "011020"
#define    sop_fldl2e     "011021"
#define    sop_fldpi      "011022"
#define    sop_fldlg2     "011023"
#define    sop_fldln2     "011024"
#define    sop_fldz       "011025"
#define    sop_fprem      "011026"
#define    sop_fyl2xp1    "011027"
#define    sop_fsqrt      "011028"
#define    sop_fsincos    "011029"
#define    sop_frndint    "01102a"
#define    sop_fscale     "01102b"
#define    sop_fsin       "01102c"
#define    sop_fcos       "01102d"


//da
#define    sop_fiadd        "01102e"
#define    sop_fimul        "01102f"
#define    sop_ficom        "011030"
#define    sop_ficomp       "011031"
#define    sop_fisub        "011032"
#define    sop_fisubr       "011033"
#define    sop_fidiv        "011034"
#define    sop_fidivr       "011035"
#define    sop_fcmovb       "011036"
#define    sop_fcmovbe      "011037"
#define    sop_fcmove       "011038"
#define    sop_fcmovu       "011039"
#define    sop_fucompp      "01103a"


//db
#define    sop_fild        "01103b"
#define    sop_fisttp      "01103c"
#define    sop_fist        "01103d"
#define    sop_fistp       "01103e"
//#define    sop_fld         "01103f"
//#define    sop_fstp        "011040"
#define    sop_fcmovnb     "011041"
#define    sop_fcmovnbe    "011042"
#define    sop_fclex       "011043"
#define    sop_finit       "011044"
#define    sop_fcomi       "011045"
#define    sop_fcmovne     "011046"
#define    sop_fcmovnu     "011047"
#define    sop_fucomi      "011048"

//dc dd de df
#define    sop_frstor      "011049"
#define    sop_fsave       "01104a"
#define    sop_fstsw       "01104b"












