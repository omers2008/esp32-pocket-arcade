#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

struct UwbReading {
  uint16_t anchor=0,tag=0;
  uint8_t sequence=0;
  bool valid=false;
  int16_t angle=0,rawAngle=0;
  uint16_t distanceCm=0,rawDistanceCm=0;
};

// MaUWB AOA Gen2 General (81-byte) and General RSSI (89-byte) reports.
// Payload offsets and XOR follow the Anchor datasheet and TagViewer reference.
class UwbParser {
 public:
  uint32_t bytesSeen=0,badFrames=0,frames=0;
  void reset(){used=0;bytesSeen=badFrames=frames=0;}
  bool feed(uint8_t byte,UwbReading &out) {
    ++bytesSeen;
    if(used==sizeof(buffer))discard(1);
    buffer[used++]=byte;
    while(used) {
      if(buffer[0]!=0x2a){discard(1);continue;}
      if(used<3)return false;
      uint32_t size=uint32_t(u16(buffer+1))+5;
      if(size!=81 && size!=89){discard(1);continue;}
      if(used<size)return false;
      uint8_t check=0;for(size_t i=3;i<size-2;++i)check^=buffer[i];
      if(buffer[size-1]!=0x23 || buffer[19]!=(size==81?1:0x64) || check!=buffer[size-2]) {
        ++badFrames;discard(1);continue;
      }
      uint16_t anchor=u16(buffer+25);
      if((buffer[20]!=5 && buffer[20]!=6) || anchor>3){discard(size);continue;}
      size_t offset=31+(size==81?4:6)*anchor;
      out.anchor=anchor;out.tag=u16(buffer+27);out.sequence=buffer[29];
      out.angle=i16(buffer+offset);out.distanceCm=u16(buffer+offset+2);
      size_t raw=size==81?63:71;
      out.rawAngle=i16(buffer+raw);out.rawDistanceCm=u16(buffer+raw+2);
      out.valid=(buffer[30]&(1u<<anchor)) && out.angle>=-180 && out.angle<=180 && out.distanceCm>0;
      ++frames;discard(size);return true;
    }
    return false;
  }
 private:
  uint8_t buffer[89]={};size_t used=0;
  static uint16_t u16(const uint8_t *p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
  static int16_t i16(const uint8_t *p){uint16_t v=u16(p);return v<0x8000?int16_t(v):int16_t(int32_t(v)-65536);}
  void discard(size_t n){used-=n;if(used)memmove(buffer,buffer+n,used);}
};
