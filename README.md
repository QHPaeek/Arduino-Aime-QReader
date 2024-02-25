# Arduino-Aime-QReader

使用 ESP32S3 + PN532 +OV2640制作的 Aime-QR 兼容读卡器。

- 支持卡片类型： [FeliCa](https://zh.wikipedia.org/wiki/FeliCa)（Amusement IC、Suica、八达通等）和 [MIFARE](https://zh.wikipedia.org/wiki/MIFARE)（Aime，Banapassport）
- 逻辑实现是通过对官方读卡器串口数据进行分析猜测出来的，并非逆向，不保证正确实现
- 通信数据格式参考了 [Segatools](https://github.com/djhackersdev/segatools) 和官方读卡器抓包数据
- 通过读取QR码并模拟Aime卡读取逻辑以实现扫码登陆功能
- 可以通过baudrate_tool轻易的修改内置LED参数以及功能而无需重新刷写固件
- 由于使用了USB-CDC，因此无需修改波特率就能适配大部分游戏

### 使用方法：

1. 按照 [PN532](https://github.com/elechouse/PN532) 的提示安装库
2. 将目录下[ESP32QRCodeReader](/ESP32QRCodeReader)目录拷贝至`C:\Users\youruser\Documents\Arduino\libraries`目录下
3. 按照`/PCB` 目录内对应硬件的使用方式，短接读卡器的mode-sw跳线进入下载模式，同时将读卡器与电脑中间接好线，并调整 PN532 上的拨码开关
4. 上传[Arduino-Aime-QReader](Arduino-Aime-QReader.ino)，按照支持列表打开设备管理器设置 COM 端口号
5. 如果有使用 [Segatools](https://github.com/djhackersdev/segatools)，请关闭 Aime 模拟读卡器
6. 上传程序打开游戏测试

QR码的格式：以“Aime"开头，后写入16位任何ascii字符，并生成二维码即可。

例如：`Aimeu7si81jfy6E5wQ8i`

### 支持游戏列表：

| 代号        | 默认 COM 号 | 支持的卡          |
| --------- | -------- | ------------- |
| SDDT/SDEZ | COM1     | FeliCa,MIFARE |
| SDEY      | COM2     | MIFARE        |
| SDHD      | COM4     | FeliCa,MIFARE |
| SBZV/SDDF | COM10    | FeliCa,MIFARE |
| SDBT      | COM12    | FeliCa,MIFARE |

- 参考 config_common.json 内 aime > unit > port 确认端口号
- 在 `"high_baudrate" : true` 的情况下，本读卡器程序支持 emoney 功能，端末认证和刷卡支付均正常（需要游戏和服务器支持）

### 开发板适配情况：

仅适配ESP32S3。请在Arduino IDE中安装这个包

### 已知问题：

- 在 NDA_08 命令的写入 Felica 操作没有实现，因为未确认是否会影响卡片后续使用
- 未确定`res.status`错误码的定义，因此`res.status`的值可能是错误的
- 因为 PN532 库不支持同时读取多张卡片，所以未实现`mifare_select_tag`，只会读到最先识别的卡片
- 扫码后不登陆而选择返回可能会导致重复刷卡
  
  

### TODO:

增加QR识别准确率，引入PN532串口直通功能。

### 引用库：

- 驱动 WS2812B：[FastLED](https://github.com/FastLED/FastLED)
- 驱动 PN532：[PN532](https://github.com/elechouse/PN532) 或 [Aime_Reader_PN532](https://github.com/Sucareto/Aime_Reader_PN532)
- 读取 FeliCa 参考：[PN532を使ってArduinoでFeliCa学生証を読む方法](https://qiita.com/gpioblink/items/91597a5275862f7ffb3c)
- 读取 FeliCa 数据的程序：[NFC TagInfo](https://play.google.com/store/apps/details?id=at.mroland.android.apps.nfctaginfo)，[NFC TagInfo by NXP](https://play.google.com/store/apps/details?id=com.nxp.taginfolite)
- ESP32扫码：[ESP32QRCodeReader](https://github.com/alvarowolfx/ESP32QRCodeReader)

交流群：531883107
