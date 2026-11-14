#pragma once
#include <stdint.h>
#include <math.h>
#include "../../ESP32_Snake/UwbProtocol.h"

// Incremental position control: the sensor's angle is relative to its rotating
// face, so assigning 90+error as an absolute servo angle would hunt forever.
class PanTracker {
 public:
  static constexpr float MIN_POSITION=30,MAX_POSITION=150,CENTER=90;
  static constexpr uint32_t SAMPLE_TIMEOUT_MS=300,CONTROL_MS=50;

  void begin(uint32_t now,bool reversed=true) {
    position=CENTER;reverse=reversed;enabled=true;locked=sample=moving=false;
    lastTick=lastSample=now;filteredError=0;anchor=tag=0;
  }
  void observe(const UwbReading &r,uint32_t now) {
    if(locked && (r.anchor!=anchor || r.tag!=tag))return;
    bool usable=r.valid && r.distanceCm>0 && r.angle>=-60 && r.angle<=60;
    if(!usable){sample=moving=false;return;}
    if(!locked){anchor=r.anchor;tag=r.tag;locked=true;}
    if(sample && now-lastSample<SAMPLE_TIMEOUT_MS) {
      float alpha=1-expf(-float(now-lastSample)/120.0f);
      filteredError+=alpha*(float(r.angle)-filteredError);
    } else filteredError=float(r.angle);
    lastSample=now;sample=true;
    float magnitude=fabsf(filteredError);
    if(magnitude<=4)moving=false;
    else if(magnitude>=7)moving=true;
  }
  bool tick(uint32_t now) {
    uint32_t elapsed=now-lastTick;
    if(elapsed<CONTROL_MS)return false;
    lastTick=now;
    if(!enabled || !fresh(now) || !moving)return false;
    // Cap catch-up: a delayed loop must not cause a large physical jump.
    if(elapsed>100)elapsed=100;
    float speed=filteredError*0.6f*(reverse?-1.0f:1.0f);
    if(speed>20)speed=20;
    if(speed< -20)speed=-20;
    float next=position+speed*float(elapsed)/1000;
    if(next<MIN_POSITION)next=MIN_POSITION;
    if(next>MAX_POSITION)next=MAX_POSITION;
    bool changed=fabsf(next-position)>0.001f;position=next;return changed;
  }
  bool fresh(uint32_t now) const {return sample && now-lastSample<SAMPLE_TIMEOUT_MS;}
  bool isEnabled() const {return enabled;}
  bool isReversed() const {return reverse;}
  bool hasTarget() const {return locked;}
  uint16_t targetTag() const {return tag;}
  uint16_t targetAnchor() const {return anchor;}
  float command() const {return position;}
  float error() const {return filteredError;}
  uint16_t pulseUs() const {return uint16_t(1000+position*1000/180+0.5f);}
  void setReverse(bool value){reverse=value;sample=moving=false;}
  void setEnabled(bool value){enabled=value;sample=moving=false;}
  void nextTarget(){locked=sample=moving=false;}
  void centerAndPause(uint32_t now){position=CENTER;enabled=false;sample=moving=false;lastTick=now;}
  const char *status(uint32_t now) const {
    if(!enabled)return "PAUSED";
    if(!locked)return "WAIT TAG";
    if(!fresh(now))return "NO TAG";
    if(moving && (position<=MIN_POSITION || position>=MAX_POSITION))return "LIMIT";
    return moving?"TRACKING":"CENTERED";
  }
 private:
  float position=CENTER,filteredError=0;
  bool enabled=true,reverse=true,locked=false,sample=false,moving=false;
  uint16_t anchor=0,tag=0;
  uint32_t lastTick=0,lastSample=0;
};
