#include <stdio.h>



//mmx
#define    sop_movd                "012001"
#define    sop_movq                "012002"

#define    sop_packsswb            "012003"
#define    sop_packssdw            "012004"
#define    sop_packuswb            "012005"
#define    sop_punpckhbw           "012006"
#define    sop_punpckhwd           "012007"
#define    sop_punpckhdq           "012008"
#define    sop_punpcklbw           "012009"
#define    sop_punpcklwd           "01200a"
#define    sop_punpckldq           "01200b"
#define    sop_paddb               "01200c"
#define    sop_paddw               "01200d"
#define    sop_paddd               "01200e"
#define    sop_paddsb              "01200f"
#define    sop_paddsw              "012010"
#define    sop_paddusb             "012011"
#define    sop_paddusw             "012012"
#define    sop_psubb               "012013"
#define    sop_psubw               "012014"
#define    sop_psubd               "012015"
#define    sop_psubsb              "012016"
#define    sop_psubsw              "012017"
#define    sop_psubusb             "012018"
#define    sop_psubusw             "012019"
#define    sop_pmulhw              "01201a"
#define    sop_pmullw              "01201b"
#define    sop_pmaddwd             "01201c"

#define    sop_pcmpeqb            "01201d"
#define    sop_pcmpeqw            "01201e"
#define    sop_pcmpeqd            "01201f"
#define    sop_pcmpgtb            "012020"
#define    sop_pcmpgtw            "012021"
#define    sop_pcmpgtd            "012022"

#define    sop_pand             "012023"
#define    sop_pandn            "012024"
#define    sop_por              "012025"
#define    sop_pxor             "012026"

#define    sop_psllw            "012027"
#define    sop_pslld            "012028"
#define    sop_psllq            "012029"
#define    sop_psrlw            "01202a"
#define    sop_psrld            "01202b"
#define    sop_psrlq            "01202c"
#define    sop_psraw            "01202d"
#define    sop_psrad            "01202e"

#define    sop_emms             "01202f"




//sse
#define    sop_movaps            "012101"
#define    sop_movups            "012102"
#define    sop_movhps            "012103"
#define    sop_movhlps           "012104"
#define    sop_movlps            "012105"
#define    sop_movlhps           "012106"
#define    sop_movmskps          "012107"
#define    sop_movss             "012108"

#define    sop_addps            "012109"
#define    sop_addss            "01210a"
#define    sop_subps            "01210b"
#define    sop_subss            "01210c"
#define    sop_mulps            "01210d"
#define    sop_divps            "01210e"
#define    sop_divss            "01210f"
#define    sop_rcpps            "012110"
#define    sop_rcpss            "012111"
#define    sop_sqrtps           "012112"
#define    sop_sqrtss           "012113"
#define    sop_rsqrtps          "012114"
#define    sop_rsqrtss          "012115"
#define    sop_maxps            "012116"
#define    sop_maxss            "012117"
#define    sop_minps            "012118"
#define    sop_minss            "012119"

#define    sop_cmpps            "01211a"
#define    sop_cmpss            "01211b"
#define    sop_comiss           "01211c"
#define    sop_ucomiss          "01211d"

#define    sop_andps            "01211e"
#define    sop_andnps           "01211f"
#define    sop_orps             "012120"
#define    sop_xorps            "012121"

#define    sop_shufps           "012122"
#define    sop_unpckhps         "012123"
#define    sop_unpcklps         "012124"

#define    sop_cvtpi2ps           "012125"
#define    sop_cvtsi2ss           "012126"
#define    sop_cvtps2pi           "012127"
#define    sop_cvttps2pi          "012128"
#define    sop_cvtss2si           "012129"
#define    sop_cvttss2si          "01212a"

#define    sop_ldmxcsr            "01212b"
#define    sop_stmxcsr            "01212c"

#define    sop_pavgb             "01212d"
#define    sop_pavgw             "01212e"
#define    sop_pextrw            "01212f"
#define    sop_pinsrw            "012130"
#define    sop_pmaxub            "012131"
#define    sop_pmaxsw            "012132"
#define    sop_pminub            "012133"
#define    sop_pminsw            "012134"
#define    sop_pmovmskb          "012135"
#define    sop_pmulhuw           "012136"
#define    sop_psadbw            "012137"
#define    sop_pshufw            "012138"

#define    sop_maskmovq          "012139"
#define    sop_movntq            "01213a"
#define    sop_movntps           "01213b"
#define    sop_prefetch          "01213c"
#define    sop_sfence            "01213d"



