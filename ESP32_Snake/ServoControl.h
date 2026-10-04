#pragma once
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <atomic>
#include "ArcadeDisplay.h"
#include "FloorInput.h"

class ServoControl {
 public:
  // Must match the other ESP32's station MAC and Wi-Fi channel.
  static constexpr uint8_t CHANNEL=1;
  void start(){
    input.reset();lastDraw=0;shownStatus=-99;shownFloor=0;lastSent=0;shownSent=-1;pending=false;
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

  }
  void update(int x,int y,bool confirm){
    input.update(x,y);
    if(ready && confirm && !pending){
      Message message{static_cast<uint8_t>(input.floor)};
      radioStatus.store(0);pending=true;lastSent=input.floor;
      if(esp_now_send(receiver,reinterpret_cast<const uint8_t*>(&message),sizeof(message))!=ESP_OK){
        radioStatus.store(-1);pending=false;
      }
    }
    if(radioStatus.load()!=0)pending=false;
  }
  void leave(){shutdown();}
  void service(){}
  void draw(ArcadeDisplay &display){
    int status=radioStatus.load();
    if(shownFloor==input.floor && shownStatus==status && shownSent==lastSent && shownCentered==input.centered)return;
    // Radio callbacks must never draw. Snapshot state here in the main loop.
    if(lastDraw && uint32_t(millis()-lastDraw)<100)return;
    display.setGame(-1);
    if(!display.colorFrame([&](Adafruit_GFX &d){
      using namespace ColorMenu;
      d.fillScreen(BG);d.setTextWrap(false);
      label(d,12,12,"SELECT FLOOR",CYAN,2);
      label(d,12,38,"JOYSTICK MOVE   13 SELECT",MUTED);
      for(int floor=1;floor<=10;++floor){
        int x=14+(floor-1)%5*60,y=56+(floor-1)/5*58;
        bool selected=floor==input.floor;
        d.fillRoundRect(x,y,52,52,5,selected?ACTIVE:CARD);
        d.drawRoundRect(x,y,52,52,5,selected?CYAN:MUTED);
        if(selected)d.drawRoundRect(x+1,y+1,50,50,4,CYAN);
        d.setTextSize(3);d.setTextColor(selected?GOLD:WHITE);
        d.setCursor(x+(floor==10?8:17),y+15);d.print(floor);
      }
      label(d,12,170,error?error:"Release stick between moves",error?0xF9C7:MUTED);
      label(d,12,184,!ready?"RADIO NOT READY":status==1?"RADIO: DELIVERED":status<0?"RADIO: SEND FAILED":"RADIO: WAITING",status<0?0xF9C7:MUTED);
      d.setTextSize(1);d.setTextColor(MUTED);d.setCursor(12,200);d.print("LAST SENT: ");if(lastSent)d.print(lastSent);else d.print("-");
      label(d,12,223,"13 SEND FLOOR   12 MENU",WHITE);
    }))return;
    shownFloor=input.floor;shownStatus=status;shownSent=lastSent;shownCentered=input.centered;lastDraw=millis();
  }
 private:
  const uint8_t receiver[6]={0x1C,0x69,0x20,0x30,0x73,0x1C};
  struct Message{uint8_t floor;};
  static_assert(sizeof(Message)==1,"Receiver protocol must remain one byte");
  inline static std::atomic<int> radioStatus{0};
  FloorInput input;
  bool ready=false,initialized=false,pending=false;
  bool shownCentered=false;
  uint32_t lastDraw=0;
  int shownFloor=0,lastSent=0,shownSent=-1;
  int shownStatus=-99;
  const char *error=nullptr;
  static void sent(const esp_now_send_info_t *,esp_now_send_status_t result){
    radioStatus.store(result==ESP_NOW_SEND_SUCCESS?1:-1);
  }
  void shutdown(){
    if(initialized){esp_now_deinit();initialized=false;}
    ready=false;WiFi.mode(WIFI_OFF);
  }
  void fail(const char *reason){error=reason;shutdown();}
};
