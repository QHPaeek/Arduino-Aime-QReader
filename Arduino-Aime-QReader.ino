#include "reader.h"

void onQrCodeTask(void *pvParameters)
{
  while (true)
  {
    switch (packet_read()) {
    case 0:
      break;

    case CMD_TO_NORMAL_MODE:
      sys_to_normal_mode();
      break;
    case CMD_GET_FW_VERSION:
      sys_get_fw_version();
      break;
    case CMD_GET_HW_VERSION:
      sys_get_hw_version();
      break;

    // Card read
    case CMD_START_POLLING:
      nfc_start_polling();
      break;
    case CMD_STOP_POLLING:
      nfc_stop_polling();
      break;
    case CMD_CARD_DETECT:
      nfc_card_detect();
      break;

    // MIFARE
    case CMD_MIFARE_KEY_SET_A:
      memcpy(KeyA, req.key, 6);
      res_init();
      break;

    case CMD_MIFARE_KEY_SET_B:
      res_init();
      memcpy(KeyB, req.key, 6);
      break;

    case CMD_MIFARE_AUTHORIZE_A:
      nfc_mifare_authorize_a();
      break;

    case CMD_MIFARE_AUTHORIZE_B:
      nfc_mifare_authorize_b();
      break;

    case CMD_MIFARE_READ:
      nfc_mifare_read();
      break;

    // FeliCa
    case CMD_FELICA_THROUGH:
      nfc_felica_through();
      break;

    // LED
    case CMD_EXT_BOARD_LED_RGB:
    //  if(system_setting[0] & 0b100){
        LED_show((uint8_t)(req.color_payload[0]?system_setting[1]:0), (uint8_t)(req.color_payload[1]?system_setting[1]:0),(uint8_t)(req.color_payload[2]?system_setting[1]:0));
    //    LED_show(req.color_payload[0],req.color_payload[1],req.color_payload[2]);
    //  }
      break;

    case CMD_EXT_BOARD_INFO:
      sys_get_led_info();
      break;

    case CMD_EXT_BOARD_LED_RGB_UNKNOWN:
      break;

    //case CMD_READ_EEPROM:
    //  res_init();
    //  res.payload_len = 4;
    //  res.frame_len = 10;
    //  for(uint8_t i = 0;i<3;i++){
    //    res.eeprom_data[i] = system_setting[i];
    //  }
    //  res.board_vision = BOARD_VISION;
    //  break;
    
    //case CMD_WRITE_EEPROM:
    //    system_setting[0] = req.eeprom_data[0] & 0xFE;
    //    system_setting[1] = req.eeprom_data[1];
    //    EEPROM.write(0, system_setting[0]);
    //    EEPROM.write(1, system_setting[1]);
    //    SerialDevice.begin((system_setting[0] & 0b10)? 115200 : 38400);
    //    if (system_setting[0] & 0b100 != 0b100){
    //      LED_show(0,0,0);
    //    }
    //  res_init();
    //  break;

    default:
      res_init();
  }
  packet_write();
  }
}

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200,SERIAL_8N1, 1, 2, false, 1000);
  //Wire.setPins(1,2);
  LED_Init();
  reader.setup();
  reader.beginOnCore(1);
  nfc.begin();
    while (!nfc.getFirmwareVersion()) {
    delay(500);
    Serial.println("failed");
  //  if(system_setting[0] & 0b100){
      LED_show(0XFF,0x00,0x00);
  //  }'
  }
  nfc.setPassiveActivationRetries(0x10);
  nfc.SAMConfig();
  memset(req.bytes, 0, sizeof(req.bytes));
  memset(res.bytes, 0, sizeof(res.bytes));
  LED_show(0X00,0x00,0xFF);
  //Serial._onLineState(true,true);
  xTaskCreate(onQrCodeTask, "onQrCode", 4 * 1024, NULL, 4, NULL);
}
 
void loop()
{
  delay(100);
}