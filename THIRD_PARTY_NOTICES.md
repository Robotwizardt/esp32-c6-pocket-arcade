# 第三方代码和来源

本工程的显示、触摸、电源和 LVGL adapter 端口取自工作区中的微雪官方示例：

`work/waveshare/02_Example/ESP-IDF-v5.5.3/09_LVGL_V9_Test`

上游仓库：[waveshareteam/ESP32-C6-Touch-AMOLED-2.16](https://github.com/waveshareteam/ESP32-C6-Touch-AMOLED-2.16)
固定提交：`294543798f1a44e2f2c4d2976522323f2beee11d`。
板卡文档：[Waveshare 官方说明](https://docs.waveshare.com/ESP32-C6-Touch-AMOLED-2.16)。
芯片为 CO5300 / CST9220；以下 SH8601 / CST9217 是官方示例采用的驱动组件名称。

复制或轻量改动的本地组件如下：

- `components/port_bsp`：官方 SH8601 QSPI、CST9217 触摸和 I2C 端口；新增集中式板级引脚配置。
- `components/app_bsp`：官方 LVGL adapter 端口；缓冲高度改为 48 行以匹配端口的 50 行 SPI 传输上限。
- `components/pmicpower`：官方 AXP2101 电源端口及其 MIT 授权声明。

LVGL 与 Espressif/Waveshare 组件不复制到源码中，由 `main/idf_component.yml` 和
`dependencies.lock` 固定并由 ESP-IDF Component Manager 获取。各依赖包自带的
`LICENSE` / `license.txt` 文件应随下载的 managed component 一并保留。

大厅 UI 和 `main/main.cpp` 是本项目新增代码。新增代码仅依赖上述组件的公开 API，
不改变第三方组件的授权和版权信息。

中文字体源自 [Google Fonts / Noto Sans SC](https://github.com/google/fonts/tree/main/ofl/notosanssc)，
按 SIL Open Font License 1.1 分发。授权全文保存在 `main/fonts/OFL.txt`。
使用 `fontTools` 将可变字体固定为字重 500，再用 `lv_font_conv` 1.5.3 生成 4 bpp 子集。
子集包括 ASCII 和 UI 所用汉字、标点，字体文件哈希及字符表在 `main/fonts/font-manifest.json`。

新增 `main/imu_port.cpp` 的 QMI8658 寄存器配置参考同一官方仓库的 `02_I2C_QMI8658` 示例与 [QMI8658C 数据手册](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28/QMI8658C.pdf)。本项目实现轻量 I2C 采样、独立采样任务和互补滤波；未引入新的第三方运行时依赖。