//sse2
#define    sop_movapd            "012201"
#define    sop_movupd            "012202"
#define    sop_movhpd            "012203"
#define    sop_movlpd            "012204"
#define    sop_movmskpd          "012205"
#define    sop_movsd             "012206"

#define    sop_addps            "012207"
#define    sop_addsd            "012208"
#define    sop_subpd            "012209"
#define    sop_subsd            "01220a"
#define    sop_mulpd            "01220b"
#define    sop_mulsd            "01220c"
#define    sop_divpd            "01220d"
#define    sop_divsd            "01220e"
#define    sop_sortpd           "01220f"
#define    sop_sortsd           "012210"
#define    sop_maxpd            "012211"
#define    sop_maxsd            "012212"
#define    sop_minpd            "012213"
#define    sop_minsd            "012214"

#define    sop_andpd            "012215"
#define    sop_andnpd           "012216"
#define    sop_orpd             "012217"
#define    sop_xorpd            "012218"

#define    sop_cmppd              "012219"
#define    sop_cmpsd              "01221a"
#define    sop_comisd             "01221b"
#define    sop_ucomisd            "01221c"

#define    sop_shufpd             "01221d"
#define    sop_unpckhpd           "01221e"
#define    sop_unpcklpd           "01221f"

#define    sop_cvtpd2pi            "012220"
#define    sop_cvttpd2pi           "012221"
#define    sop_cvtpi2pd            "012222"
#define    sop_cvtpd2dq            "012223"
#define    sop_cvttpd2dq           "012224"
#define    sop_cvtdq2pd            "012225"
#define    sop_cvtps2pd            "012226"
#define    sop_cvtpd2ps            "012227"
#define    sop_cvtss2sd            "012228"
#define    sop_cvtsd2ss            "012229"
#define    sop_cvtsd2si            "01222a"
#define    sop_cvttsd2si           "01222b"
#define    sop_cvtsi2sd            "01222c"

#define    sop_cvtsq2ps            "01222d"
#define    sop_cvtps2dq            "01222e"
#define    sop_cvttps2dq           "01222f"

#define    sop_movdqa             "012230"
#define    sop_movdqu             "012231"
#define    sop_movq2dq            "012232"
#define    sop_movdq2q            "012233"
#define    sop_pmuludq            "012234"
#define    sop_paddq              "012235"
#define    sop_psubq              "012236"
#define    sop_pshuflw            "012237"
#define    sop_pshufhw            "012238"
#define    sop_pshufd             "012239"
#define    sop_pslldq             "01223a"
#define    sop_psrldq             "01223b"
#define    sop_punpckhqdq         "01223c"
#define    sop_punpcklqdq         "01223d"

#define    sop_clfush            "01223e"
#define    sop_lfence            "01223f"
#define    sop_mfence            "012240"
#define    sop_pause             "012241"
#define    sop_maskmovdqu        "012242"
#define    sop_movntpd           "012243"
#define    sop_movntdq           "012244"
#define    sop_movnti            "012245"



//see3
#define    sop_fisttp            "012301"

#define    sop_lddqu             "012302"

#define    sop_addsubps          "012303"
#define    sop_addsubpd          "012304"

#define    sop_haddps            "012305"
#define    sop_hsubps            "012306"
#define    sop_haddpd            "012307"
#define    sop_hsubpd            "012308"

#define    sop_movshdup          "012309"
#define    sop_movsldup          "01230a"
#define    sop_movddup           "01230b"

#define    sop_monitor           "01230c"
#define    sop_mwait             "01230d"

#define    sop_phaddw            "01230e"
#define    sop_phaddsw           "01230f"
#define    sop_phaddd            "012310"
#define    sop_phsubw            "012311"
#define    sop_phsubsw           "012312"
#define    sop_phsubd            "012313"

#define    sop_pabsb             "012314"
#define    sop_pabsw             "012315"
#define    sop_pabsd             "012316"

#define    sop_pmaddubsw         "012317"

#define    sop_pmulhrsw          "012318"

#define    sop_pshufb            "012319"

#define    sop_pshufb            "01231a"

#define    sop_psignb            "01231b"

#define    sop_palignr           "01231c"



//see4
#define    sop_pmulld            "012401"
#define    sop_pmuldq            "012402"

#define    sop_dppd              "012403"
#define    sop_dpps              "012404"

