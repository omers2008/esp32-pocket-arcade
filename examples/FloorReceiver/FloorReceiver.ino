// Companion receiver: receives floor requests only; does not drive the servo.
// Merge targetFloor into the other project's position/sensor control logic.
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <atomic>

const uint8_t controllerMAC[6]={0x00,0x70,0x07,0xA5,0xC0,0xC0};
std::atomic<int> requestedFloor{0};

void onFloor(const esp_now_recv_info_t *info,const uint8_t *data,int len){
  if(!info || len!=1 || memcmp(info->src_addr,controllerMAC,6)!=0)return;
  if(data[0]>=1 && data[0]<=10)requestedFloor.store(data[0]);
}

void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  if(esp_wifi_set_channel(1,WIFI_SECOND_CHAN_NONE)!=ESP_OK || esp_now_init()!=ESP_OK){
    Serial.println("ESP-NOW setup failed");return;
  }
  esp_now_register_recv_cb(onFloor);
  Serial.print("Receiver MAC: ");Serial.println(WiFi.macAddress());
  Serial.println("Ready for floors 1-10");
}

void loop(){
  int targetFloor=requestedFloor.exchange(0);
  if(targetFloor){Serial.print("Requested floor: ");Serial.println(targetFloor);}
  delay(10);
}
