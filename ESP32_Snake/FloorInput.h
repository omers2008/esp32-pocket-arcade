#pragma once
class FloorInput {
 public:
  int floor=1;
  bool centered=false;
  void reset(){floor=1;centered=false;}
  void update(int x,int y){
    if(x>=-750 && x<=750 && y>=-750 && y<=750){centered=true;return;}
    if(!centered)return;
    int ax=x<0?-x:x,ay=y<0?-y:y;
    if(ax<=1000 && ay<=1000)return;
    int column=(floor-1)%5;
    if(ax>=ay){
      if(x>0 && column<4)++floor;
      else if(x<0 && column>0)--floor;
    } else {
      if(y>0 && floor>5)floor-=5;
      else if(y<0 && floor<=5)floor+=5;
    }
    centered=false;
  }
};