#define    sop_movntdqa           "012405"

#define    sop_blendpd            "012406"
#define    sop_blendps            "012407"
#define    sop_blendvpd           "012408"
#define    sop_blendvps           "012409"
#define    sop_pblendvb           "01240a"
#define    sop_pblendw            "01240b"

#define    sop_pminuw            "01240c"
#define    sop_pminud            "01240d"
#define    sop_pminsb            "01240e"
#define    sop_pminsd            "01240f"
#define    sop_pmaxuw            "012410"
#define    sop_pmaxud            "012411"
#define    sop_pmaxsb            "012412"
#define    sop_pmaxsd            "012413"

#define    sop_roundps            "012414"
#define    sop_roundpd            "012415"
#define    sop_roundss            "012416"
#define    sop_roundsd            "012417"

#define    sop_extractps         "012418"
#define    sop_insertps          "012419"
#define    sop_pinsrb            "01241a"
#define    sop_pinsrd            "01241b"
#define    sop_pinsrq            "01241c"
#define    sop_pextrb            "01241d"
#define    sop_pextrw            "01241e"
#define    sop_pextrd            "01241f"
#define    sop_pextrq            "012420"

#define    sop_pmovsxbw            "012421"
#define    sop_pmovzxbw            "012422"
#define    sop_pmovsxbd            "012423"
#define    sop_pmovzxbd            "012424"
#define    sop_pmovsxwd            "012425"
#define    sop_pmovzxwd            "012426"
#define    sop_pmovsxbq            "012427"
#define    sop_pmovzxbq            "012428"
#define    sop_pmovsxwq            "012429"
#define    sop_pmovzxwq            "01242a"
#define    sop_pmovsxdq            "01242b"
#define    sop_pmovzxdq            "01242c"

#define    sop_mpsadbw             "01242d"

#define    sop_phminposuw          "01242e"

#define    sop_ptest               "01242f"

#define    sop_pcmpeqq             "012430"

#define    sop_packusdw            "012431"


//see4,2
#define    sop_pcmpestri             "012532"
#define    sop_pcmpestrm             "012533"
#define    sop_pcmpistri             "012534"
#define    sop_pcmpistrm             "012535"

#define    sop_pcmpgtq               "012536"

#define    sop_aesdec                "012537"
#define    sop_aesdeclast            "012538"
#define    sop_aesenc                "012539"
#define    sop_aesenclast            "01253a"
#define    sop_aesimc                "01253b"
#define    sop_aeskeygenassist       "01253c"
#define    sop_pclmulqdq             "01253d"



////////////////////////////////////////////////////////////////////////////////////////////////





//mmx
#define    op_movd                0x012001
#define    op_movq                0x012002

#define    op_packsswb            0x012003
#define    op_packssdw            0x012004
#define    op_packuswb            0x012005
#define    op_punpckhbw           0x012006
#define    op_punpckhwd           0x012007
#define    op_punpckhdq           0x012008
#define    op_punpcklbw           0x012009
#define    op_punpcklwd           0x01200a
#define    op_punpckldq           0x01200b
#define    op_paddb               0x01200c
#define    op_paddw               0x01200d
#define    op_paddd               0x01200e
#define    op_paddsb              0x01200f
#define    op_paddsw              0x012010
#define    op_paddusb             0x012011
#define    op_paddusw             0x012012
#define    op_psubb               0x012013
#define    op_psubw               0x012014
#define    op_psubd               0x012015
#define    op_psubsb              0x012016
#define    op_psubsw              0x012017
#define    op_psubusb             0x012018
#define    op_psubusw             0x012019
#define    op_pmulhw              0x01201a
#define    op_pmullw              0x01201b
#define    op_pmaddwd             0x01201c

#define    op_pcmpeqb            0x01201d
#define    op_pcmpeqw            0x01201e
#define    op_pcmpeqd            0x01201f
#define    op_pcmpgtb            0x012020
#define    op_pcmpgtw            0x012021
#define    op_pcmpgtd            0x012022

#define    op_pand             0x012023
#define    op_pandn            0x012024
#define    op_por              0x012025
#define    op_pxor             0x012026

#define    op_psllw            0x012027
#define    op_pslld            0x012028
#define    op_psllq            0x012029
#define    op_psrlw            0x01202a
#define    op_psrld            0x01202b
#define    op_psrlq            0x01202c
#define    op_psraw            0x01202d
#define    op_psrad            0x01202e

#define    op_emms             0x01202f




