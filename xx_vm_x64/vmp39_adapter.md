# VMP 3.9.4 适配：vmp39_restore 模块

## 概述

基于 `g0th1c54e4/vmp394-handlers` 仓库（VMP 3.9.4 加壳样本 + Triton taint 分析），在 xx_vm_x64 插件中新增 **VMP394-Restore** 菜单，实现 VM 字节码静态反汇编（虚拟指令流 + VR 槽位标注）。

## 文件

| 文件 | 作用 |
|---|---|
| `vmp39_restore.c` | 解码主逻辑：handler 字典查找 + delta 表匹配 + VR 标注 + 文件回退读取 |
| `vmp39_restore.h` | 模块接口 |
| `vmp39_dict_data.c` | 自动生成的字典数据：86 个 handler + 312 条 delta（来自样本 trace） |
| `xx_vm64.cpp` | 菜单挂载（MENU_VMP394_RESTORE / menu_vmp394_restore 命令） |

## VMP 3.9.4 字节码格式（已实证）

```
每条虚拟指令 = [操作数(0-2B)] + [4B 加密 delta]，长度由 handler 决定（4/5/6/8/9/12/13B）
handler 分派 = 相对 delta（vbase += delta），delta 从加密 dword 经 per-handler 变换解出
vkey(r9) 滚动密钥；VR 槽位 = VM 入口 rsp + n*8
```

- **handler 词汇**：vPopReg64/vPushImm64/vPushReg64/vNand32/vNor32/vAdd64/vShr64/vJmp/vExit 等（NAND/NOR 万能门组合）
- **VR 映射**：VR0=rflags, VR4=r8, VR14=rbx 等（每样本不同）
- **vpc 递减方向**：字节码从高地址向低地址执行

## 使用方法

1. 编译 xx_vm_x64 插件（Release），部署 `xx_vm_x64.dp64` + `xx_comm64.dll` + `xxdisasm64.dll` 到 x64dbg 插件目录
2. x64dbg 加载 3.9.4 加壳样本，停在 VM 入口（此时 rdi=vpc, r9=vkey, rbx=vbase）
3. 菜单 **VMP394-Restore**（或命令 `menu_vmp394_restore`）→ 输出虚拟指令流

## 已验证结果（branch.vmp.exe）

```
[0] vPopReg64 VR4; vPopReg64 VR14  len=6 ops=007b  -> 1402257c3
[1] vPopReg64 VR4; vPopReg64 VR14  len=6 ops=2837  -> 1402257c3
...（连续 8 条与 handlers.txt 完全一致）
[8] vPopReg64 VR22; vPushImm64 0x140001188  len=13 ops=ea388690b9ed1945fe -> 140079a64
```

## 已知限制

1. **vkey 静态模拟漂移**：VMP 3.9 每字节解密依赖精确 CF 状态，静态 op_decode 链在第 N 条后漂移导致 delta 失配。当前用"高 24 位匹配 + unique-next fallback"缓解，可解码前缀；完整解码需动态读取真实 r9 或符号执行。
2. **delta 表样本特定**：字典数据从 branch.vmp.exe trace 提取，换样本需重新生成（`vmp394-handlers/cmp&jne/gen_c_dict.py`）。
3. handler 语义为"指令组合"（如 `vPopReg64;vPopReg64`），VR 编号解码依赖 op_decode 近似。

## 数据生成流程

```bash
# 从仓库样本提取字典（依赖 success.taint.txt + handlers.txt + branch.vmp.exe）
cd vmp394-handlers/cmp&jne && python gen_c_dict.py
cp vmp39_dict_data.c <插件目录>/vmp39_dict_data.c
```

## 3.8 适配状态

**VMP 3.8.1 handler 特征**（来源：看雪 thread-280283 黑蓝）+ 本机调研：
- **vm_start = pushfd**，**vm_exit = popfd**
- **push**：`mov [vsp±imm32(混淆)],reg32`；push imm32 两次、vsp-8——**监测 vsp 变化认定**（imm32 被 VMP 混淆为 imm32±imm32，不能靠静态读 imm）
- **pop**：`mov reg,mem`；pop imm32 两次、vsp+8——监测 vsp 变化判断
- **(add)sub**：`push reg1; push reg2; (sub)not reg1; mov reg,mem; mov reg,mem; add reg,reg; (sub)not reg`
- 核心结论：**3.8 handler 识别必须动态执行 + 监测 vsp 变化**，静态读指令/imm 不可靠

