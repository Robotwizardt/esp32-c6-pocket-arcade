# 烧录九款游戏的中文大厅

这些文件由 ESP-IDF 5.5.3 为 **ESP32-C6-Touch-AMOLED-2.16** 构建，已在 COM5 完成烧录与启动验证；游戏实机操作反馈见 ACCEPTANCE.md。
烧录后会用游戏大厅替换开发板当前程序。

连接开发板 USB-C，打开 ESP-IDF 终端，在本目录执行以下命令，将 COMx 换成实际串口：

```powershell
python -m esptool --chip esp32c6 --port COMx --baud 460800 write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader.bin 0x8000 partition-table.bin 0x10000 game_lobby.bin
```

| 文件 | 烧录地址 |
| --- | --- |
| bootloader.bin | 0x0 |
| partition-table.bin | 0x8000 |
| game_lobby.bin | 0x10000 |

`game-lobby-merged.bin` 是合并镜像，适用于需要单个文件的烧录工具，地址为 **0x0**。
合并镜像包含分区之间的填充数据，会清除其覆盖范围内原有 NVS 数据；保留独立文件烧录方式更便于后续更新。
文件校验值见 `SHA256SUMS.txt`。

如果无法自动连接，按住 BOOT 后重新接电进入下载模式，再执行烧录命令。
烧录完成后若仍停留在下载模式，松开 BOOT 并重新上电。
正常启动日志应出现 `Starting Pocket Arcade lobby`，随后出现 `Chinese lobby ready`，屏幕显示中文“游戏大厅”。

仅更新应用并保留现有 2048 存档时，可执行：

```powershell
python -m esptool --chip esp32c6 --port COMx --baud 460800 write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x10000 game_lobby.bin
```
