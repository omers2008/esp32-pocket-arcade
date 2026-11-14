#define ARCADE_TEST_TFT
#include <Arduino.h>
#include "../ESP32_Snake/ArcadeDisplay.h"
#include "../ESP32_Snake/UwbLinkProtocol.h"
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/UwbTagView.h"
#undef private
uint32_t testClock=0;
int main() {
  using namespace UwbLink;
  Message m;m.kind=READING;m.session=0x12345678;m.sequence=0x87654321;
  m.ageMs=91;m.uartBytes=0x12345678;m.badFrames=9;m.frames=1234;
  m.reading.anchor=3;m.reading.tag=0x154f;m.reading.sequence=200;
  m.reading.valid=true;m.reading.angle=-179;m.reading.rawAngle=-180;
  m.reading.distanceCm=65535;m.reading.rawDistanceCm=123;
  uint8_t bytes[PACKET_SIZE];encode(m,bytes);Message decoded;
  assert(bytes[0]=='U' && bytes[3]=='2' && bytes[4]==1 && bytes[5]==READING);
  assert(bytes[6]==0x21 && bytes[9]==0x87 && bytes[38]==0x78 && bytes[41]==0x12);
  assert(decode(bytes,sizeof(bytes),decoded));
  assert(decoded.session==m.session && decoded.sequence==m.sequence && decoded.ageMs==91);
  assert(decoded.reading.anchor==3 && decoded.reading.tag==0x154f && decoded.reading.sequence==200);
  assert(decoded.reading.angle==-179 && decoded.reading.rawAngle==-180 && decoded.reading.distanceCm==65535);
  assert(decoded.uartBytes==m.uartBytes && decoded.badFrames==9 && decoded.frames==1234);
  assert(!decode(bytes,sizeof(bytes)-1,decoded));
  bytes[0]='?';assert(!decode(bytes,sizeof(bytes),decoded));encode(m,bytes);
  bytes[4]=2;assert(!decode(bytes,sizeof(bytes),decoded));encode(m,bytes);
  bytes[5]=3;assert(!decode(bytes,sizeof(bytes),decoded));encode(m,bytes);
  bytes[12]=4;assert(!decode(bytes,sizeof(bytes),decoded));encode(m,bytes);
  bytes[17]=2;assert(!decode(bytes,sizeof(bytes),decoded));encode(m,bytes);
  put16(bytes+18,181);assert(!decode(bytes,sizeof(bytes),decoded));
  bytes[17]=0;assert(decode(bytes,sizeof(bytes),decoded) && !decoded.reading.valid);
  m.kind=STATUS;encode(m,bytes);assert(decode(bytes,sizeof(bytes),decoded));

  Order order;m.session=1;m.sequence=100;
  assert(order.accept(m));assert(!order.accept(m));--m.sequence;assert(!order.accept(m));
  m.sequence=101;assert(order.accept(m));m.session=2;m.sequence=1;assert(order.accept(m));
  order.reset();m.sequence=0xffffffffu;assert(order.accept(m));m.sequence=0;assert(order.accept(m));

  Pending pending;UwbReading r=m.reading;r.anchor=0;r.tag=1;r.angle=20;r.valid=true;r.distanceCm=200;
  pending.store(r,100);r.angle=30;pending.store(r,110);
  r.tag=2;pending.store(r,120);UwbReading next;uint16_t age;
  int slot=pending.next(130,next,age);assert(slot>=0 && next.tag==1 && next.angle==30 && age==20);
  pending.sent(slot);slot=pending.next(130,next,age);assert(slot>=0 && next.tag==2 && age==10);
  pending.sent(slot);assert(pending.next(130,next,age)<0);
  r.tag=3;pending.store(r,1000);assert(pending.next(1999,next,age)>=0 && age==999);
  assert(pending.next(2000,next,age)<0); // Never replay a sample that is already stale.
  pending.store(r,0xfffffff0u);assert(pending.next(10,next,age)>=0 && age==26);
  for(int i=0;i<20;++i){r.tag=uint16_t(i);pending.store(r,100+i);}
  int count=0;while((slot=pending.next(200,next,age))>=0){++count;pending.sent(slot);}
  assert(count==8); // Fixed bound, rather than a backlog of every UART report.

  UwbTagView view;view.start();view.useWireless(true);assert(view.status(0)==5);
  view.bridgeStatus(0,0,0,100,1);assert(view.status(100)==3); // Radio link, no UART.
  view.bridgeStatus(40,1,0,200,1);assert(view.status(200)==2);
  r.tag=1;r.angle=20;r.valid=true;r.distanceCm=200;
  view.bridgeStatus(81,1,1,300,1);view.receiveReading(r,250);assert(view.live(300));
  view.bridgeStatus(81,1,1,1200,1);assert(view.live(1200));
  view.update(0,0,false,false,1250);assert(!view.live(1250) && !view.positionValid);
  view.bridgeStatus(81,1,1,2000,1);assert(!view.live(2000) && view.status(2000)==1);
  assert(view.status(3500)==5); // Heartbeat loss has its own indication.
  view.bridgeStatus(0,0,0,3600,2);assert(view.selected<0 && view.status(3600)==3);
  view.useWireless(false);assert(view.status(3600)==4); // Radio startup failure.
  ArcadeDisplay display;assert(display.begin());view.draw(display,3600);
  assert(!tftTrace.writing);
  std::cout<<"PASS: radio packet layout, validation, ordering/reboot, coalescing, sample aging and link-vs-tag status.\n";
}
