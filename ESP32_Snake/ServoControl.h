#pragma once
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <atomic>
#include "ArcadeDisplay.h"
#include "ServoInput.h"

class ServoControl {
 public:
  // Must match the other ESP32's station MAC and Wi-Fi channel.
  static constexpr uint8_t CHANNEL=1;
  void start(){
    input.reset();exiting=false;lastDraw=0;shownStatus=-99;shownCommand=0;shownArmed=false;
    radioStatus.store(0);error=nullptr;
    if(!ready){
      if(!WiFi.mode(WIFI_STA)){error="Wi-Fi start failed";return;}
      WiFi.setSleep(false);
      if(esp_wifi_set_channel(CHANNEL,WIFI_SECOND_CHAN_NONE)!=ESP_OK){fail("Channel setup failed");return;}
      if(esp_now_init()!=ESP_OK){fail("ESP-NOW init failed");return;}
      initialized=true;
      esp_now_peer_info_t peer={};memcpy(peer.peer_addr,receiver,6);
      peer.channel=CHANNEL;peer.ifidx=WIFI_IF_STA;peer.encrypt=false;
      if(esp_now_add_peer(&peer)!=ESP_OK){fail("Receiver setup failed");return;}
      if(esp_now_register_send_cb(sent)!=ESP_OK){fail("Radio callback failed");return;}
      ready=true;
    }
    send('S');
  }
  void update(int x,bool stop){
    char command=input.update(x,stop);
    if(ready && (command!=lastCommand || uint32_t(millis()-lastSend)>=100))send(command);
  }
  void leave(){
    input.reset();
    if(ready){send('S');exiting=true;exitAt=millis();}
  }
  // Continue stop retries briefly after leaving, without blocking the menu.
  void service(){
    if(!exiting)return;
    if(uint32_t(millis()-exitAt)>=350){shutdown();exiting=false;return;}
    if(uint32_t(millis()-lastSend)>=100)send('S');
  }
  void draw(ArcadeDisplay &display){
    int status=radioStatus.load();
    if(shownCommand==input.command && shownStatus==status && shownArmed==input.armed)return;
    // Radio callbacks must never draw. Snapshot state here in the main loop.
    if(lastDraw && uint32_t(millis()-lastDraw)<100)return;
    display.setGame(-1);
    if(!display.colorFrame([&](Adafruit_GFX &d){
      using namespace ColorMenu;
      d.fillScreen(BG);d.setTextWrap(false);
      label(d,12,12,"SERVO CONTROL",CYAN,2);
      label(d,12,42,"TO 1C:69:20:30:73:1C  /  CH 1",MUTED);
      d.fillRoundRect(12,68,296,76,8,CARD);
      const char *name=input.command=='L'?"LEFT":input.command=='R'?"RIGHT":"STOP";
      label(d,input.command=='R'?115:124,90,name,input.command=='S'?GOLD:CYAN,3);
      label(d,12,158,error?error:!input.armed?"CENTER JOYSTICK TO ARM":"JOYSTICK LEFT / CENTER / RIGHT",error?0xF9C7:WHITE);
      label(d,12,180,!ready?"RADIO NOT READY":status==1?"RADIO: DELIVERED":status<0?"RADIO: SEND FAILED":"RADIO: WAITING",status<0?0xF9C7:MUTED);
      label(d,12,196,"No servo-position feedback",MUTED);
      label(d,12,223,"13 STOP   12 STOP + MENU",WHITE);
    }))return;
    shownCommand=input.command;shownStatus=status;shownArmed=input.armed;lastDraw=millis();
  }
 private:
  const uint8_t receiver[6]={0x1C,0x69,0x20,0x30,0x73,0x1C};
  struct Message{char command;};
  static_assert(sizeof(Message)==1,"Receiver protocol must remain one byte");
  inline static std::atomic<int> radioStatus{0};
  ServoInput input;
  bool ready=false,initialized=false,exiting=false,shownArmed=false;
  uint32_t lastSend=0,exitAt=0,lastDraw=0;
  char lastCommand='S',shownCommand=0;
  int shownStatus=-99;
  const char *error=nullptr;
  static void sent(const esp_now_send_info_t *,esp_now_send_status_t result){
    radioStatus.store(result==ESP_NOW_SEND_SUCCESS?1:-1);
  }
  void send(char command){
    Message message{command};lastCommand=command;lastSend=millis();
    if(esp_now_send(receiver,reinterpret_cast<const uint8_t*>(&message),sizeof(message))!=ESP_OK)radioStatus.store(-1);
  }
  void shutdown(){
    if(initialized){esp_now_deinit();initialized=false;}
    ready=false;WiFi.mode(WIFI_OFF);
  }
  void fail(const char *reason){error=reason;shutdown();}
};
