#include "Jikong_Handler.h"

//initilize empty dispatch table 
JikongMessenger::Handler JikongMessenger::_dispatchTable[256] = {};

// call class constructor and initilize class varaibles 
JikongMessenger::JikongMessenger(HardwareSerial* serial_input, uint16_t Reply_timeout, uint8_t NumberCells) : _serial(serial_input), _timeout(Reply_timeout), _numCells(NumberCells){
    
    if(_numCells > _maxCells){_numCells = _maxCells;} //clamp number 

    _initHandlers(); 
    _clearData();
    _serial->flush(); //empty the TX buffer 
    while(_serial->available()){ // empty RX buffer 
        _serial->read();
    }
}

void JikongMessenger::_initCellVoltage_mV(){ 
    
    for(int i = 0; i < _numCells; i++){
        _cells->cellNum = i+1;
        _cells->cellVoltage = 0;
    }
   _Stored_Data.cell_voltage_mV = _cells;
}

void JikongMessenger::_clearWarningFlags(){
    _Stored_Data.warning_flags.low_capacity = false;       
    _Stored_Data.warning_flags.MOS_tube_OT = false;
    _Stored_Data.warning_flags.charging_OV = false; 
    _Stored_Data.warning_flags.discharge_UV = false; 
    _Stored_Data.warning_flags.battery_OT = false;
    _Stored_Data.warning_flags.charging_OC = false;
    _Stored_Data.warning_flags.discharge_OC = false;
    _Stored_Data.warning_flags.Cell_pressure_differential = false;
    _Stored_Data.warning_flags.BB_OT = false;
    _Stored_Data.warning_flags.battery_low_temp = false;
    _Stored_Data.warning_flags.monomer_OV = false;          
    _Stored_Data.warning_flags.monomer_UV = false;         
    _Stored_Data.warning_flags.protection_309A = false; 
}

void JikongMessenger::_clearStatusFlags(){
    _Stored_Data.status_flags.charging_MOS_status = NULL; //1: Charging MOSFET ON, 0: Charging MOSFET OFF
    _Stored_Data.status_flags.discharge_MOS_status = NULL; //1: Discharge MOSFET ON, 0: Discharge MOSFET OFF
    _Stored_Data.status_flags.balance_switch_state = NULL;    //1=Balance switch  ON, 0=Balance switch 0 OFF
    _Stored_Data.status_flags.battery_conn_status = NULL; //1: Battery connected / normal, 0: Battery offline / disconnected 
}

