#pragma once
#include "UwbProtocol.h"

namespace UwbLink {
constexpr uint8_t CHANNEL=1;
constexpr uint8_t ARCADE_MAC[6]={0x00,0x70,0x07,0xA5,0xC0,0xC0};
constexpr uint8_t BRIDGE_MAC[6]={0x20,0x50,0x0D,0x6F,0xFC,0x68};
constexpr size_t PACKET_SIZE=42;
enum Kind:uint8_t { READING=1,STATUS=2 };
struct Message {
  Kind kind=STATUS;
  uint32_t sequence=0,session=0,uartBytes=0,badFrames=0,frames=0;
  uint16_t ageMs=0;
  UwbReading reading;
};
inline void put16(uint8_t *p,uint16_t v){p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
inline void put32(uint8_t *p,uint32_t v){for(int i=0;i<4;++i)p[i]=uint8_t(v>>(8*i));}
inline uint16_t get16(const uint8_t *p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
inline uint32_t get32(const uint8_t *p){uint32_t v=0;for(int i=0;i<4;++i)v|=uint32_t(p[i])<<(8*i);return v;}
inline int16_t signed16(const uint8_t *p){uint16_t v=get16(p);return v<0x8000?int16_t(v):int16_t(int32_t(v)-65536);}
// Explicit little-endian layout, independent of compiler padding or bool size.
inline void encode(const Message &m,uint8_t *p) {
  memset(p,0,PACKET_SIZE);memcpy(p,"UWB2",4);p[4]=1;p[5]=m.kind;
  put32(p+6,m.sequence);put16(p+10,m.ageMs);
  put16(p+12,m.reading.anchor);put16(p+14,m.reading.tag);
  p[16]=m.reading.sequence;p[17]=m.reading.valid?1:0;
  put16(p+18,uint16_t(m.reading.angle));put16(p+20,m.reading.distanceCm);
  put16(p+22,uint16_t(m.reading.rawAngle));put16(p+24,m.reading.rawDistanceCm);
  put32(p+26,m.uartBytes);put32(p+30,m.badFrames);put32(p+34,m.frames);put32(p+38,m.session);
}
inline bool decode(const uint8_t *p,size_t size,Message &m) {
  if(size!=PACKET_SIZE || memcmp(p,"UWB2",4) || p[4]!=1 || (p[5]!=READING && p[5]!=STATUS))return false;
  if(p[5]==READING && (get16(p+12)>3 || p[17]>1))return false;
  m.kind=static_cast<Kind>(p[5]);m.sequence=get32(p+6);m.ageMs=get16(p+10);
  m.reading.anchor=get16(p+12);m.reading.tag=get16(p+14);m.reading.sequence=p[16];m.reading.valid=p[17]!=0;
  m.reading.angle=signed16(p+18);m.reading.distanceCm=get16(p+20);
  m.reading.rawAngle=signed16(p+22);m.reading.rawDistanceCm=get16(p+24);
  m.uartBytes=get32(p+26);m.badFrames=get32(p+30);m.frames=get32(p+34);m.session=get32(p+38);
  if(m.kind==READING && m.reading.valid && (m.reading.angle < -180 || m.reading.angle>180 || !m.reading.distanceCm))return false;
  return true;
}
class Order {
 public:
  void reset(){seen=false;}
  bool accept(const Message &m) {
    if(seen && session==m.session) {
      uint32_t delta=m.sequence-sequence;
      if(!delta || delta>=0x80000000u)return false;
    }
    seen=true;session=m.session;sequence=m.sequence;return true;
  }
 private:
  bool seen=false;uint32_t session=0,sequence=0;
};

// Latest sample per identity: a slow/broken radio cannot accumulate old poses.
class Pending {
 public:
  void store(const UwbReading &r,uint32_t now) {
    int index=-1;
    for(int i=0;i<8;++i)if(slots[i].used && slots[i].reading.anchor==r.anchor && slots[i].reading.tag==r.tag){index=i;break;}
    if(index<0)for(int i=0;i<8;++i)if(!slots[i].used){index=i;break;}
    if(index<0){index=0;for(int i=1;i<8;++i)if(now-slots[i].stamp>now-slots[index].stamp)index=i;}
    slots[index]={r,now,true,true};
  }
  int next(uint32_t now,UwbReading &r,uint16_t &age) {
    for(int n=0;n<8;++n) {
      int i=(cursor+n)%8;
      if(!slots[i].pending)continue;
      uint32_t elapsed=now-slots[i].stamp;
      if(elapsed>=1000){slots[i].pending=false;continue;}
      r=slots[i].reading;age=uint16_t(elapsed);return i;
    }
    return -1;
  }
  void sent(int index){slots[index].pending=false;cursor=(index+1)%8;}
 private:
  struct Slot{UwbReading reading;uint32_t stamp=0;bool used=false,pending=false;};
  Slot slots[8];int cursor=0;
};
}
