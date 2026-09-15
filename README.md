# ESP32-C6 Touch AMOLED 2.16 游戏大厅

[项目介绍：掌上游戏乐园](PROJECT_INTRO.md) · [预编译固件与烧录](firmware/FLASH.md)

这是基于 ESP-IDF 5.5.3 和 LVGL 9.5.0 的游戏大厅，目标板为微雪
ESP32-C6 Touch AMOLED 2.16（480 × 480、CST9220 触摸、CO5300 QSPI AMOLED）。
沿用微雪官方示例的 `esp_lcd_touch_cst9217` 和 `esp_lcd_sh8601` 驱动组件及该板初始化序列；组件名称与实际芯片名称不同。

当前版本包含中文大厅和九款可玩的游戏：2048、贪吃蛇、灯泡全灭、叠叠高、打砖块、记忆翻牌、扫雷、反应挑战，以及第三页的重力滚球。

- 大厅显示 9 个可扩展的游戏条目，四个条目一页，支持 `上一页` / `下一页` 翻页。
- 九款游戏均显示 `可游玩`。
- 点击卡片进入详情页，再点击 `开始游戏`。
- 详情页的 `返回` 返回大厅。
- 首页右上角 `亮度` 打开亮度面板，滑块范围为 20%–100%。
- 当前使用全中文界面和 Noto Sans SC 中文子集字体（500 字重，16/20/24/28 px）。亮度仅在本次开机期间保留，重启恢复 78%。
- 2048 使用四向滑动操作（最小滑动距离 22 px），支持一键撤销一步、重新开始、达成 2048 后继续挑战和游戏结束后查看棋盘。
- 2048 的最高分、当前棋盘、一步撤销快照和随机状态会以带 magic/schema/CRC 的版本化 NVS blob 保存；移动后延迟 900 ms 保存，退出游戏时立即保存。NVS 初始化或写入失败时保留内存中的进度并在界面显示 `存档失败`，不会自动擦除 NVS 分区。
- `GameEntry` 注册表保留 `launch` 函数指针。实现下一款游戏后，将对应条目的 `nullptr`
  换成游戏入口函数，详情页会自动显示 `开始游戏` 并调用该入口。

## 构建和烧录

已编译固件位于 `firmware/`，直接烧录步骤见 `firmware/FLASH.md`。

打开本工程目录，在 ESP-IDF 5.5.3 环境中执行（`COMx` 换成开发板串口）：

```powershell
idf.py set-target esp32c6
idf.py build
idf.py -p COMx flash monitor
```

工程已经锁定直接依赖版本，首次构建时 ESP-IDF Component Manager 会根据
`main/idf_component.yml` 和 `dependencies.lock` 下载组件。若要使用仓库外的
IDF 工具链，可以把 `IDF_PATH` 指向 ESP-IDF 5.5.x，并从本目录运行命令。

本次工作区已在 `work/esp-idf` 和 `work/idf-tools` 准备好独立工具链，
可在工作区根目录执行 `./work/build.ps1` 重新构建。SDK 和缓存不包含在源码压缩包内。
USB 控制台使用 USB Serial/JTAG，默认日志速度 115200。

## 板卡配置

板级引脚集中在 [`components/port_bsp/user_config.h`](components/port_bsp/user_config.h)：

- QSPI CS = GPIO15，时钟 = GPIO0，数据线 = GPIO1/2/3/4
- I2C SCL/SDA = GPIO7/8
- 触摸 RESET = GPIO11，触摸 INT = GPIO5

显示和触摸初始化来自 Waveshare 官方 ESP-IDF 示例。`display_bsp` 的 DMA 传输上限
按官方示例设置为 50 行，LVGL adapter 使用 48 行缓冲，确保刷新区域不会超过 SPI
传输缓冲区。

## 添加游戏

在 `main/main.cpp` 的 `kGames` 数组追加条目即可复用卡片、翻页和详情页：

```cpp
static void launch_my_game(void)
{
    // 创建游戏屏幕或切换到你的游戏模块
}

// GameEntry 的最后一个字段填 launch_my_game，而不是 nullptr。
```

