#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ESP32QRCodeReader.h>
#define SerialDevice Serial
#define NUM_LEDS 11
#define LED_PIN 9
#define BOARD_VISION 4

#include "lib/WS2812_FastLed.h"
#include <Wire.h>
#include <PN532_HSU.h>
PN532_HSU pn532(Serial2);

#include "PN532.h"
PN532 nfc(pn532);
uint8_t KeyA[6], KeyB[6];

//#include <EEPROM.h>
//uint8_t system_setting[3] = {0};
uint8_t system_setting[3] = {0b00000110,128,3};
ESP32QRCodeReader reader(CAMERA_MODEL_ESP32_S3_CAM);
struct QRCodeData qrCodeData;
uint8_t QR_access_code[16] = {0};
uint8_t mifare_buffer[16] = {0x1d,0XB4,0X0F,0X0D,0XAB,0X88,0X04,0X00,0XC8,0X32,0X00,0X20,0X00,0X00,0X00,0X16};
uint8_t QR_flag = 0;

bool QRconvert(const char* input, uint8_t output[16]) {
    // 检查输入字符串是否以"Aime"开头
    if (strncmp(input, "Aime", 4) != 0) {
        return false;
    }
    for (int i = 0; i < 16; ++i) {
        output[i] = (uint8_t)input[i];
    }
    return true;
}

// 检查数组内每个半字节是否大于9，如果大于9则等于9
void checkAndLimitHalfBytes(uint8_t array[], size_t length) {
    for (size_t i = 0; i < length; ++i) {
        // 获取高4位和低4位
        uint8_t highNibble = (array[i] >> 4) & 0x0F;
        uint8_t lowNibble = array[i] & 0x0F;

        // 检查高4位和低4位是否大于9，如果是则设置为9
        if (highNibble > 9) {
            highNibble = 9;
        }
        if (lowNibble > 9) {
            lowNibble = 9;
        }

        // 合并高4位和低4位
        array[i] = (highNibble << 4) | lowNibble;
    }
}

enum {
  CMD_GET_FW_VERSION = 0x30,
  CMD_GET_HW_VERSION = 0x32,
  // Card read
  CMD_START_POLLING = 0x40,
  CMD_STOP_POLLING = 0x41,
  CMD_CARD_DETECT = 0x42,
  CMD_CARD_SELECT = 0x43,
  CMD_CARD_HALT = 0x44,
  // MIFARE
  CMD_MIFARE_KEY_SET_A = 0x50,
  CMD_MIFARE_AUTHORIZE_A = 0x51,
  CMD_MIFARE_READ = 0x52,
  CMD_MIFARE_WRITE = 0x53,
  CMD_MIFARE_KEY_SET_B = 0x54,
  CMD_MIFARE_AUTHORIZE_B = 0x55,
  // Boot,update
  CMD_TO_UPDATER_MODE = 0x60,
  CMD_SEND_HEX_DATA = 0x61,
  CMD_TO_NORMAL_MODE = 0x62,
  CMD_SEND_BINDATA_INIT = 0x63,
  CMD_SEND_BINDATA_EXEC = 0x64,
  // FeliCa
  CMD_FELICA_PUSH = 0x70,
  CMD_FELICA_THROUGH = 0x71,
  CMD_FELICA_THROUGH_POLL = 0x00,
  CMD_FELICA_THROUGH_READ = 0x06,
  CMD_FELICA_THROUGH_WRITE = 0x08,
  CMD_FELICA_THROUGH_GET_SYSTEM_CODE = 0x0C,
  CMD_FELICA_THROUGH_NDA_A4 = 0xA4,
  // LED board
  CMD_EXT_BOARD_LED = 0x80,
  CMD_EXT_BOARD_LED_RGB = 0x81,
  CMD_EXT_BOARD_LED_RGB_UNKNOWN = 0x82,  // 未知
  CMD_EXT_BOARD_INFO = 0xf0,
  CMD_EXT_FIRM_SUM = 0xf2,
  CMD_EXT_SEND_HEX_DATA = 0xf3,
  CMD_EXT_TO_BOOT_MODE = 0xf4,
  CMD_EXT_TO_NORMAL_MODE = 0xf5,
  //读卡器上位机功能
  CMD_READ_EEPROM = 0xf6,
  CMD_WRITE_EEPROM =0xf7,
};

enum {  // 未确认效果
  ERROR_NONE = 0,
  ERROR_NFCRW_INIT_ERROR = 1,
  ERROR_NFCRW_FIRMWARE_UP_TO_DATE = 3,
  ERROR_NFCRW_ACCESS_ERROR = 4,
  ERROR_CARD_DETECT_TIMEOUT = 5,
  ERROR_CARD_DETECT_ERROR = 32,
  ERROR_FELICA_ERROR = 33,
};