//sse
#define    op_movaps            0x012101
#define    op_movups            0x012102
#define    op_movhps            0x012103
#define    op_movhlps           0x012104
#define    op_movlps            0x012105
#define    op_movlhps           0x012106
#define    op_movmskps          0x012107
#define    op_movss             0x012108

#define    op_addps            0x012109
#define    op_addss            0x01210a
#define    op_subps            0x01210b
#define    op_subss            0x01210c
#define    op_mulps            0x01210d
#define    op_divps            0x01210e
#define    op_divss            0x01210f
#define    op_rcpps            0x012110
#define    op_rcpss            0x012111
#define    op_sqrtps           0x012112
#define    op_sqrtss           0x012113
#define    op_rsqrtps          0x012114
#define    op_rsqrtss          0x012115
#define    op_maxps            0x012116
#define    op_maxss            0x012117
#define    op_minps            0x012118
#define    op_minss            0x012119

#define    op_cmpps            0x01211a
#define    op_cmpss            0x01211b
#define    op_comiss           0x01211c
#define    op_ucomiss          0x01211d

#define    op_andps            0x01211e
#define    op_andnps           0x01211f
#define    op_orps             0x012120
#define    op_xorps            0x012121

#define    op_shufps           0x012122
#define    op_unpckhps         0x012123
#define    op_unpcklps         0x012124

#define    op_cvtpi2ps           0x012125
#define    op_cvtsi2ss           0x012126
#define    op_cvtps2pi           0x012127
#define    op_cvttps2pi          0x012128
#define    op_cvtss2si           0x012129
#define    op_cvttss2si          0x01212a

#define    op_ldmxcsr            0x01212b
#define    op_stmxcsr            0x01212c

#define    op_pavgb             0x01212d
#define    op_pavgw             0x01212e
#define    op_pextrw            0x01212f
#define    op_pinsrw            0x012130
#define    op_pmaxub            0x012131
#define    op_pmaxsw            0x012132
#define    op_pminub            0x012133
#define    op_pminsw            0x012134
#define    op_pmovmskb          0x012135
#define    op_pmulhuw           0x012136
#define    op_psadbw            0x012137
#define    op_pshufw            0x012138

#define    op_maskmovq          0x012139
#define    op_movntq            0x01213a
#define    op_movntps           0x01213b
#define    op_prefetch          0x01213c
#define    op_sfence            0x01213d



//sse2
#define    op_movapd            0x012201
#define    op_movupd            0x012202
#define    op_movhpd            0x012203
#define    op_movlpd            0x012204
#define    op_movmskpd          0x012205
#define    op_movsd             0x012206

#define    op_addps            0x012207
#define    op_addsd            0x012208
#define    op_subpd            0x012209
#define    op_subsd            0x01220a
#define    op_mulpd            0x01220b
#define    op_mulsd            0x01220c
#define    op_divpd            0x01220d
#define    op_divsd            0x01220e
#define    op_sortpd           0x01220f
#define    op_sortsd           0x012210
#define    op_maxpd            0x012211
#define    op_maxsd            0x012212
#define    op_minpd            0x012213
#define    op_minsd            0x012214

#define    op_andpd            0x012215
#define    op_andnpd           0x012216
#define    op_orpd             0x012217
#define    op_xorpd            0x012218

#define    op_cmppd              0x012219
#define    op_cmpsd              0x01221a
#define    op_comisd             0x01221b
#define    op_ucomisd            0x01221c

#define    op_shufpd             0x01221d
#define    op_unpckhpd           0x01221e
#define    op_unpcklpd           0x01221f

#define    op_cvtpd2pi            0x012220
#define    op_cvttpd2pi           0x012221
#define    op_cvtpi2pd            0x012222
#define    op_cvtpd2dq            0x012223
#define    op_cvttpd2dq           0x012224
#define    op_cvtdq2pd            0x012225
#define    op_cvtps2pd            0x012226
#define    op_cvtpd2ps            0x012227
#define    op_cvtss2sd            0x012228
#define    op_cvtsd2ss            0x012229
#define    op_cvtsd2si            0x01222a
#define    op_cvttsd2si           0x01222b
#define    op_cvtsi2sd            0x01222c

#define    op_cvtsq2ps            0x01222d
#define    op_cvtps2dq            0x01222e
#define    op_cvttps2dq           0x01222f