void JikongMessenger::_initHandlers(){

  for (int i = 0; i < 256; i++) {
    _dispatchTable[i] = nullptr;
  }

  _dispatchTable[0x79] = &JikongMessenger::_cell_voltage_mV;          

    /* ---------------- Temperatures ---------------- */
  _dispatchTable[0x80] = &JikongMessenger::_handle_power_tube_temp_dC;          
  _dispatchTable[0x81] = &JikongMessenger::_handle_battery_box_temp_dC;         
  _dispatchTable[0x82] = &JikongMessenger::_handle_battery_temp_dC;            

  /* ---------------- Electrical ---------------- */
  _dispatchTable[0x83] = &JikongMessenger::_handle_total_voltage_mV;          
  _dispatchTable[0x84] = &JikongMessenger::_handle_current_dA;                  

  _dispatchTable[0x85] = &JikongMessenger::_handle_remaining_capacity_pct,      
  _dispatchTable[0x86] = &JikongMessenger::_temp_sensor_count_n;            
  _dispatchTable[0x87] = &JikongMessenger::_handle_cycle_count;                  
  _dispatchTable[0x89] = &JikongMessenger::_handle_total_cycle_capacity_Ah;      
  _dispatchTable[0x8a] = &JikongMessenger::_handle_string_count_n;                

  /* ---------------- Status & Warnings ---------------- */
  _dispatchTable[0x8b] = &JikongMessenger::_handle_warning_flags;                 
  _dispatchTable[0x8c] = &JikongMessenger::_handle_status_flags;              

  /* ---------------- Voltage Protections ---------------- */
  _dispatchTable[0x8e] = &JikongMessenger::_handle_pack_overvoltage_mV;
  _dispatchTable[0x8f] = &JikongMessenger::_handle_pack_undervoltage_mV;

  _dispatchTable[0x90] = &JikongMessenger::_handle_cell_overvoltage_mV;
  _dispatchTable[0x91] = &JikongMessenger::_handle_cell_overvoltage_recovery_mV;
  _dispatchTable[0x92] = &JikongMessenger::_handle_cell_overvoltage_delay_s;
  _dispatchTable[0x93] = &JikongMessenger::_handle_cell_undervoltage_mV;
  _dispatchTable[0x94] = &JikongMessenger::_handle_Monomer_undervoltage_release_mV;
  _dispatchTable[0x95] = &JikongMessenger::_handle_cell_undervoltage_delay_s;

  _dispatchTable[0x96] = &JikongMessenger::_handle_cell_pressure_diff_protection_mV;
  /* ---------------- Current Protections ---------------- */
  _dispatchTable[0x97] = &JikongMessenger::_handle_discharge_overcurrent_A; 
  _dispatchTable[0x98] = &JikongMessenger::_handle_discharge_overcurrent_delay_s;

  _dispatchTable[0x99] = &JikongMessenger::_handle_charge_overcurrent_A; 
  _dispatchTable[0x9a] = &JikongMessenger::_handle_charge_overcurrent_delay_s; 

  /* ---------------- Balancing ---------------- */
  _dispatchTable[0x9b] = &JikongMessenger::_handle_balance_start_voltage_mV; 
  _dispatchTable[0x9c] = &JikongMessenger::_handle_balance_diff_mV;
  _dispatchTable[0x9d] = &JikongMessenger::_handle_active_balance_enabled; 

  /* ---------------- Temperature Protections ---------------- */

  _dispatchTable[0x9e] = &JikongMessenger::_handle_Power_tube_temp_protection_value_dC;
  _dispatchTable[0x9f] = &JikongMessenger::_handle_Power_tube_temp_recovery_value_dC; 

  _dispatchTable[0xa0] = &JikongMessenger::_handle_box_high_temp_dC;
  _dispatchTable[0xa1] = &JikongMessenger::_handle_box_temp_recovery_dC; 

  _dispatchTable[0xa2] = &JikongMessenger::_battery_temp_diff_dC; 

  _dispatchTable[0xa3] = &JikongMessenger::_charge_high_temp_dC; 
  _dispatchTable[0xa4] = &JikongMessenger::_discharge_high_temp_dC;

  _dispatchTable[0xa5] = &JikongMessenger::_charge_low_temp_dC;
  _dispatchTable[0xa6] = &JikongMessenger::_charge_low_temp_recovery_dC;

  _dispatchTable[0xa7] = &JikongMessenger::_discharge_low_temp_dC;
  _dispatchTable[0xa8] = &JikongMessenger::_discharge_low_temp_recovery_dC; 

  /* ---------------- Configuration ---------------- */
  _dispatchTable[0xa9] = &JikongMessenger::_battery_string_setting;
  _dispatchTable[0xaa] = &JikongMessenger::_battery_capacity_Ah;

  _dispatchTable[0xab] = &JikongMessenger::_charge_mos_enabled; 
  _dispatchTable[0xac] = &JikongMessenger::_discharge_mos_enabled; 

  _dispatchTable[0xad] = &JikongMessenger::_current_calibration_offset;

  _dispatchTable[0xae] = &JikongMessenger::_P_board_address;
  _dispatchTable[0xaf] = &JikongMessenger::_battery_type;                
  _dispatchTable[0xb0] = &JikongMessenger::_sleep_delay_s; 

  _dispatchTable[0xb1] = &JikongMessenger::_low_cap_alarmVol_pct; 

  _dispatchTable[0xb2] = &JikongMessenger::_parameter_password; 

  _dispatchTable[0xb3] = &JikongMessenger::_dedicated_charger_enabled; 

  _dispatchTable[0xb4] = &JikongMessenger::_device_id; 

  _dispatchTable[0xb5] = &JikongMessenger::_manufacture_date; 

  _dispatchTable[0xb6] = &JikongMessenger::_runtime_hours; 

  _dispatchTable[0xb7] = &JikongMessenger::_firmware_version; 

  _dispatchTable[0xb8] = &JikongMessenger::_current_calibration_enabled; 

  _dispatchTable[0xb9] = &JikongMessenger::_actual_capacity_Ah; 

  _dispatchTable[0xba] = &JikongMessenger::_manufacturer_id; 

  _dispatchTable[0xc0] = &JikongMessenger::_protocol_version_number;

}