**已实现（vmp38_restore.cpp + 菜单 VMP38-Restore）**：
- **动态执行对接完成**：vmp38_restore.cpp 复用插件 xx_execute64 执行引擎（step_one：DbgMemRead 读指令 → xx_disasm → xx_execute → 对比 vsp），取混淆 imm32 真值
- xx_execute64.c 对接要点：xx_link/xx_execute 用 extern "C" 引用（xx_execute64.c 是 .c 编译）；xx_link 需初始化（vmp38 内调 xx_list_init）；xx_vm.h 的 debug_buf 改 extern（在 xx_execute64.c 定义）
- vsp 追踪：push → vsp-8、pop → vsp+8、handler 边界按 jmp/ret 切分（kanxue 280283 方法）
- 菜单 VMP38-Restore（MENU_VMP38_RESTORE=17）：GUI x64dbg 下用户停在 VM 入口触发
- **headless 限制**：headless 的插件菜单命令触发不稳定（vmp394 可触发、vmp38 不触发），3.8 运行验证需 GUI
- VM 入口调用点 `0x1400011b1 call 0x140243832`；入口跳板 → `0x140240dd6` → `0x14007c6f0` 初始化 → 首个 handler `0x1400b8a61`
- handler 元数据表：`.38vm0+0x404` 起 6 字节/条 `[u16 len][u32 vm_off]`（68511 条，加密态）
- 字节码区：`.38vm0` 段开头 0x0-0x600（加密）

**已增强（handler 语义识别）**：
- **独立迷你模拟器**（不依赖 xx_execute，避免与 3.5 引擎全局状态冲突）：DbgDisasmAt 反汇编 + 手工 vsp 追踪
- **handler 语义分类**：push（vsp-8，输出压入操作数）/ pop（vsp+8）/ alu（add/sub/xor/and/or/not/neg/imul 等）/ mixed / mov-other
- **3.8 push/pop-like 识别**（kanxue 核心）：`mov [vsp±imm],reg` → push-like（写 vsp 内存），`mov reg,[vsp±imm]` → pop-like（读 vsp 内存）；movsx/movzx 已排除误判（只认精确 mov）
- **add/sub 精确识别（kanxue 模式，宽松版）**：`match_kanxue_addsub`——push 计数 + add/sub + not 组合判定（容忍 3.8 深度混淆的垃圾指令：rol/xor/neg/imul/movsx 等），返回 ADD/SUB
- **实测（3.8 样本 VM 入口 0x7ff7d5253832）**：handler[4] ADD（push=7 add=2 not=1）、handler[6] ADD（push=3 add=1 not=1）识别成功；137 步 7 handler
- vmp39 基线不受影响（回归 9 条正常）
- 菜单 VMP38-Restore（MENU_VMP38_RESTORE=17）：GUI 下用户停在 VM 入口触发

**已增强（mini-exec 指令覆盖扩展）**：
- **指令覆盖**：mov（imm/reg/内存写）/add/sub/xor/and/or/**inc/dec/adc/sbb/imul/mul/rol/ror/shl/shr/sar**/not/neg/lea（reg*scale±imm）
- **宽度感知**（关键）：mov r32/r16/r8 **零扩展**（清高位）；add/sub/xor/rol/ror/shl/shr/sar **只改对应宽度、保留其他位**（如 `sar r8b,0xE5` 只动低 8 位）；`ror r8w,0xED` 只动低 16 位
- **真值提取**：`push reg` → 寄存器值；`mov [vsp±imm],reg` → 源寄存器值；`mov [vsp±imm],0x混淆imm` → 直接解码
- **实测（3.8 样本 VM 入口）**：混淆常量 `0xFFFFFFFFE9878892` 正确提取（=-0x186E776E）；r8 跨指令追踪（lea→add r8w→sar r8b→ror r8w→inc→push）值连续一致（0x15966e813 等）
- vmp39 基线不受影响（回归 9 条正常）