游戏入口运行在 LVGL 事件回调上下文中。若游戏包含较长初始化或持续循环，应该
创建 FreeRTOS 任务，并继续使用 `Lvgl_lock()` / `Lvgl_unlock()` 保护跨任务的 LVGL
访问。

## 2048 操作

在大厅点击 `2048` 卡片，进入详情页后点击 `开始游戏`。在棋盘上向上、下、左或右
滑动即可移动数字；相邻的相同数字会合并。顶部的 `撤销` 只撤销最近一次有效移动，
`重开` 会在确认后开始新局，`返回` 会保存进度并返回大厅。

达到 2048 后可以选择 `继续挑战`，继续合并更大的数字；没有可用移动时可以使用
一次撤销（如果本局已有有效移动）或重新开始。

## 贪吃蛇操作

在大厅点击 `贪吃蛇` → `开始游戏`。蛇身宽 22 px，棋盘为 16×16 格，每格 24 px。
首次有效滑动开始；在屏幕任意区域向上、下、左、右滑动转向。不能直接掉头，每个移动周期最多接受一次转向。
初速每格 300 ms，吃到一个食物加快 10 ms，最快每格 140 ms。蛇头为浅绿色，食物为橙色。
点击 `暂停` 后需点击 `继续` 恢复；`重开` 需要确认。碰到边界或身体结束，填满棋盘则通关。
滑过按钮不会触发点击。返回后重新进入会开始新局；最高分仅保留在本次开机期间，贪吃蛇没有断电存档。

## 灯泡全灭操作

点击一个灯泡会切换自身及上下左右灯泡。将 5×5 棋盘所有灯泡熄灭即可过关。
每个棋盘均通过合法操作打乱生成，保证有解；重置可以恢复本关初始棋盘，通关后进入下一关。

## 叠叠高操作

点击棋盘开始，再次点击让移动的黄色方块落下。只有与下方方块重叠的部分会保留，完全错开则结束。
误差不超过 3 px 时自动对齐；初始移动速度 80 px/s，每层增加 5 px/s，上限 210 px/s。
支持暂停/继续、确认重开；高塔会自动向上跟随显示。最高层数在本次开机期间保留。
两款新游戏都不保存断电进度，返回后重新进入开始新局。

## 第二页四款游戏

- **打砖块**：点击棋盘发球，左右拖动控制挡板；24 块砖、3 条生命，掉球后点击再次发球。支持暂停/继续和确认重开。
- **记忆翻牌**：4×4 卡片、8 对数字，点击两张配对。不匹配会显示 700 ms 再盖上，期间不能翻第三张；完成全部配对获胜。
- **扫雷**：6×6 棋盘、6 颗雷，第一次开格及相邻区域无雷。顶部按钮切换开格/插旗模式，旗子最多 6 面；打开所有安全格获胜。空白区域自动展开。
- **反应挑战**：点击大区域开始，等待变绿并显示“现在点击！”后立即按下。抢跑或超时不计入成绩；完成 5 次有效测试后显示平均反应时间。重开确认会取消正在计时的一次，避免暂停导致不公平成绩。

这四款游戏重新进入即为新局，不保存断电进度。
导航在事件回调结束后先释放旧画面，再创建新画面，避免游戏和大厅同时占用界面内存。

## 重力滚球（第三页）

利用板载 QMI8658 六轴传感器，以加速度计和陀螺仪融合得到倾斜方向。倾斜屏幕让黄色小球绕过墙壁，进入绿色终点，共三关。

首次进入按引导校准：

1. 将屏幕朝上平放，点击“开始校准”，保持静止约一秒。
2. 降低屏幕右边缘，稍作保持后点击“确认方向”。
3. 降低屏幕下边缘，稍作保持后再次确认。
4. 放回平放姿态，点击棋盘开始。