void JikongMessenger::_clearData(){

    _initCellVoltage_mV();
 
        /* ---------------- Temperatures ---------------- */
        _Stored_Data.power_tube_temp_dC = 0;           
        _Stored_Data.battery_box_temp_dC = 0;           
        _Stored_Data.battery_temp_dC = 0;              

        /* ---------------- Electrical ---------------- */
        _Stored_Data.total_voltage_mV = 0;             
        _Stored_Data.current_dA = 0;                   

        _Stored_Data.remaining_capacity_pct = 0;     
        _Stored_Data.temp_sensor_count_n= 0;            
        _Stored_Data.cycle_count = 0;                  
        _Stored_Data.total_cycle_capacity_Ah = 0;      
        _Stored_Data.string_count_n = 0;                 

        /* ---------------- Status & Warnings ---------------- */
        _clearWarningFlags();              
        _clearStatusFlags();                
        /* ---------------- Voltage Protections ---------------- */
        _Stored_Data.pack_overvoltage_mV = 0;
        _Stored_Data.pack_undervoltage_mV = 0;

        _Stored_Data.cell_overvoltage_mV= 0;
        _Stored_Data.cell_overvoltage_recovery_mV = 0;
        _Stored_Data.cell_overvoltage_delay_s = 0;

        _Stored_Data.cell_undervoltage_mV = 0;
        _Stored_Data.Monomer_undervoltage_release_mV = 0;
        _Stored_Data.cell_undervoltage_delay_s = 0;

        _Stored_Data.cell_pressure_diff_protection_mV = 0;

        /* ---------------- Current Protections ---------------- */
        _Stored_Data.discharge_overcurrent_A = 0;
        _Stored_Data.discharge_overcurrent_delay_s = 0;

        _Stored_Data.charge_overcurrent_A = 0;
        _Stored_Data.charge_overcurrent_delay_s = 0;

        /* ---------------- Balancing ---------------- */
        _Stored_Data.balance_start_voltage_mV = 0;
        _Stored_Data.balance_diff_mV = 0;
        _Stored_Data.active_balance_enabled = NULL;

        /* ---------------- Temperature Protections ---------------- */
        
        _Stored_Data.Power_tube_temp_protection_value_dC = 0;
        _Stored_Data.Power_tube_temp_recovery_value_dC = 0;

        _Stored_Data.box_high_temp_dC = 0;
        _Stored_Data.box_temp_recovery_dC = 0;

        _Stored_Data.battery_temp_diff_dC = 0;

        _Stored_Data.charge_high_temp_dC = 0;
        _Stored_Data.discharge_high_temp_dC = 0;

        _Stored_Data.charge_low_temp_dC = 0;
        _Stored_Data.charge_low_temp_recovery_dC = 0;

        _Stored_Data.discharge_low_temp_dC = 0;
        _Stored_Data.discharge_low_temp_recovery_dC = 0;

        /* ---------------- Configuration ---------------- */
        _Stored_Data.battery_string_setting = 0;
        _Stored_Data.battery_capacity_setting = 0 ;
        _Stored_Data.battery_capacity_Ah = 0;
        _Stored_Data.charge_mos_enabled = NULL;
        _Stored_Data.discharge_mos_enabled = NULL;
        _Stored_Data.current_calibration_offset = 0;
        _Stored_Data.P_board_address = 0;
        _Stored_Data.battery_type = 0;                 
        _Stored_Data.sleep_delay_s = 0;
        _Stored_Data.low_cap_alarmVol_pct = 0;
        for(int i = 0; i < 10; i++){_Stored_Data.parameter_password[i]= 0;}
        _Stored_Data.parameter_password[9] = '\0';
        _Stored_Data.device_id = 0;
        _Stored_Data.manufacture_year = 0;
        _Stored_Data.manufacture_month = 0;
        _Stored_Data.runtime_hours = 0; 
        for(int i = 0; i < 15; i++){
            _Stored_Data.firmware_version[i] = 0;
        }
        _Stored_Data.firmware_version[15] = '\0';
        _Stored_Data.current_calibration_enabled = NULL;
        _Stored_Data.actual_capacity_Ah = 0;
        for(int i = 0; i < 24; i++){
            _Stored_Data.manufacturer_id[i] = 0;
        }
        _Stored_Data.manufacturer_id[24] = '\0';
        _Stored_Data.protocol_version_number = 0;
    } 