**已增强（mini-exec 内存读 + movzx/movsx）**：
- **迷你内存模型**：vsp 相对偏移槽位表（MINI_MEM，256 槽），push-like mov 写表、pop-like mov 读表
- **绝对内存读**：`mov reg,[rbx±imm]` 等用 DbgMemRead 读真实进程内存（样本已加载）
- **movzx/movsx**：零扩展/符号扩展（movsx 按源宽度 sign-extend）
- **修复的 bug**：
  - op_width 补 bl/cl/sil/dil/bpl/spl 等 8 位寄存器
  - reg_index_from_name 用 strcmp（原 strncmp 误匹配）
  - **mov 分支加 `!strchr(op1,'[')` 保护**（内存目的不当 reg 写）
  - **宽度检测 qword 优先**：`mov qword ptr [..],reg` 的 op1 含 "qword"（含子串 "word"），原逻辑误判 w=16 截断真值——改为 qword 优先判断
- **实测（3.8 样本 VM 入口）**：push real 值全部完整无截断——rbp=0x31358404、r8=0x159667d56、混淆常量 0xffffffffe9878892
- vmp39 基线不受影响（回归 9 条正常）

**已增强（xadd/xchg/cmpxchg + 跨槽位别名）**：
- **xadd [mem],reg**：交换并加（旧内存值→reg，mem+=reg），宽度感知
- **xchg [mem],reg**：交换（内存↔寄存器）
- **cmpxchg [mem],reg**：近似（假设比较失败：reg=mem）
- **跨槽位别名读取**：mini_mem_read 支持 containment（大槽位内读子字段，如 [rsp+0xA] 从 [rsp+0x8] 槽提取）+ partial overlap（取最高覆盖槽）
- **实测（3.8 样本 VM 入口）**：`xadd word ptr [rsp+r8-0xC68C7F7], r11w` 分支命中；真值全部完整
- vmp39 基线不受影响（回归 9 条正常）

**已增强（内存地址计算 compute_abs_addr）**：
- **compute_abs_addr**：通用绝对地址计算 `[base + index*scale ± disp]`，支持任意 base/index 寄存器 + 纯 disp + reg*scale
- abs_mem 读、movzx/movsx 内存分支统一改用 compute_abs_addr（删除重复内联解析）
- **覆盖形式**：`[rax+r10-0x176]`、`[rsp+rax*4-0x5D7]`、`[rsp+rax-0x6A]`、`[rbx]`、`[rbx+rbp-const]` 等
- **实测（3.8 样本 VM 入口）**：复杂索引内存读（xadd [rsp+rax*4-0x5D7],rsi、movzx ebx,[rsp+rax-0x6A]、mov edx,[rbx]）正确解析；真值完整
- vmp39 基线不受影响（回归 9 条正常）

**已增强（VM 入口自动定位 vmp38_auto_locate）**：
- **自动定位**：遍历主模块段找最大非 .text 段（VM 段），扫描 .text 找 call/jmp 到 VM 段的指令 → 返回 VM 入口
- **菜单集成**：VMP38-Restore 触发时先 auto_locate，找到则自动跳转 VM 入口执行；找不到回退当前 EIP
- **实测（3.8 样本，headless 无手动设 rip）**：自动找到 `call @ 0x7ff7d50111b1 -> VM 0x7ff7d5253832`，识别 7 handler（2 ADD）
- **用户无需手动停 VM 入口**；headless 也可用（无需设 rip）
- vmp39 基线不受影响（回归 9 条正常）

**已增强（ValueCommand::Calc 解密链实现）**：
- **vmp38_value_decrypt/encrypt**（vmp38_restore.cpp，对照 3.5.1 源码 processors.cc）：
  - 命令集：add/sub/inc/dec/xor/not/neg/bswap/rol/ror + 宽度（8/16/32/64）
  - **Encrypt**：命令链正序（item(0)→item(n-1)）；**Decrypt**：逆序 + 互补互换（add↔sub、inc↔dec、rol↔ror）
  - vmp38_try_decrypt_from_steps：从 STEP_REC 序列提取 ALU 命令链并代数解密
  - parse_imm_from_disasm：从 disasm 提取立即数
- **加密常量标注**：push-like mov 直写 imm 时输出 `const[enc] ... = 0x... (VMP-encrypted imm, N bits)`
- **端到端验证（Python 模拟）**：真实常量 0x01000193 → VMP 加密链（not/inc/add/rol/not/ror）→ 0x28961ab5 → 解密还原 0x01000193 ✓；round-trip 100/100
- **实测（3.8 样本）**：`const[enc] mov [rsp+rdx*8-0x308], 0xFFFFFFFFE9878892` 标注
- vmp39 基线不受影响（回归 9 条正常）

