#pragma once

// Center-to-arm plus hysteresis keeps menu motion and stick noise from driving
// the remote servo. A stop-button press requires centering again.
class ServoInput {
 public:
  static constexpr int START_THRESHOLD=1000;
  static constexpr int STOP_THRESHOLD=750;
  bool armed=false;
  char command='S';
  void reset(){armed=false;command='S';}
  char update(int x,bool stop){
    if(stop){reset();return command;}
    if(!armed){if(x>=-STOP_THRESHOLD && x<=STOP_THRESHOLD)armed=true;return command='S';}
    if(x>START_THRESHOLD)command='R';
    else if(x<-START_THRESHOLD)command='L';
    else if((command=='R' && x<=STOP_THRESHOLD)||(command=='L' && x>=-STOP_THRESHOLD))command='S';
    return command;
  }
};