#define    op_movdqa             0x012230
#define    op_movdqu             0x012231
#define    op_movq2dq            0x012232
#define    op_movdq2q            0x012233
#define    op_pmuludq            0x012234
#define    op_paddq              0x012235
#define    op_psubq              0x012236
#define    op_pshuflw            0x012237
#define    op_pshufhw            0x012238
#define    op_pshufd             0x012239
#define    op_pslldq             0x01223a
#define    op_psrldq             0x01223b
#define    op_punpckhqdq         0x01223c
#define    op_punpcklqdq         0x01223d

#define    op_clfush            0x01223e
#define    op_lfence            0x01223f
#define    op_mfence            0x012240
#define    op_pause             0x012241
#define    op_maskmovdqu        0x012242
#define    op_movntpd           0x012243
#define    op_movntdq           0x012244
#define    op_movnti            0x012245



//see3
#define    op_fisttp            0x012301

#define    op_lddqu             0x012302

#define    op_addsubps          0x012303
#define    op_addsubpd          0x012304

#define    op_haddps            0x012305
#define    op_hsubps            0x012306
#define    op_haddpd            0x012307
#define    op_hsubpd            0x012308

#define    op_movshdup          0x012309
#define    op_movsldup          0x01230a
#define    op_movddup           0x01230b

#define    op_monitor           0x01230c
#define    op_mwait             0x01230d

#define    op_phaddw            0x01230e
#define    op_phaddsw           0x01230f
#define    op_phaddd            0x012310
#define    op_phsubw            0x012311
#define    op_phsubsw           0x012312
#define    op_phsubd            0x012313

#define    op_pabsb             0x012314
#define    op_pabsw             0x012315
#define    op_pabsd             0x012316

#define    op_pmaddubsw         0x012317

#define    op_pmulhrsw          0x012318

#define    op_pshufb            0x012319

#define    op_pshufb            0x01231a

#define    op_psignb            0x01231b

#define    op_palignr           0x01231c



//see4
#define    op_pmulld            0x012401
#define    op_pmuldq            0x012402

#define    op_dppd              0x012403
#define    op_dpps              0x012404

#define    op_movntdqa           0x012405

#define    op_blendpd            0x012406
#define    op_blendps            0x012407
#define    op_blendvpd           0x012408
#define    op_blendvps           0x012409
#define    op_pblendvb           0x01240a
#define    op_pblendw            0x01240b

#define    op_pminuw            0x01240c
#define    op_pminud            0x01240d
#define    op_pminsb            0x01240e
#define    op_pminsd            0x01240f
#define    op_pmaxuw            0x012410
#define    op_pmaxud            0x012411
#define    op_pmaxsb            0x012412
#define    op_pmaxsd            0x012413

#define    op_roundps            0x012414
#define    op_roundpd            0x012415
#define    op_roundss            0x012416
#define    op_roundsd            0x012417

#define    op_extractps         0x012418
#define    op_insertps          0x012419
#define    op_pinsrb            0x01241a
#define    op_pinsrd            0x01241b
#define    op_pinsrq            0x01241c
#define    op_pextrb            0x01241d
#define    op_pextrw            0x01241e
#define    op_pextrd            0x01241f
#define    op_pextrq            0x012420

#define    op_pmovsxbw            0x012421
#define    op_pmovzxbw            0x012422
#define    op_pmovsxbd            0x012423
#define    op_pmovzxbd            0x012424
#define    op_pmovsxwd            0x012425
#define    op_pmovzxwd            0x012426
#define    op_pmovsxbq            0x012427
#define    op_pmovzxbq            0x012428
#define    op_pmovsxwq            0x012429
#define    op_pmovzxwq            0x01242a
#define    op_pmovsxdq            0x01242b
#define    op_pmovzxdq            0x01242c

#define    op_mpsadbw             0x01242d

#define    op_phminposuw          0x01242e

#define    op_ptest               0x01242f

#define    op_pcmpeqq             0x012430

#define    op_packusdw            0x012431


//see4,2
#define    op_pcmpestri             0x012532
#define    op_pcmpestrm             0x012533
#define    op_pcmpistri             0x012534
#define    op_pcmpistrm             0x012535

#define    op_pcmpgtq               0x012536

#define    op_aesdec                0x012537
#define    op_aesdeclast            0x012538
#define    op_aesenc                0x012539
#define    op_aesenclast            0x01253a
#define    op_aesimc                0x01253b
#define    op_aeskeygenassist       0x01253c
#define    op_pclmulqdq             0x01253d






