**已增强（命令链自动提取 + 自动解密）**：
- **extract_calc_chain**：从 STEP_REC 序列自动提取 ValueCommand 命令链（add/sub/xor/not/neg/rol/ror/bswap/inc/dec，遇非 ALU 停止）
- **try_decrypt_enc_imm**：对加密 imm 应用 vmp38_value_decrypt 代数还原
- **identify_handler 集成**：扫描 handler 内 `mov [mem],imm` 直写，对大值/负值 imm（>0x10000000 或 >0x7fffffff）自动解密
- **实测（3.8 样本）**：`enc=0xffffffffe9878892 -> real=0xffffffffe987898e`（加密常量自动解密）；小偏移（0x8d/0x61 等）已被过滤不误报
- **已知限制**：命令链含 handler 内无关 ALU（未追踪加密值流向寄存器），解密结果指示方向但非精确（mini-exec 动态追踪仍是最精确真值来源）
- vmp39 基线不受影响（回归 9 条正常）

**已增强（精确命令链提取）**：
- **extract_calc_chain_precise**：追踪加密值载体寄存器（carrier），只收集作用于载体的 ALU（uses_carrier 检查 + mov reg2,reg1 传播载体）
- **identify_handler 双路径解密**：
  1. `mov reg, imm(enc)` → carrier=reg，精确链
  2. `mov [mem], imm(enc)` → carrier=-1，宽松链
- **候选过滤**：32 位随机值（>0x40000000）或 64 位负值
- **实测（3.8 样本）**：`enc=0xacb33eab -> real=0xffffffff534cc153`（mov r11d 载体精确链）；`enc=0xdf3dd1bc -> real=0xdf3dd1bd`
- **结论**：VMP 3.8 常量重建是**跨寄存器分散**的（r11→r8→eax...），单载体追踪仍含噪音；**mini-exec 动态追踪仍是真值最可靠来源**（0x159667d56 等精确），代数解密为辅助交叉验证
- vmp39 基线不受影响（回归 9 条正常）

**已增强（多载体追踪）**：
- **extract_calc_chain_precise 多载体版**：维护载体集合（MAX_CARRIERS=8），`mov reg2,reg1` 传播 + ALU 结果流入 dst + lea 源引用都扩展载体集
- **lea 建模**：lea dst,[rs*scale+disp] 解析 scale，disp 作为 ADD 命令（scale 乘法无法用单 VMP 命令表达）
- **实测（3.8 样本）**：`enc=0xacb33eab -> real=0xd5d51fffb33cfe04`（多载体跨寄存器链）
- **结论（重要）**：VMP 3.8 的 0xACB33EAB 等是**加密中间值**（参与 lea r8=3*r11-const 等多步重建），纯代数解密该中间值无意义；**最终常量靠 mini-exec 动态追踪**（如 r8 最终 0x159667d56）。多载体追踪扩大了链覆盖但 lea 乘法+跨寄存器噪音使纯代数不精确——再次印证"VMP 3.8 魔数需动态/符号执行还原"
- vmp39 基线不受影响（回归 9 条正常）

**已增强（RIP 相对寻址 + 段前缀）**：
- **compute_abs_addr_ex**：增加 ip + instr_len 参数，支持 `[rip+imm]`（addr = ip + instr_len + disp）
- **STEP_REC 加 instr_len**：step_one 填充，mini_exec 内存读用它
- abs_mem 读、movzx/movsx 内存分支统一用 compute_abs_addr_ex（带 rip）
- **段前缀**：fs:/gs: 识别 stub（headless 下 fs 段基址难取，多数 VMP 用 rip 相对）
- **实测**：3.8 样本 VM handler 区 0x2000 采样无 RIP 指令（VMP 3.8 用绝对/寄存器寻址）——RIP 支持是完整性补全，其他样本可能需要
- vmp39 基线不受影响（回归 9 条正常）

