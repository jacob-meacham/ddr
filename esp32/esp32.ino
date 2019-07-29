#include <rom/rtc.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9F"

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristic2;

BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;
bool bleConnected = false;

// #define DEBUG

const int TRIGGER_THRESHOLD = 80;

const int NUM_ANALOG = 4;
const int LEFT = A4;
const int RIGHT = 32;
const int UP = A3;
const int DOWN = A2;
const int SELECT = A0; // Digital
const int START = 33; // Digital

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LEFT, INPUT);
  pinMode(RIGHT, INPUT);
  pinMode(UP, INPUT);
  pinMode(DOWN, INPUT);
  pinMode(START, INPUT);
  pinMode(SELECT, INPUT);

  xTaskCreate(taskServer, "server", 20000, NULL, 5, NULL);
}

uint16_t buttonsPrev = 0;
void loop() {
  int analogPins[NUM_ANALOG] = { LEFT, RIGHT, UP, DOWN };
  int digitalPins[2] = { START, SELECT };
  
  int readValues[6] = { 0 };
  char out[6] = { 'a', 'd', 'w', 's', 'o', 'p' };
 
  bool pressed = false;
 
  for (int i = 0; i < NUM_ANALOG; i++) {
    readValues[i] = analogRead(analogPins[i]);
  }

  for (int i = 0; i < 2; i++) {
    readValues[NUM_ANALOG + i] = TRIGGER_THRESHOLD + 1; // Reset the trigger value
    if (!digitalRead(digitalPins[i])) {
      readValues[NUM_ANALOG + i] = 0; // Set under trigger threshold
    }
  }

  uint16_t buttons = 0;
  for (int i = 0; i < 6; i++) {
    if(readValues[i] < TRIGGER_THRESHOLD) {
      buttons |= (1 << i);
      pressed = true;
    }
  }

  if (bleConnected) {
    if (buttons != buttonsPrev) {
      uint8_t msg[] = {0x00, 0x00, buttons, (buttons >> 8)};
      input->setValue(msg, sizeof(msg));
      input->notify();
  
      buttonsPrev = buttons;
    }
  }

  //Illuminate the LED if a button is pressed
  if(pressed) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
  
#if defined(DEBUG)
  Serial.printf("LEFT: %d, RIGHT: %d, UP: %d, DOWN: %d, START: %d, SELECT: %d\n", readValues[0], readValues[1], readValues[2], readValues[3], readValues[4], readValues[5]);
  delay(1000);
#endif

  delay(5);
}

class DDRPadCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    bleConnected = true;
    BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    desc->setNotifications(true);
  }

  void onDisconnect(BLEServer* pServer){
    bleConnected = false;
    BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    desc->setNotifications(false);
  }
};

 class DDRPadOutputCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic* me){
    uint8_t* value = (uint8_t*)(me->getValue().c_str());
    ESP_LOGI(LOG_TAG, "special keys: %d", *value);
  }
};

void taskServer(void*){
  BLEDevice::init("DDR Pad");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new DDRPadCallbacks());
  hid = new BLEHIDDevice(pServer);
  input = hid->inputReport(1); // <-- input REPORTID from report map
  output = hid->outputReport(1); // <-- output REPORTID from report map
  output->setCallbacks(new DDRPadOutputCallbacks());
  std::string name = "DDR Pad";
  hid->manufacturer()->setValue(name);
  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->hidInfo(0x00,0x02);
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pService->start();
  BLESecurity *pSecurity = new BLESecurity();;
  pSecurity->setCapability(ESP_IO_CAP_NONE);

//  const uint8_t report[] = {
//    USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
//    USAGE(1),           0x06,       // Keyboard
//    COLLECTION(1),      0x01,       // Application
//    REPORT_ID(1),       0x01,        //   Report ID (1)
//    USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
//    USAGE_MINIMUM(1),   0xE0,
//    USAGE_MAXIMUM(1),   0xE7,
//    LOGICAL_MINIMUM(1), 0x00,
//    LOGICAL_MAXIMUM(1), 0x01,
//    REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
//    REPORT_COUNT(1),    0x08,
//    HIDINPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
//    REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
//    REPORT_SIZE(1),     0x08,
//    HIDINPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
//    REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
//    REPORT_SIZE(1),     0x08,
//    LOGICAL_MINIMUM(1), 0x00,
//    LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
//    USAGE_MINIMUM(1),   0x00,
//    USAGE_MAXIMUM(1),   0x65,
//    HIDINPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
//    REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
//    REPORT_SIZE(1),     0x01,
//    USAGE_PAGE(1),      0x08,       //   LEDs
//    USAGE_MINIMUM(1),   0x01,       //   Num Lock
//    USAGE_MAXIMUM(1),   0x05,       //   Kana
//    HIDOUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
//    REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
//    REPORT_SIZE(1),     0x03,
//    HIDOUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
//    END_COLLECTION(0)
//  };
  const uint8_t reportMap[] = {
    0x05, 0x01,                   // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                   // USAGE (Game Pad)
    0xa1, 0x01,                   // COLLECTION (Application)
    0xa1, 0x02,                   //    COLLECTION (Logical)
    0x85, 0x01,                    //      REPORT_ID (1)
    
    0x75, 0x08,                   //      REPORT_SIZE (8)
    0x95, 0x02,                   //      REPORT_COUNT (2)
    0x05, 0x01,                   //      USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                   //      USAGE (X)
    0x09, 0x31,                   //      USAGE (Y)
    0x15, 0x81,                   //      LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                   //      LOGICAL_MAXIMUM (127)
    0x81, 0x02,                   //      INPUT (Data, Var, Abs)
    
    0x75, 0x01,                   //      REPORT_SIZE (1)
    0x95, 0x0b,                   //      REPORT_COUNT (11)
    0x15, 0x00,                   //      LOGICAL_MINIMUM (0)
    0x25, 0x01,                   //      LOGICAL_MAXIMUM (1)
    0x05, 0x09,                   //      USAGE_PAGE (Button)
    0x19, 0x01,                   //      USAGE_MINIMUM (Button 1)
    0x29, 0x0b,                   //      USAGE_MAXIMUM (Button 11)
    0x81, 0x02,                   //      INPUT (Data, Var, Abs)
    // PADDING for byte alignment
    0x75, 0x01,                   //      REPORT_SIZE (1)
    0x95, 0x05,                   //      REPORT_COUNT (5)
    0x81, 0x03,                   //      INPUT (Constant, Var, Abs)
    
    0xc0,                         //   END_COLLECTION
    0xc0                          // END_COLLECTION
  };
  hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));
  hid->startServices();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_GAMEPAD);
  pAdvertising->addServiceUUID(hid->hidService()->getUUID());
  pAdvertising->start();
  hid->setBatteryLevel(7);
  ESP_LOGD(LOG_TAG, "Advertising started!");
  delay(portMAX_DELAY);
}
