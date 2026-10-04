#pragma once
class FloorInput {
 public:
  int floor=1;
  bool centered=false;
  void reset(){floor=1;centered=false;}
  void update(int y){
    if(y>=-750 && y<=750){centered=true;return;}
    if(!centered)return;
    if(y>1000){if(floor<10)++floor;centered=false;}
    else if(y<-1000){if(floor>1)--floor;centered=false;}
  }
};
