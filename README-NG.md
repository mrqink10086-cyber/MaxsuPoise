# MaxsuPoise — CommonLibSSE-NG 移植版 0.3.6

本目录已从旧 CommonLibSSE 构建方式迁移至 CommonLibSSE-NG。保留原版韧性计算、行为注入数据和 dmenu 设置。

0.3.6 减少装备扫描、重复 JSON 解析及游戏设置查找，并修复配置并发读写与部分空指针访问。保持原有逐帧恢复、延迟和保护时间语义，不新增 AVX2 / FMA 要求。优化与测试结果见 [docs/OPTIMIZATION.md](docs/OPTIMIZATION.md)，安全审计见 [docs/SAFETY-AUDIT.md](docs/SAFETY-AUDIT.md)。

## 运行环境

| Skyrim 版本 | 最低 SKSE 版本 | 构建预设 |
| --- | --- | --- |
| 1.6.1170 | 2.2.6 | NG / AE |
| 1.5.97 | 2.0.20 | NG / SE |

默认 NG 产物同时支持上表两个版本。其他游戏版本及 VR 不在本次支持范围内，加载入口会拒绝不支持的版本。

需要安装对应游戏版本的 **Address Library for SKSE Plugins** 和 **Behavior Data Injector**。原有代码要求 `BehaviorDataInjector.dll` 已加载，否则不会安装韧性 Hook。TrueHUD 是可选 HUD 集成。第三方依赖 DLL 不包含在本项目安装包中。

将安装包内容放入游戏 `Data` 目录，或直接用 MO2 / Vortex 安装 ZIP。DLL 路径为 `Data/SKSE/Plugins/MaxsuPoise.dll`，INI、`BehaviorDataInjector` 和 `dmenu` 数据一并保留。不要同时加载其他版本的 MaxsuPoise DLL。

日志位于游戏的 SKSE 日志目录中，文件名为 `MaxsuPoise.log`。

## 构建

需要 Visual Studio 2022 的 C++ 工具链、Windows SDK、CMake 3.24+ 和 vcpkg。

```powershell
$env:VCPKG_ROOT = "C:/path/to/vcpkg"
cmake --preset NG
cmake --build --preset NG --parallel 6
cmake --install build/ng --config Release --component Plugin --prefix dist/MaxsuPoise-NG-0.3.6
```

`COMMONLIBSSE_NG_PATH` 默认是相邻目录 `../CommonLibSSE-NG-ng`，可在配置时覆盖。DKUtil 已随源码放入 `external/DKUtil`，版本固定为提交 `19fcb2d2ee00a9b678bf08b24c53208ab7618fc6`。

原来的 `!Update.ps1` 不再参与构建。安装使用 CMake 的 `install` 规则，不依赖个人游戏目录或自动部署环境变量。

本机 `VCPKG_ROOT` 指向不存在的 `C:/vcpkg`，因此实际验证使用现成的依赖缓存：

```powershell
cmake --preset NG -DCMAKE_TOOLCHAIN_FILE= -DCMAKE_PREFIX_PATH=E:/source/repos/valhallaCombat/build/vcpkg_installed/x64-windows-static-md -DFETCHCONTENT_SOURCE_DIR_HDE64=E:/source/repos/valhallaCombat/build/_deps/hde64-src -DCOMMONLIB_ENABLE_IPO=OFF
cmake --build --preset NG --parallel 6
```

完整偏移证据及复查方法见 [docs/NG-MIGRATION.md](docs/NG-MIGRATION.md)。