校准会记录静止角速度偏置、重力参考和屏幕方向；不需要猜测芯片的安装方向。小角度死区抑制漂移。
顶部“暂停”可继续或重来，“校准”可重新设置方向；数据中断会自动暂停，恢复后点击重试。
采样使用共享 I2C 总线上的独立任务；游戏退出后关闭加速度和陀螺仪测量，任务保留等待下一次进入。
此游戏不保存断电进度或校准值。每次进入需重新校准，下一关不需要重新校准。

## 验证范围

已运行真实 LVGL 9.5.0 主机渲染和模拟指针输入。两页大厅、详情、亮度弹窗布局已检查；
100 轮导航/弹窗测试在缓存预热后可用堆内存未持续下降。具体结果见 `ACCEPTANCE.md`。
已通过 COM5 烧录中文版并读取实机启动日志；用户确认中文显示和触摸正常。
2048 的主机引擎和 UI 测试、固件构建及上板操作记录见 `ACCEPTANCE.md`。

## 复现主机验收

先通过 `idf.py reconfigure` 下载依赖，然后使用本机 CMake 与 C/C++ 编译器：

```powershell
cmake -S tests/host -B build-host -G "Visual Studio 17 2022" -A x64
cmake --build build-host --config Debug --parallel 8
cd build-host
./Debug/lobby_acceptance.exe
./Debug/engine2048.exe
./Debug/storage2048.exe
./Debug/storage2048.exe init-failure
./Debug/game2048_ui_test.exe
./Debug/snake_engine_test.exe
./Debug/snake_stress_test.exe
./Debug/snake_ui_test.exe
./Debug/lights_engine_test.exe
./Debug/lights_ui_test.exe
./Debug/stack_engine_test.exe
./Debug/stack_ui_test.exe
./Debug/breakout_engine_test.exe
./Debug/breakout_ui_test.exe
./Debug/memory_engine_test.exe
./Debug/memory_ui_test.exe
./Debug/mines_engine_test.exe
./Debug/mines_ui_test.exe
./Debug/reaction_engine_test.exe
./Debug/reaction_ui_test.exe
./Debug/tilt_engine_test.exe
./Debug/tilt_input_test.exe
./Debug/tilt_ui_test.exe
```

测试包含实际 UI 源码，只有板级 I/O 使用替身。会生成四张 PPM 截图、运行指针输入和
100 轮导航测试；参数 `0` 可只运行控件检查。也可以通过 `-DLVGL_SOURCE_DIR=...`
指定单独下载的 LVGL 9.5.0 源码目录。

## 更新中文字体

常规构建直接编译 `main/fonts` 内的 C 字体文件，不需要安装字体工具。
新增汉字后需重新生成子集，避免缺字：

```powershell
pip install fonttools
npm install lv_font_conv@1.5.3
python tools/generate_fonts.py NotoSansSC.ttf node_modules/lv_font_conv/lv_font_conv.js
```

字体来源和授权见 `THIRD_PARTY_NOTICES.md`。生成脚本会固定字重为 500 并检查所需字形。

### 重力滚球难度调整

三关使用连接棋盘边界的交替开口墙，必须上下往返通过。墙数依次为3、4、5，开口宽度为72、52、36像素，球直径20像素。第三关需要更小幅度倾斜，提前回正减速。

### 电量显示

大厅顶部显示AXP2101报告的电量百分比，每10秒刷新；充电中显示绿色，电池供电且剩余不超过20%显示黄色。未接电池、USB供电、读取失败分别提示，未知电量显示--。退出大厅释放刷新定时器。百分比为芯片估计值，不代表经过续航标定。

### 实体按键翻页

大厅中BOOT（GPIO9）上一页，KEY/IO10（GPIO10）下一页；屏幕面向自己、USB朝下时对应左右键。40 ms消抖，长按不连翻，同时按下不翻页；详情、游戏与亮度弹窗中不触发。中间PWR仍为电源键。BOOT在复位时仍具有芯片原生下载模式功能。引脚及布局依据[官方资料](https://docs.waveshare.net/ESP32-C6-Touch-AMOLED-2.16/)和[原理图](https://files.waveshare.com/wiki/ESP32-C6-Touch-AMOLED-2.16/ESP32-C6-Touch-AMOLED-2.16-Schematic.pdf)。