**已增强（段前缀完整解析 + lea scale 精确建模）**：
- **段前缀**：fs:/gs: 用 `Script::Misc::ParseExpression("teb()")` 获取 TEB 基址 + 偏移（需 include _scriptapi_misc.h）
- **lea scale 精确建模**：重写 lea 分支，区分 base-reg 与独立 index*scale：
  - `lea r8,[r11+r11*2-const]` = 3*r11-const ✓
  - `lea r8,[r11*2+disp]`（无 base）精确处理 ✓
- **数学验证**：`lea rbp,[rbx+rbp-0x313583FC]` = 0x31358404+rbp0-0x313583FC（rbp0=0 headless）=0x8 ✓；`lea r8` = 3*0xacb33eab-0x4CC2FDD4 = 0x1b956be2d ✓
- **精度说明**：lea 建模数学精确，但结果依赖**初始寄存器完整**（headless 测试未设 rbp/r11 等导致偏差）；GUI 下用户停 VM 入口时寄存器真实，建模精确
- vmp39 基线不受影响（回归 9 条正常）

**已增强（VM 入口寄存器自动采集 vmp38_capture_vm_regs）**：
- **capture_vm_regs**：设临时断点 VM 入口 → Script::Debug::Run/Wait → 断点命中采集全部寄存器（Script::Register::Get，RegisterEnum 成员）→ 删断点
- **快路径**：当前 ip 已在 VM 入口时直接采集不运行
- **菜单**：当前用直接 restore（headless 兼容）；capture 面向 GUI（用户点菜单自动跑到 VM 入口采集真实寄存器——解决 headless 初始寄存器不全导致 lea 偏差）
- **headless 限制**：Script::Debug::Run/Wait 在菜单回调内卡死（headless 脚本线程不恢复）——菜单默认不 capture
- **踩坑**：RegisterEnum 成员是 RAX/RBX/RIP 等（非 REG_* 前缀）；批量 python 编辑 xx_vm64.cpp 易弄乱 case 的 brace，需逐行检查
- vmp39 基线不受影响（回归 9 条正常）

**已验证（capture_vm_regs + lea 精确）**：
- **快路径验证成功**：headless 设 EIP 到 VM 入口 + 设 rbp=0x31358404/r11=0xACB33EAB，菜单触发 capture 快路径采集：
  `vmp38_capture(fast): ip=7ff7d5253832 rsp=7ff7d50114b8 rbp=31358404 r11=acb33eab` ✓
- **lea 精确性证明**：lea rbp=[rbx+rbp-0x313583FC] = 0x31358404+0x31358404-0x313583FC = **0x3135840c**（capture 真实 rbp 传入后 push real=0x3135840c，之前 headless 默认 rbp=0 时是 0x8）
- **GUI 效果**：用户点 VMP38-Restore → capture 自动跑到 VM 入口采集真实寄存器 → lea 精确 → 常量还原准确
- **菜单最终**：capture 优先 + fallback 当前寄存器（headless 卡死走 fallback 不阻塞）
- vmp39 基线不受影响（回归 9 条正常）

**32 位适配（xx_vm_x32）**：
- **vmp38_restore.cpp 移植到 xx_vm_x32**：VMP32_BUILD 条件编译（VMP_CTX r[8] 32 位、VMP_VSP_STEP=4、栈寄存器 esp/esi/edi、间接 jmp 用 mreg 寄存器值）
- **菜单 VMP38-Restore** 已挂到 xx_vm32.cpp
- **xx_comm32.dll/xxdisasm32.dll 编译**（vcproj→vcxproj 生成，Release）
- **部署**：D:/x64dbg/x32/plugins/xx_vm_x32.dp32 + xx_comm32.dll + xxdisasm32.dll
- **32 位样本**：F:\xxvm\vmp38_x32\Project1_38.vmp.exe（3.8.4 加壳 x86-32，.38vm0/.38vm1/.38vm2 三段）
- **验证**：auto_locate 找到 VM 入口（0x891022 call -> 0xBD9D31）；capture 快路径采集 esp/ebp/ebx；handler[0] 识别
- **headless 限制**：.38vm2 段内存不可读（headless 未映射大段）-> handler 还原停住；GUI 下应正常（完整映射）
- **32 vs 64 差异**：寄存器 16->8、vsp 步长 8->4、分派寄存器多态（eax/ecx/edx）、无 handler 元数据表、VM 入口直接 call（无跳板）、常量加密形态相同