void JikongMessenger::begin(uint32_t baudRate = 115200){
    //initilize connection 
    _serial->begin(baudRate, SERIAL_8N1);
    //clear existing data for new serial instance 
    _clearData(); 

    // clear RX buffer 
    while (_serial->available()) { 
        _serial->read();
    }
}

uint16_t JikongMessenger::_getChecksum(byte* frame, uint8_t len){
    uint32_t sum = 0;
    for(int i = 0; i < len; i++){
        sum += frame[i]; 
    }
    return sum;
}
  
void JikongMessenger::request_data(){
    
    byte ReadAllCommand[] = { // Raw bytes that form the "Send All command" from Jikong documentation
        0x4E, 0x57, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 
        0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x68, 0x00, 0x00, 0x01, 0x29
    };
    uint8_t length = 21; // length of "Send All" command

    Dynamic_Array JKreply;
    _sendFrame(JKreply, ReadAllCommand, length);
    _parseData(JKreply.body, JKreply.index);
    free(JKreply.body); // clear memory allocated for BMS response
} 

void JikongMessenger::_sendFrame(Dynamic_Array &reply, byte *msg, uint8_t len){

  while (_serial->available()){ //clear RX buffer 
    _serial->read();
  }
  _serial->write(msg, len); // send message frame to the BMS
 
    reply.index = 0;  // set response length to 0
    uint8_t buffer[520]; 
    
    unsigned long startTime = millis(); // set timeout parameters 

    while ((millis() - startTime) < _timeout) {
        while(_serial->available()){
            uint8_t byte = _serial->read();
            if (reply.index < sizeof(buffer)){
                buffer[reply.index++] = byte;
                if(buffer[reply.index - 4] == 0x68){
                    uint16_t checksum = (uint16_t)buffer[reply.index] << 8 | (uint16_t)buffer[reply.index - 1]; 
                    uint16_t length = (uint16_t)buffer[2] <<8 | (uint16_t)buffer[3];
                   if(checksum == _getChecksum(buffer, reply.index) && length == reply.index){break;}
                }
            } 
            // 
            else {
                break; // buffer overflow protection 
            }
            startTime = millis(); // reset timer on activity 
            }
        // yield to ESP32 RTOS / watchdog
        delay(1);
    }
    reply.body = (byte*)malloc(reply.index);

    if (!reply.body){return;} // check for failed memory allocation 
    memcpy(reply.body, buffer, reply.index);
}


void JikongMessenger::_parseData(uint8_t* message, uint16_t length){
    for(uint16_t i = 11; i < length - 9;){
    uint8_t byte = message[i];
    Handler handler = _dispatchTable[byte]; 

        if(handler){
            (this ->*handler)(message, i, length);
        }
        else{
             i++;
        }
    }    
}
