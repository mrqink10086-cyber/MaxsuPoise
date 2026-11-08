# 0.3.6 执行逻辑与缓存优化

日期：2026-11-08（Asia/Shanghai）。基准为已迁移到 NG 的 0.3.5。

## 修改范围

| 路径 | 修改与缓存寿命 |
| --- | --- |
| `src/PoiseHealthHandler.cpp` | 一次总韧性计算只获取一次装备快照，多个槽位复用。保持引擎容器遍历顺序、衣服占槽和皮肤回退语义，不持久保存角色装备指针。 |
| `src/GameSettings.h` | 缓存 15 个设置的绑定；每次使用仍读取当前数值。未找到的设置继续重试，DataLoaded 和菜单更新时重新绑定。 |
| `src/SettingsHandler.h/.cpp` | 武器倍率使用固定数组，护甲槽位使用连续 vector；以不可变配置快照原子发布，菜单更新排到 SKSE 主线程任务。 |
| `src/AnnotationCache.h` | 每线程按注释文本缓存解析结果，包括无效 JSON。每线程最多 512 条、1 MiB 键文本，超出总预算清空；超过 1 MiB 的单条文本在解析前拒绝。拥有键和值，不持有动画指针。 |
| `src/StaggerHandler.cpp` | 免疫计算按需执行，关键字从高到低提前返回，效果列表一次遍历；注释时间仍从当前动画获取。选中角色的诊断输出继续计算完整信息。 |
| `src/PoiseDamageCalculator.cpp` | 命中处理一次解析角色句柄并保持引用，格挡倍率只在格挡分支读取。 |
| `src/Hooks/PoiseRegenHandler.cpp` | 保持原逐帧算法，跳过数值完全不变的图变量写入；固定图变量名复用 BSFixedString。 |

没有增加后台角色调度线程，也没有降低更新时间频率。脱战和硬直期间的计时冻结、延迟跨零点后的下一帧恢复，以及原有恢复超量行为均保持。

未启用 AVX2、SIMD 批处理或 FMA。当前路径主要消耗在引擎调用、容器构造和 JSON 解析；几个标量浮点式不适合通过新增指令集要求来优化。浮点计算顺序和原有公式保持。

## 验证

测试编译实际的总韧性与恢复函数体，只替换引擎接口；注释解析器、缓存和设置绑定使用生产代码。结果见 [audit/performance-checks.txt](audit/performance-checks.txt)。

- 3000 组随机装备数据：与原实现逐位相同的浮点结果；装备快照从 12344 次降至 3000 次。
- 5000 组随机恢复状态：与原实现结果一致，跳过 468 次重复韧性写入。
- 设置：覆盖缺省值、延迟出现、实时修改、重新绑定和缓存命中不查表。
- 注释：覆盖无效 JSON、钳制、逐次 startTime、边界帧、条数/文本预算；超过 1 MiB 的文本重复输入也不会调用解析器。
- 100000 次相同注释：原解析 99.7156 ms，热缓存 2.5408 ms，约 39.2 倍。该数字是隔离微基准，不代表游戏帧率提升。
- 同一隔离测试使用 MSVC AddressSanitizer 通过，见 [audit/safety-checks-asan.txt](audit/safety-checks-asan.txt)。

```powershell
cmake -S tests -B build/performance-checks -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=E:/source/repos/valhallaCombat/build/vcpkg_installed/x64-windows-static-md
cmake --build build/performance-checks --config Release --parallel 6
ctest --test-dir build/performance-checks -C Release --verbose

cmake -S tests -B build/safety-checks-asan -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=E:/source/repos/valhallaCombat/build/vcpkg_installed/x64-windows-static-md "-DCMAKE_CXX_FLAGS=/fsanitize=address /EHsc /Zi" "-DCMAKE_EXE_LINKER_FLAGS=/INCREMENTAL:NO /DEBUG"
cmake --build build/safety-checks-asan --config Release --parallel 6
# 将当前 MSVC 工具链的 bin/Hostx64/x64 加入 PATH，以便找到 ASan 运行库。
ctest --test-dir build/safety-checks-asan -C Release --verbose
```

这些检查覆盖可脱离引擎的算法和内存访问，没有运行真实游戏战斗，也没有检测引擎内部的并发访问。

## 保留的配置行为

护甲槽位仍只从 `Data/SKSE/Plugins/MaxsuPoise_ArmorSlot.ini` 读取。原项目随包的 `MaxsuPoise.ini` 包含同名段，但旧实现不会读取它；本次保持原路径，避免优化同时改变护甲韧性数值。没有新增自动回退。

优化前源码备份为 `build/pre-optimization-source.zip`，最终差异为 `build/optimization.patch`。0.3.5 安装包保持原样。

独立安全审核后增加了单条注释 1 MiB 上限；更长的注释不再参与免疫计算，避免每帧持有动画锁重复解析。这是异常大输入的明确行为变化。
