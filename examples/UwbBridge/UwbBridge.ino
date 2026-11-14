#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <atomic>
#include <Preferences.h>
#include "../../ESP32_Snake/UwbLinkProtocol.h"
#include "PanTracker.h"

// Anchor TXD -> GPIO16 (3.3 V-safe signal), VCC -> 5 V, GND -> GND.
// Anchor RXD and ESP32 GPIO17 stay disconnected. Never send module commands.
// SG90 signal -> GPIO18; servo power -> external regulated 5 V, common GND.
constexpr uint8_t SERVO_PIN=18;
constexpr uint32_t SERVO_HZ=50;
constexpr uint8_t SERVO_RESOLUTION=16;
PanTracker pan;
bool servoReady=false;
HardwareSerial anchorSerial(2);
UwbParser parser;
UwbLink::Pending pending;
std::atomic<bool> busy{false};
std::atomic<uint32_t> delivered{0},failed{0};
bool radioReady=false;
uint32_t session=0,sequence=0,lastSend=0,nextAttempt=0,lastLog=0;
uint8_t packet[UwbLink::PACKET_SIZE];

void writePan() {
  if(servoReady) {
    uint32_t duty=(uint32_t(pan.pulseUs())*65536u+10000u)/20000u;
    if(!ledcWrite(SERVO_PIN,duty)){servoReady=false;Serial.println("Servo PWM write failed");}
  }
}

bool savePanSettings() {
  Preferences prefs;
  if(!prefs.begin("uwbpan",false))return false;
  bool savedReverse=prefs.putBool("reverse",pan.isReversed())==1;
  bool savedTracking=prefs.putBool("tracking",pan.isEnabled())==1;
  prefs.end();return savedReverse && savedTracking;
}

void panConsole() {
  for(int n=0;n<32 && Serial.available();++n) {
    char command=char(Serial.read());
    if(command>='a' && command<='z')command-=32;
    if(command=='R') {
      pan.setReverse(!pan.isReversed());
    } else if(command=='P')pan.setEnabled(false);
    else if(command=='T')pan.setEnabled(true);
    else if(command=='C'){pan.centerAndPause(millis());writePan();}
    else if(command=='['){pan.positionAndPause(75,millis());writePan();}
    else if(command==']'){pan.positionAndPause(105,millis());writePan();}
    else if(command=='N'){pan.nextTarget();continue;}
    else continue;
    Serial.printf("Pan command %c: position=%.1f us=%u tracking=%d reverse=%d saved=%d\n",
      command,double(pan.command()),unsigned(pan.pulseUs()),pan.isEnabled(),pan.isReversed(),savePanSettings());
  }
}

void onSent(const esp_now_send_info_t *,esp_now_send_status_t status) {
  if(status==ESP_NOW_SEND_SUCCESS)delivered.fetch_add(1);else failed.fetch_add(1);
  busy.store(false);
}

void setup() {
  Serial.begin(115200);
  bool reverse=true,tracking=true;
  Preferences prefs;
  if(prefs.begin("uwbpan",true)){reverse=prefs.getBool("reverse",true);tracking=prefs.getBool("tracking",true);prefs.end();}
  pan.begin(millis(),reverse);
  if(!tracking)pan.setEnabled(false);
  servoReady=ledcAttach(SERVO_PIN,SERVO_HZ,SERVO_RESOLUTION);writePan();
  Serial.printf("SG90 GPIO18 / 50 Hz / center 1500us / reverse=%d / PWM %s\n",reverse,servoReady?"OK":"FAILED");
  Serial.println("Pan console: [ 75deg, ] 105deg, C center+pause, P pause, T track, R reverse, N next tag; settings saved");
  anchorSerial.setRxBufferSize(4096);
  anchorSerial.begin(115200,SERIAL_8N1,16,17);
  WiFi.mode(WIFI_STA);WiFi.setSleep(false);
  Serial.printf("\nUWB BRIDGE\nStation MAC: %s\n",WiFi.macAddress().c_str());
  if(esp_wifi_set_channel(UwbLink::CHANNEL,WIFI_SECOND_CHAN_NONE)!=ESP_OK || esp_now_init()!=ESP_OK) {
    Serial.println("ESP-NOW initialization failed");return;
  }
  esp_now_peer_info_t peer={};memcpy(peer.peer_addr,UwbLink::ARCADE_MAC,6);
  peer.channel=UwbLink::CHANNEL;peer.ifidx=WIFI_IF_STA;peer.encrypt=false;
  if(esp_now_add_peer(&peer)!=ESP_OK || esp_now_register_send_cb(onSent)!=ESP_OK) {
    Serial.println("ESP-NOW peer/callback setup failed");return;
  }
  session=esp_random();radioReady=true;lastSend=millis()-500;
  Serial.println("To arcade 00:70:07:A5:C0:C0 / channel 1 / UART RX16 @115200");
}

void loop() {
  for(int n=0;n<4096 && anchorSerial.available();++n) {
    int byte=anchorSerial.read();UwbReading reading;
    if(byte>=0 && parser.feed(uint8_t(byte),reading)) {
      uint32_t stamp=millis();pending.store(reading,stamp);pan.observe(reading,stamp);
    }
  }
  uint32_t now=millis();
  panConsole();
  if(servoReady && pan.tick(now))writePan();
  if(radioReady && !busy.load() && int32_t(now-nextAttempt)>=0) {
    UwbLink::Message message;int slot=pending.next(now,message.reading,message.ageMs);
    if(slot>=0 || now-lastSend>=500) {
      message.kind=slot>=0?UwbLink::READING:UwbLink::STATUS;
      message.sequence=++sequence;message.session=session;
      message.uartBytes=parser.bytesSeen;message.badFrames=parser.badFrames;message.frames=parser.frames;
      UwbLink::encode(message,packet);busy.store(true);
      esp_err_t result=esp_now_send(UwbLink::ARCADE_MAC,packet,sizeof(packet));
      if(result==ESP_OK){lastSend=nextAttempt=now;if(slot>=0)pending.sent(slot);}
      else {busy.store(false);failed.fetch_add(1);nextAttempt=now+20;}
    }
  }
  if(now-lastLog>=1000) {
    lastLog=now;
    Serial.printf("UART=%lu B frames=%lu bad=%lu radio ACK=%lu fail=%lu pan=%.1f us=%u %s tag=%04X/A%u error=%+.1f reverse=%d\n",
      (unsigned long)parser.bytesSeen,(unsigned long)parser.frames,(unsigned long)parser.badFrames,
      (unsigned long)delivered.load(),(unsigned long)failed.load(),double(pan.command()),unsigned(pan.pulseUs()),
      servoReady?pan.status(now):"PWM ERROR",unsigned(pan.targetTag()),unsigned(pan.targetAnchor()),double(pan.error()),pan.isReversed());
  }
  delay(1);
}
