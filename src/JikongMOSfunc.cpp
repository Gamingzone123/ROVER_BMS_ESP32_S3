#include "Jikong_Handler.h"

bool JikongMessenger::setMOS_state(bool enable_discharge, bool enable_charge){
  if (enable_charge && enable_discharge) {
    byte chargeON[2] = {0xAB, 0x01};
    bool cON = _createMOSframe(chargeON, 2);
    delay(500);

    byte dischargeON[2] = {0xAC, 0x01};
    bool dcON = _createMOSframe(dischargeON, 2);
    return cON && dcON;
  }

  if (!enable_charge && enable_discharge) {
    byte chargeOFF[2] = {0xAB, 0x00};
    bool cOFF = _createMOSframe(chargeOFF, 2);
    delay(500);

    byte dischargeON[2] = {0xAC, 0x01};
    bool dcON = _createMOSframe(dischargeON, 2);
    return cOFF && dcON;
  }

  if (enable_charge && !enable_discharge) {
    byte chargeON[2] = {0xAB, 0x01};
    bool cON = _createMOSframe(chargeON, 2);
    delay(500);

    byte dischargeOFF[2] = {0xAC, 0x00};
    bool dcOFF = _createMOSframe(dischargeOFF, 2);
    return cON && dcOFF;
  }

  // both OFF
  byte chargeOFF[2] = {0xAB, 0x00};
  bool cOFF = _createMOSframe(chargeOFF, 2);
  delay(500);

  byte dischargeOFF[2] = {0xAC, 0x00};
  bool dcOFF = _createMOSframe(dischargeOFF, 2);
  return cOFF && dcOFF;
}

bool JikongMessenger::_MOScommandACK(uint8_t* msg, uint16_t length){
  int8_t start_ind  = -1;

  for(uint16_t i = 0; i + 1 < length; i++){
    if(msg[i]==0x4E && msg[i+1] == 0x57){
      start_ind = i;
      break; 
    }
  }

  if(start_ind < 0){return false;}
  if(start_ind + 10 >= length) {return false;}

  return  msg[start_ind + 8] == 0x02 && msg[start_ind + 9] == 0x00 && msg[start_ind + 10] == 0x01;
}

//[0x4E, 0x57,xx, xx, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x00, "0xAB", "0x01" , 0x00, 0x00, 0x00, 0x01, 0x68, Checksum]
bool JikongMessenger::_createMOSframe(byte *command, uint8_t size){

  uint8_t len = size + 20; 
  byte *frame = (byte*)malloc(len);

  if(!frame){
      Serial.println("Frame Memory allocation failed."); 
      return false;
  }

  frame[0] = 0x4E;
  frame[1] = 0x57;

  //append length (big endian conversion)
  frame[2] = ((len -2 ) >> 8) & 0xFF;
  frame[3] = (len -2 ) & 0xFF;


  frame[4] = 0x00;
  frame[5] = 0x00;
  frame[6] = 0x00;
  frame[7] = 0x00;

  frame[8]= 0x02; //Command word 
  frame[9] = 0x03; // Frame source 
  frame[10] = 0x00;

  // add frame
  frame[11] = command[0];
  frame[12] = command[1];
  //record number
  frame[13] = 0x00;
  frame[14] = 0x00;
  frame[15] = 0x00;
  frame[16] = 0x01;
  //end byte 
  frame[17] = 0x68;
  uint32_t checksum = _getChecksum(frame, len - 4);
  frame[18] = 0x00;
  frame[19] = 0x00;
  frame[20] = checksum >> 8 & 0xFF;
  frame[21] = checksum & 0xFF;

  Dynamic_Array JKreply;
  _sendFrame(JKreply, frame, len);

  bool ack = _MOScommandACK(JKreply.body, JKreply.index);
  free(frame);
  free(JKreply.body);
  return ack;
} 