typedef union {
  uint8_t bytes[128];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t payload_len;
    union {
      uint8_t key[6];            // CMD_MIFARE_KEY_SET
      uint8_t color_payload[3];  // CMD_EXT_BOARD_LED_RGB
      uint8_t eeprom_data[2];     //系统内部设置
      struct {                   // CMD_CARD_SELECT,AUTHORIZE,READ
        uint8_t uid[4];
        uint8_t block_no;
      };
      struct {  // CMD_FELICA_THROUGH
        uint8_t encap_IDm[8];
        uint8_t encap_len;
        uint8_t encap_code;
        union {
          struct {  // CMD_FELICA_THROUGH_POLL
            uint8_t poll_systemCode[2];
            uint8_t poll_requestCode;
            uint8_t poll_timeout;
          };
          struct {  // CMD_FELICA_THROUGH_READ,WRITE,NDA_A4
            uint8_t RW_IDm[8];
            uint8_t numService;
            uint8_t serviceCodeList[2];
            uint8_t numBlock;
            uint8_t blockList[1][2];  // CMD_FELICA_THROUGH_READ
            uint8_t blockData[16];    // CMD_FELICA_THROUGH_WRITE
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_request_t;

typedef union {
  uint8_t bytes[128];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t status;
    uint8_t payload_len;
    union {
      uint8_t version[1];  // CMD_GET_FW_VERSION,CMD_GET_HW_VERSION,CMD_EXT_BOARD_INFO
      uint8_t block[16];   // CMD_MIFARE_READ
      struct{
        uint8_t eeprom_data[3];
        uint8_t board_vision;
      };
      struct {             // CMD_CARD_DETECT
        uint8_t count;
        uint8_t type;
        uint8_t id_len;
        union {
          uint8_t mifare_uid[4];
          struct {
            uint8_t IDm[8];
            uint8_t PMm[8];
          };
        };
      };
      struct {  // CMD_FELICA_THROUGH
        uint8_t encap_len;
        uint8_t encap_code;
        uint8_t encap_IDm[8];
        union {
          struct {  // FELICA_CMD_POLL
            uint8_t poll_PMm[8];
            uint8_t poll_systemCode[2];
          };
          struct {
            uint8_t RW_status[2];
            uint8_t numBlock;
            uint8_t blockData[1][1][16];
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_response_t;

packet_request_t req;
packet_response_t res;

uint8_t len, r, checksum;
bool escape = false;
uint8_t SERIALnum = 0;

uint8_t packet_read() {
  while (SerialDevice.available()) {
    r = SerialDevice.read();
    if (r == 0xE0) {
      req.frame_len = 0xFF;
      continue;
    }
    if (req.frame_len == 0xFF) {
      req.frame_len = r;
      len = 0;
      checksum = r;
      continue;
    }
    if (r == 0xD0) {
      escape = true;
      //Serial.println("readOK");
      continue;
    }
    if (escape) {
      r++;
      escape = false;
    }
    req.bytes[++len] = r;
    if (len == req.frame_len && checksum == r) {
      //Serial.printf("test");
      return req.cmd;
    }
    checksum += r;
    SERIALnum = SERIALnum -1;
  }
  return 0;
}

void packet_write() {
  uint8_t checksum = 0, len = 0;
  if (res.cmd == 0) {
    return;
  }
  SerialDevice.write(0xE0);
  while (len <= res.frame_len) {
    uint8_t w;
    if (len == res.frame_len) {
      w = checksum;
    } else {
      w = res.bytes[len];
      checksum += w;
    }
    if (w == 0xE0 || w == 0xD0) {
      SerialDevice.write(0xD0);
      SerialDevice.write(--w);
    } else {
      SerialDevice.write(w);
    }
    len++;
  }
  res.cmd = 0;
}

void res_init(uint8_t payload_len = 0) {
  res.frame_len = 6 + payload_len;
  res.addr = req.addr;
  res.seq_no = req.seq_no;
  res.cmd = req.cmd;
  res.status = ERROR_NONE;
  res.payload_len = payload_len;
}

void sys_to_normal_mode() {
  res_init();
  if (nfc.getFirmwareVersion()) {
    res.status = ERROR_NFCRW_FIRMWARE_UP_TO_DATE;
  } else {
    res.status = ERROR_NFCRW_INIT_ERROR;
    LED_show(0xFF,00,00);
  }
}

void sys_get_fw_version() {
  const char* fw_version = (system_setting[0] & 0b10) ? "\x94" : "TN32MSEC003S F/W Ver1.2";
  res_init(sizeof(fw_version) - 1);
  memcpy(res.version, fw_version, res.payload_len);
}

void sys_get_hw_version() {
  const char* hw_version = (system_setting[0] & 0b10) ? "837-15396" : "TN32MSEC003S H/W Ver3.0";
  res_init(sizeof(hw_version) - 1);
  memcpy(res.version, hw_version, res.payload_len);
}

void sys_get_led_info() {
  const char* led_info = (system_setting[0] & 0b10) ? "000-00000\xFF\x11\x40" : "15084\xFF\x10\x00\x12";
  res_init(sizeof(led_info) - 1);
  memcpy(res.version, led_info, res.payload_len);
}


void nfc_start_polling() {
  res_init();
  nfc.setRFField(0x00, 0x01);
}

void nfc_stop_polling() {
  res_init();
  nfc.setRFField(0x00, 0x00);
}

void nfc_card_detect() {
  uint16_t SystemCode;
  uint8_t bufferLength;
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, res.mifare_uid, &res.id_len) && nfc.getBuffer(&bufferLength)[4] == 0x08) {  // Only read cards with sak=0x08
    res_init(0x07);
    res.count = 1;
    res.type = 0x10;
  } else if (nfc.felica_Polling(0xFFFF, 0x00, res.IDm, res.PMm, &SystemCode, 100) == 1) {
    res_init(0x13);
    res.count = 1;
    res.type = 0x20;
    res.id_len = 0x10;
  }else if (reader.receiveQrCode(&qrCodeData, 50)){
    if(qrCodeData.valid){
      QRconvert((const char *)qrCodeData.payload, QR_access_code);
      res_init(0x07);
      res.count = 1;
      res.type = 0x10;
      QR_flag = 1;
    }
    else {
    res_init(1);
    res.count = 0;
    res.status = ERROR_NONE;
    }
  }
  else {
    res_init(1);
    res.count = 0;
    res.status = ERROR_NONE;
  }

}

void nfc_mifare_authorize_a() {
  res_init();
  if ((!nfc.mifareclassic_AuthenticateBlock(req.uid, 4, req.block_no, 0, KeyA)) && (QR_flag == 0)) {
    res.status = ERROR_NFCRW_ACCESS_ERROR;
  }
}

void nfc_mifare_authorize_b() {
  res_init();
  if ((!nfc.mifareclassic_AuthenticateBlock(req.uid, 4, req.block_no, 1, KeyB)) && (QR_flag == 0)) {
    res.status = ERROR_NFCRW_ACCESS_ERROR;
  }
}

void nfc_mifare_read() {
  res_init(0x10);
  if(QR_flag == 1){
    switch(req.block_no){
      case 0:
        memcpy(res.block,mifare_buffer,16);
        break;
      case 1:
        for(uint8_t i = 0;i<16;i++) 
        {
          res.block[i] = 0;
        }
        break;
      case 2:
        for(uint8_t j = 0;j<6;j++)
          {
            QR_access_code[j] = 0; 
          }
        checkAndLimitHalfBytes(QR_access_code,16);
        memcpy(res.block,QR_access_code,16);
        for(uint8_t i = 0;i<16;i++) 
        {
          QR_access_code[i] = 0;
        }
        QR_flag = 0;
        break;
      default:
        //memcpy(res.block,mifare_buffer,16);
        res.status = ERROR_CARD_DETECT_TIMEOUT;
        break;

    }
  }else if (!nfc.mifareclassic_ReadDataBlock(req.block_no, res.block)) {
    res_init();
    res.status = ERROR_CARD_DETECT_TIMEOUT;
  }
}

void nfc_felica_through() {
  uint16_t SystemCode;
  if (nfc.felica_Polling(0xFFFF, 0x01, res.encap_IDm, res.poll_PMm, &SystemCode, 200) == 1) {
    SystemCode = SystemCode >> 8 | SystemCode << 8;
  } else {
    res_init();
    res.status = ERROR_FELICA_ERROR;
    return;
  }
  uint8_t code = req.encap_code;
  res.encap_code = code + 1;
  switch (code) {
    case CMD_FELICA_THROUGH_POLL:
      {
        res_init(0x14);
        res.poll_systemCode[0] = SystemCode;
        res.poll_systemCode[1] = SystemCode >> 8;
      }
      break;
    case CMD_FELICA_THROUGH_GET_SYSTEM_CODE:
      {
        res_init(0x0D);
        res.felica_payload[0] = 0x01;
        res.felica_payload[1] = SystemCode;
        res.felica_payload[2] = SystemCode >> 8;
      }
      break;
    case CMD_FELICA_THROUGH_NDA_A4:
      {
        res_init(0x0B);
        res.felica_payload[0] = 0x00;
      }
      break;
    case CMD_FELICA_THROUGH_READ:
      {
        uint16_t serviceCodeList[1] = { (uint16_t)(req.serviceCodeList[1] << 8 | req.serviceCodeList[0]) };
        for (uint8_t i = 0; i < req.numBlock; i++) {
          uint16_t blockList[1] = { (uint16_t)(req.blockList[i][0] << 8 | req.blockList[i][1]) };
          if (nfc.felica_ReadWithoutEncryption(1, serviceCodeList, 1, blockList, res.blockData[i]) != 1) {
            memset(res.blockData[i], 0, 16);  // dummy data
          }
        }
        res.RW_status[0] = 0;
        res.RW_status[1] = 0;
        res.numBlock = req.numBlock;
        res_init(0x0D + req.numBlock * 16);
      }
      break;
    case CMD_FELICA_THROUGH_WRITE:
      {
        res_init(0x0C);  // WriteWithoutEncryption,ignore
        res.RW_status[0] = 0;
        res.RW_status[1] = 0;
      }
      break;
    default:
      res_init();
      res.status = ERROR_FELICA_ERROR;
  }
  res.encap_len = res.payload_len;
}