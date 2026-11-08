# NG 移植及偏移核查

核查日期：2026-11-08（Asia/Shanghai）。

## CommonLibSSE-NG 版本

本地库：`E:/source/repos/CommonLibSSE-NG-ng`，版本 15.7.3。

通过 GitHub API 将 `ng` 分支最新提交 [07262cc2ae75777a422ed0c9621310fadef7c382](https://github.com/alandtse/CommonLibSSE-NG/commit/07262cc2ae75777a422ed0c9621310fadef7c382) 的完整文件树与本地逐项比对。上游提交时间为 `2026-11-07T23:51:09Z`。共 2813 个文件，无缺失、无内容变更；`scripts/build-all-presets.cmd` 仅有 CRLF / LF 差异。因此保留本地库，无需重新拉取，也没有修改库源码。

机器可读结果：[audit/commonlib-upstream.json](audit/commonlib-upstream.json)。

## 1.6.1170 二进制检查

以下地址均为 RVA，实际地址为模块基址加 RVA。虚函数使用主虚表 `VTABLE[0]`；`0xAD` 是槽位索引，其字节偏移为 `0x568`。

| Hook | Address Library ID 与偏移 | 位置 RVA | 实际目标 RVA / ID | 结果 |
| --- | --- | --- | --- | --- |
| Character::Update(float) | 虚表 207886，槽位 0xAD | 虚表 0x18A5558 | 0x667D40 / 37348 | 通过 |
| PlayerCharacter::Update(float) | 虚表 208040，槽位 0xAD | 虚表 0x18AB9C0 | 0x732660 / 40447 | 通过 |
| 武器命中调用 | 38627 + 0x4A8 | 0x6BAD18 | 0x6B7BC0 / 38586 | 通过 |
| 魔法硬直调用 | 34982 + 0x6F | 0x5D421F | 0x68D0D0 / 37710 | 通过 |
| 攻击者 Perk 调用 | 37710 + 0xA1 | 0x68D171 | 0x385F30 / 23526 | 通过 |
| 目标 Perk 调用 | 37710 + 0xB9 | 0x68D189 | 0x385F30 / 23526 | 通过 |

四处调用均验证了 `E8 rel32` 指令和解码后的目标地址。1170 原有 AE 偏移正确，无需改成其他数值。CommonLibSSE-NG 的 `Actor::Update(float)` 声明也对应槽位 `0xAD`。

Windows x64 调用约定核查：命中处理的 `RCX` 为 Actor 指针，`RDX` 为 HitData 指针；硬直函数的 `RCX` 为目标 Actor，`XMM1` 为 float，`R8` 为攻击者 Actor；Perk 分发的 `ECX` 为入口编号、`RDX` 为 Perk 所属 Actor、`R8` 为另一 Actor、`R9` 为输出 float 指针。原来 Perk Hook 的 `float&` 与目标使用相同寄存器传址，本次改为明确的 `float*`，移除原函数永远不会执行的调用，继续保持禁用重复 Perk 缩放的行为。

核查对象为本地已有的解包游戏二进制，SHA-256、Address Library 文件哈希、指令字节和调用上下文均记录在 [audit/1170.json](audit/1170.json)。这是静态二进制及 ABI 检查；游戏内战斗与存档加载尚需实机验证。

## 保留的 1.5.97 支持

| Hook | ID / 偏移 | 目标 RVA / ID |
| --- | --- | --- |
| Character::Update(float) | 虚表 261397，槽位 0xAD | 0x5D6FB0 / 36357 |
| PlayerCharacter::Update(float) | 虚表 261916，槽位 0xAD | 0x69E580 / 39375 |
| 武器命中调用 | 37673 + 0x3C0 | 0x626400 / 37633 |
| 魔法硬直调用 | 34188 + 0x6F | 0x5FA1B0 / 36700 |
| 攻击者 Perk 调用 | 36700 + 0x9A | 0x32ECE0 / 23073 |
| 目标 Perk 调用 | 36700 + 0xAE | 0x32ECE0 / 23073 |

以上六项也全部通过，结果见 [audit/1597.json](audit/1597.json)。

## 主要修改

- CMake 直接构建相邻的 CommonLibSSE-NG，明确声明依赖，固定 DKUtil，提供 NG、AE、SE 预设及安装规则。
- 更新插件版本到 0.3.5，按编译选项声明具体游戏兼容版本，加载时检查 SKSE 版本，使用 NG 的 SKSE 初始化与日志实现。
- 使用 `SKSE::Init` 一次预留四处调用 Hook 共 56 字节的 trampoline。当前 NG 的旧 `AllocTrampoline` 接口内部只初始化一次，原来每个 Hook 各申请 14 字节的方式无法累计分配，已同步迁移。
- 删除旧 `UsesAddressLibrary(true)` 用法与重复模块重置，取消调试构建等待调试器的阻塞。
- 通用 Perk 分发改用 `RE::BGSEntryPoint::HandleEntryPoint`。调用点 Hook 保留已验证的 SE / AE ID 与偏移。
- 原战斗公式、INI、行为注入和 dmenu 数据保持原有语义。

## 构建与产物验证

NG / Release 已通过完整编译和链接（MSVC 19.44.35229、Windows SDK 10.0.26100.0）。0.3.6 的构建日志为 `build/build-ng-optimized.log`，仅剩固定版本 DKUtil 内部忽略 `[[nodiscard]]` 返回值的 C4834 警告。

产物为 x64 `MaxsuPoise.dll`，包含 `SKSEPlugin_Load`、`SKSEPlugin_Query` 和 `SKSEPlugin_Version` 三个导出。当前优化产物的元数据为 0.3.6、兼容 1.5.97 / 1.6.1170。NG 同时支持两个游戏版本，元数据的全局 SKSE 下限为 2.0.20；加载入口会针对 1.6.1170 单独强制检查 SKSE 2.2.6。DLL 不动态依赖 CommonLibSSE、DKUtil、spdlog 或 fmt，也没有调试版 CRT 导入。

产物哈希及完整导入表见 [audit/plugin.json](audit/plugin.json)。本次编译验证的是默认 NG 预设；单版本 AE / SE 预设没有分别进行完整构建。

## 重现检查命令

```powershell
python -m pip install pefile capstone
python tools/verify_commonlib.py E:/source/repos/CommonLibSSE-NG-ng --output docs/audit/commonlib-upstream.json
python tools/audit_offsets.py E:/source/repos/ModernStaggerLock-master/build/audit/SkyrimSE-1170.exe.unpacked.exe E:/source/repos/CommonLibSSE-NG-ng/tests/REL/versionlib-1-6-1170-0.bin --output docs/audit/1170.json
python tools/audit_offsets.py E:/source/repos/ModernStaggerLock-master/build/audit/SkyrimSE-1597.exe.unpacked.exe E:/source/repos/CommonLibSSE-NG-ng/tests/REL/version-1-5-97-0.bin --output docs/audit/1597.json
```

游戏 EXE 仅作本地只读核查，不随产物分发。原始项目文件备份在 `build/original-source.zip`。
