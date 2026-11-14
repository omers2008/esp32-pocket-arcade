#include "../examples/UwbBridge/PanTracker.h"
#include <cassert>
#include <cmath>
#include <iostream>

UwbReading tag(int angle=30) {
  UwbReading r;r.valid=true;r.anchor=0;r.tag=0x154f;r.distanceCm=200;r.angle=int16_t(angle);return r;
}
int main() {
  PanTracker pan;pan.begin(0,false);
  assert(pan.command()==90 && pan.pulseUs()==1500 && !pan.tick(50));
  auto r=tag();pan.observe(r,50);assert(pan.tick(100) && pan.command()>90 && pan.command()<=91);
  assert(pan.targetTag()==0x154f && pan.targetAnchor()==0);
  pan.begin(0,true);pan.observe(r,0);assert(pan.tick(50) && pan.command()<90);
  float position=pan.command();pan.tick(300);assert(pan.command()==position && !pan.fresh(300));

  // Close-to-zero noise does not start an endless twitching loop.
  pan.begin(0,false);
  for(uint32_t t=0;t<=3000;t+=50){r=tag(int(t%9)-4);pan.observe(r,t);pan.tick(t);}
  assert(pan.command()==90);
  r=tag(30);pan.observe(r,3050);pan.tick(3050);assert(pan.command()>90);
  position=pan.command();r.valid=false;pan.observe(r,3060);pan.tick(3100);assert(pan.command()==position);

  // Locking the first identity avoids following alternating tags/anchors.
  pan.begin(0,false);r=tag();pan.observe(r,0);
  r.tag=0x9999;r.angle=-30;pan.observe(r,250);pan.tick(300);
  assert(pan.targetTag()==0x154f && pan.command()==90 && !pan.fresh(300));
  pan.nextTarget();pan.observe(r,310);assert(pan.targetTag()==0x9999);
  position=pan.command();r.angle=61;pan.observe(r,320);pan.tick(350);assert(pan.command()==position);

  // Commanded travel and pulse width stay away from the mechanical extremes.
  pan.begin(0,false);r=tag(60);
  for(uint32_t t=0;t<=20000;t+=50){pan.observe(r,t);pan.tick(t);}
  assert(pan.command()==PanTracker::MAX_POSITION && pan.pulseUs()<=1834);
  r.angle=-60;
  for(uint32_t t=20050;t<=50000;t+=50){pan.observe(r,t);pan.tick(t);}
  assert(pan.command()==PanTracker::MIN_POSITION && pan.pulseUs()>=1166);

  // A stalled loop cannot execute accumulated seconds of movement at once.
  pan.begin(0,false);r=tag(60);pan.observe(r,1000);pan.tick(1000);
  assert(pan.command()<=92);
  pan.begin(0xffffffe0u,false);pan.observe(r,0xffffffe0u);pan.tick(18);
  assert(pan.command()>90 && pan.command()<=91); // millis wrap.

  // Closed-loop model: positive angle is right; check both servo polarities.
  for(int reverse=0;reverse<2;++reverse) {
    pan.begin(0,reverse!=0);float target=25;
    for(uint32_t t=0;t<=20000;t+=50) {
      float heading=(pan.command()-90)*(reverse?-1.0f:1.0f);
      r=tag(int(lroundf(target-heading)));pan.observe(r,t);pan.tick(t);
    }
    float heading=(pan.command()-90)*(reverse?-1.0f:1.0f);
    assert(fabsf(target-heading)<=5 && fabsf(pan.command()-90)>15);
    position=pan.command();pan.setEnabled(false);
    r=tag(-30);pan.observe(r,20050);pan.tick(20050);assert(pan.command()==position);
    pan.setEnabled(true);pan.tick(20100);assert(pan.command()==position); // Fresh report required.
    pan.setReverse(!pan.isReversed());pan.tick(20150);assert(pan.command()==position);
    pan.centerAndPause(20200);assert(pan.command()==90 && pan.pulseUs()==1500 && !pan.isEnabled());
  }
  std::cout<<"PASS: proportional pan convergence, both polarities, deadband, identity lock, stale/invalid hold, pulse limits and clock wrap.\n";
}
