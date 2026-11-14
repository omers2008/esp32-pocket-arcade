#pragma once
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <atomic>
#include "UwbLinkProtocol.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Wi-Fi callback only copies packets; decoding and drawing run in loop().
class UwbRadioReceiver {
 public:
  bool start() {
    leave();order.reset();
    if(!queue)queue=xQueueCreate(24,sizeof(Event));
    if(!queue)return false;
    xQueueReset(queue);dropped.store(0);
    if(!WiFi.mode(WIFI_STA))return false;
    WiFi.setSleep(false);
    if(esp_wifi_set_channel(UwbLink::CHANNEL,WIFI_SECOND_CHAN_NONE)!=ESP_OK){leave();return false;}
    if(esp_now_init()!=ESP_OK){leave();return false;}
    initialized=true;
    if(esp_now_register_recv_cb(received)!=ESP_OK){leave();return false;}
    active.store(true);return true;
  }
  void leave() {
    active.store(false);
    if(initialized){esp_now_unregister_recv_cb();esp_now_deinit();initialized=false;}
    // Queue persists: a callback already in flight must never write freed memory.
    WiFi.mode(WIFI_OFF);
  }
  bool poll(UwbLink::Message &message,uint32_t &receivedAt) {
    if(!queue)return false;
    Event event;
    while(xQueueReceive(queue,&event,0)==pdTRUE) {
      if(UwbLink::decode(event.bytes,sizeof(event.bytes),message) && order.accept(message)) {
        receivedAt=event.stamp;return true;
      }
    }
    return false;
  }
 private:
  struct Event{uint8_t bytes[UwbLink::PACKET_SIZE];uint32_t stamp;};
  inline static QueueHandle_t queue=nullptr;
  inline static std::atomic<bool> active{false};
  inline static std::atomic<uint32_t> dropped{0};
  bool initialized=false;
  UwbLink::Order order;
  static void received(const esp_now_recv_info_t *info,const uint8_t *data,int size) {
    if(!active.load() || !queue || !info || size!=int(UwbLink::PACKET_SIZE) || memcmp(info->src_addr,UwbLink::BRIDGE_MAC,6))return;
    Event event;memcpy(event.bytes,data,sizeof(event.bytes));event.stamp=millis();
    if(xQueueSend(queue,&event,0)!=pdTRUE)dropped.fetch_add(1);
  }
};
