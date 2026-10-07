#include "Jikong_Handler.h"

//0x79, Individual Cell voltage, size = 3*n 
//The first byte is the cell number, the next two bytes is the voltage value MV. 
//When reading all the data at the same time, 0x79 is followed by one byte length data, n as shown above, and then a group of three 
//bytes represents the Cell voltage.
void JikongMessenger::_cell_voltage_mV(uint8_t* msg, uint16_t& index, uint16_t len){
    uint8_t fieldLen = msg[index+1]; // get field length
    uint8_t reportedCells = fieldLen/3; 
    if(index + fieldLen + 2 >= len) return;
    //checks 
    if(index + 1 >= len){return;}
    if(fieldLen % 3 != 0){return;}
    if(reportedCells != _numCells){Serial.print("Check Balance Leads. ");}

    for(int i = 0; i < reportedCells; i ++){
        if (i >= _numCells){break;}
        uint8_t j = index + 2 + i*3; //j =0, 5, 8, 11, 14.. 

            if(index + 2 + j + 2 >= len) break;{

                if((uint8_t)msg[j] >= 0 && (uint8_t)msg[j] <= _numCells){
                    _cells[i].cellNum = (uint8_t)msg[j];}
        
                _cells[i].cellVoltage = ((uint32_t)msg[j+1] << 8)|((uint32_t)msg[j+2]);
            }
    }
    index += fieldLen + 2;
}
        /* ---------------- Temperatures ---------------- */
//0x80, Read power tube temperature, 2 bytes 0-140 (-40 to 100) The part exceeding 100 is negative temperature, such as 101 is negative 1 degree (100 Benchmark)
void JikongMessenger::_handle_power_tube_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    // Big-endian assembly
    int16_t ptTemp = (int16_t)(((uint16_t)msg[index + 1] << 8) | (uint16_t)msg[index + 2]);

    if(ptTemp > 100){
        ptTemp = (ptTemp*(-1)) + 100; 
    }
    _Stored_Data.power_tube_temp_dC =ptTemp; 
    
    index += 3;
}  
// 0x81, Read the temperature in the battery box, 2 Bytes, 0-140 (-40 to 100) The part exceeding 100 is negative temperature, the same as above (100 reference) |
void JikongMessenger::_handle_battery_box_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    int16_t bbTemp = (int16_t)(((uint16_t)msg[index + 1] << 8) | (uint16_t)msg[index + 2]); // Big-endian assembly
    if (bbTemp > 100){bbTemp = (bbTemp*(-1)) + 100;}
    _Stored_Data.battery_box_temp_dC = bbTemp; 
    index += 3;
}    

// 0x82, Read battery temperature, 2 Bytes, 0-140 (-40 to 100) The part exceeding 100 is negative temperature, the same as above (100 reference)
void JikongMessenger::_handle_battery_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    int16_t bTemp = (int16_t)(((uint16_t)msg[index + 1] << 8) | (uint16_t)msg[index + 2]); 
    if(bTemp > 100){bTemp = (bTemp*(-1)) + 100;}
    _Stored_Data.battery_temp_dC = bTemp; 
    index += 3;   
}   

/* ---------------- Electrical ---------------- */

//0x83, Total battery voltage, 2 Bytes, 0.01V 3500*0.01=35.00v minimum unit 10mV
void JikongMessenger::_handle_total_voltage_mV(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.total_voltage_mV = ((uint32_t)msg[index + 1] << 8)|((uint32_t)msg[index + 2]);  
    index += 3;
}  

//0x84, Current data, 2 Bytes, 10000 (10000-11000)*0.01=-10.00 A (discharge) (10000-9500)*0.01=5.00 A (charging) Accuracy 10 mA unit: 0.01 A Note: C0:0x01 redefine 0x84 current data, the unit is 10 mA, the highest bit is 0 Means discharging
// 1 means charging If discharging 20 A, the data transmitted will be 2000 (0x07D0) If charging 20 A, the transmission data is 34768 (0x87D0) |
void JikongMessenger::_handle_current_dA(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    int16_t i_mA = (int16_t)((uint16_t)msg[index + 1] << 8)|((uint16_t)msg[index + 2]);
    i_mA -= 10000;
    _Stored_Data.current_dA = i_mA*(-1);  
    index += 3;
}         
// 0x85, Battery remaining capacity, 1 Byte, SOC, 0-100% 
void JikongMessenger::_handle_remaining_capacity_pct(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 1 >= len) return;
    _Stored_Data.remaining_capacity_pct = (uint16_t)(msg[index +1]);
    index += 2;  
}   
//0x86, Number of battery temperature sensors, 1 Byte, Two battery temperature sensors      
void JikongMessenger::_temp_sensor_count_n(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 1 > len) return;
    _Stored_Data.temp_sensor_count_n = (uint8_t)(msg[index + 1]);
    index += 2; 
}  
//0x87, Number of battery cycles, 2 Bytes      
void JikongMessenger::_handle_cycle_count(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.cycle_count = ((uint16_t)msg[index + 1] << 8)|((uint16_t)msg[index + 2]); 
    index += 3;
    
}    
// 0x89, Total battery cycle capacity, 4 bytes, Anshi ?? query further, little info
void JikongMessenger::_handle_total_cycle_capacity_Ah(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 4 >= len) return;
    _Stored_Data.total_cycle_capacity_Ah = uint64_t(((uint64_t)msg[index + 1] << 32)|((uint64_t)msg[index + 2]<<16)|((uint64_t)msg[index + 3]<<8)|((uint64_t)msg[index + 4]));
    index += 5;
}         
// 0x8a, Total number of battery strings, 2 Bytes
void JikongMessenger::_handle_string_count_n(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.string_count_n = ((uint8_t)msg[index + 1] << 8)|((uint8_t)msg[index + 2]);
    index += 3;
}    

/* ---------------- Status & Warnings ---------------- */
// 0x8b | Battery warning message, 2 Bytes, Bitmask 
void JikongMessenger::_handle_warning_flags(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _clearWarningFlags();
    uint16_t field = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    for(int i = 0; i < 13; i++){
        uint16_t mask = field & (1u << i);
        switch(mask){
        
        case 1: //0
            _Stored_Data.warning_flags.low_capacity = true;
            break;

        case 2: //1
            _Stored_Data.warning_flags.MOS_tube_OT = true;
            break;

        case 4: //2
            _Stored_Data.warning_flags.charging_OV = true;
            break;

        case 8: //3
            _Stored_Data.warning_flags.discharge_UV = true;
            break;

        case 16: //4
            _Stored_Data.warning_flags.battery_OT = true;
            break;

        case 32: //5
            _Stored_Data.warning_flags.charging_OC = true;
            break;

        case 64: //6
            _Stored_Data.warning_flags.discharge_OC = true;
            break;

        case 124: //7
            _Stored_Data.warning_flags.Cell_pressure_differential = true;
            break;

        case 256: //8
            _Stored_Data.warning_flags.BB_OT = true;
            break;

        case 512: //9
            _Stored_Data.warning_flags.battery_low_temp = true;
            break;
            
            break;
        case 1024: //10
            _Stored_Data.warning_flags.monomer_OV = true;
            break;

            break;
        case 2048: //11
            _Stored_Data.warning_flags.monomer_UV = true;
            break;

        
            break;
        case 4096: //12
            _Stored_Data.warning_flags.protection_309A = true;
            
            break;
        case 8192: //13
            _Stored_Data.warning_flags.protection_309A = true;
            break;
            
        }
    }
    index += 3;
}    

//0x8C, 2 Bytes, Bitmask
void JikongMessenger::_handle_status_flags(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _clearStatusFlags();
    uint16_t field = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    //0
    if(field & (1u) == 1){_Stored_Data.status_flags.charging_MOS_status = true;} //1: Charging MOSFET ON, 0: Charging MOSFET OFF
    else{_Stored_Data.status_flags.charging_MOS_status = false;}
    //1
    if(field & (1u << 1) == 2){_Stored_Data.status_flags.discharge_MOS_status = true;} //1: Discharge MOSFET ON, 0: Discharge MOSFET OFF
    else{_Stored_Data.status_flags.discharge_MOS_status = false;}
    //2
    if(field & (1u << 2) == 4){_Stored_Data.status_flags.balance_switch_state = true;} //1=Balance switch  ON, 0=Balance switch 0 OFF
    else{_Stored_Data.status_flags.balance_switch_state = false;}
    //3
    if(field & (1u << 3) == 8){_Stored_Data.status_flags.battery_conn_status = true ;}//1: Battery connected / normal, 0: Battery offline / disconnected 
    else{_Stored_Data.status_flags.battery_conn_status = false;}
    
    index += 3; 
}         

/* ---------------- Voltage Protections ---------------- */
// 0x8e, Total voltage overvoltage protection, 2 Bytes, 1000-15000 (10 mV) Minimum unit 10mV
void JikongMessenger::_handle_pack_overvoltage_mV(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.pack_overvoltage_mV = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}  
// 0x8f, Total voltage undervoltage protection, 2 Bytes, 1000-15000 (10 mV) Minimum unit 10mV      
void JikongMessenger::_handle_pack_undervoltage_mV(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.pack_undervoltage_mV = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3; 
}         
//0x90, Single overvoltage protection voltage,  2 Bytes, 1000-4500 mV
void JikongMessenger::_handle_cell_overvoltage_mV(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.cell_overvoltage_mV = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}    
//0x91, Cell overvoltage recovery voltage, 2 Bytes, 1000-4500 mV     
void JikongMessenger::_handle_cell_overvoltage_recovery_mV(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.cell_overvoltage_recovery_mV = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}   
// 0x92, Single overvoltage protection delay, 2 Bytes, 1-60 seconds     
void JikongMessenger::_handle_cell_overvoltage_delay_s(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.cell_overvoltage_delay_s = (uint16_t)(msg[index + 1] << 8 | msg[index + 2]);
    index += 3;
}   
//0x93, Single undervoltage protection voltage, 2 Bytes, 1000-4500 mV     
void  JikongMessenger::_handle_cell_undervoltage_mV(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    //Serial.println(msg[index +1]);Serial.println(msg[index +2]);
    _Stored_Data.cell_undervoltage_mV = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3; 
}  
//0x94, Monomer undervoltage recovery voltage. 2 Bytes, 1000-4500 mV        
void JikongMessenger::_handle_Monomer_undervoltage_release_mV(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.Monomer_undervoltage_release_mV = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
    
}    
//0x95, Single undervoltage protection delay, 2 Bytes, 1-60 seconds
void JikongMessenger::_handle_cell_undervoltage_delay_s(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.cell_undervoltage_delay_s = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}         
//0x96, Cell pressure difference protection value 2 Bytes, 0-1000 mV
void JikongMessenger::_handle_cell_pressure_diff_protection_mV(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.cell_pressure_diff_protection_mV = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}  

/* ---------------- Current Protections ---------------- */
//0x97, Discharge overcurrent protection value, 2 Bytes, 1-1000 A
void JikongMessenger::_handle_discharge_overcurrent_A(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.discharge_overcurrent_A = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}
//0x98, Discharge overcurrent delay, 2 Bytes, 1-60 seconds      
void JikongMessenger::_handle_discharge_overcurrent_delay_s(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.discharge_overcurrent_delay_s = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3; 
}         
//0x99, Charging overcurrent protection value, 2 Bytes, 1-1000 A
void JikongMessenger::_handle_charge_overcurrent_A(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.charge_overcurrent_A = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3; 

}    
//0x9a, Charge overcurrent delay, 2 Bytes, 1-60 seconds     
void JikongMessenger::_handle_charge_overcurrent_delay_s(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.charge_overcurrent_delay_s = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3; 
}

/* ---------------- Balancing ---------------- */
//0x9b, Balanced starting voltage, 2 Bytes 2000-4500 mV
void JikongMessenger::_handle_balance_start_voltage_mV(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.balance_start_voltage_mV = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}  
//0x9c, Balanced opening pressure difference, 2 Bytes 10-1000 mV       
void JikongMessenger::_handle_balance_diff_mV(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.balance_diff_mV = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}   
//0x9d, Active balance switch, 1 Byte, 0 (off), 1 (on)     
void JikongMessenger::_handle_active_balance_enabled(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.active_balance_enabled = (bool)(msg[index + 1]);
    index += 2; 
}         

/* ---------------- Temperature Protections ---------------- */
// 0x9e, Power tube temperature protection value, 2 Bytes, 0-100
void JikongMessenger::_handle_Power_tube_temp_protection_value_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.Power_tube_temp_protection_value_dC = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
} 
//0x9f, Power tube temperature recovery value, 2 Bytes, 0-100        
void JikongMessenger::_handle_Power_tube_temp_recovery_value_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.Power_tube_temp_recovery_value_dC = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}  
//0xa0, Temperature protection value in the battery box, 2 Bytes, 40-100
void JikongMessenger::_handle_box_high_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.box_high_temp_dC = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}  
//0xa1, Temperature recovery value in the battery box, 2 Bytes, 40-100      
void JikongMessenger::_handle_box_temp_recovery_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.box_temp_recovery_dC = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
} 
//0xa2, Battery temperature difference protection value, 2 Bytes, 5-20
void JikongMessenger::_battery_temp_diff_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.battery_temp_diff_dC = (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}     
//0xa3, Battery charging high temperature protection value, 2 Bytes, 0-100
void JikongMessenger::_charge_high_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.charge_high_temp_dC =  (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3; 

} 
//0xa4, Battery discharge high temperature protection value, 2 Bytes, 0-100      
void JikongMessenger::_discharge_high_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.discharge_high_temp_dC =  (uint16_t)(msg[index + 1] << 8)|(msg[index + 2]);
    index += 3;
}
//0xa5, Charging low temperature protection value, 2 Bytes, -45 /+25 (no reference-signed data)
void JikongMessenger::_charge_low_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    uint16_t raw =(uint16_t(msg[index + 1]) << 8) |uint16_t(msg[index + 2]);
    _Stored_Data.charge_low_temp_dC = float(raw);
    index += 3;
} 
//0xa6, Charging low temperature protection recovery value, 2 Bytes, -45 /+25 (no reference-signed data)
void JikongMessenger::_charge_low_temp_recovery_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    uint16_t raw =(uint16_t(msg[index + 1]) << 8) | uint16_t(msg[index + 2]);
    _Stored_Data.charge_low_temp_recovery_dC = float(raw);
    index += 3;
} 
//0xa7, Discharge low temperature protection value, 2 bytes, -45 /+25 (no reference-signed data)
void JikongMessenger::_discharge_low_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    uint16_t raw =(uint16_t(msg[index + 1]) << 8) |uint16_t(msg[index + 2]);
    _Stored_Data.discharge_low_temp_dC = float(raw);
    index += 3;   
} 
//0xa8, Discharge low temperature protection recovery value, 2 Bytes, -45 /+25 (no reference-signed data)
void JikongMessenger::_discharge_low_temp_recovery_dC(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    uint16_t raw = (uint16_t)msg[index + 1] << 8 | (uint16_t)msg[index + 2];
    _Stored_Data.discharge_low_temp_recovery_dC = float(raw);
    index += 3;
} 
//0xa9, Battery string setting, 1 Byte, 3-32
void JikongMessenger::_battery_string_setting(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 1 >= len) return;
    _Stored_Data.battery_string_setting = uint8_t(msg[1]); 
    index += 2;
}                      
//0xaa, Battery capacity setting, 4 Bytes, Ah (Amp Hour) 
void JikongMessenger::_battery_capacity_Ah(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 4 >= len) return;
    _Stored_Data.battery_capacity_setting  = ((uint32_t)msg[index + 1] << 24) | 
                     ((uint32_t)msg[index + 2] << 16) | 
                     ((uint32_t)msg[index + 3] << 8) | 
                     ((uint32_t)msg[index + 4]);
    _Stored_Data.battery_capacity_Ah =_Stored_Data.battery_capacity_setting * ((uint32_t)_Stored_Data.remaining_capacity_pct / 100.0f);
    index += 5;
}
//0xab, Charging MOS tube switch, 1 Bytes, 0 (off), 1 (open)
void JikongMessenger::_charge_mos_enabled(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 1 >= len) return;
    _Stored_Data.charge_mos_enabled = (bool)msg[index + 1];
    index += 2;
} 
//0xac, Discharge MOS tube switch, 1 Bytes, 0 (off), 1 (open)
void JikongMessenger::_discharge_mos_enabled(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 1 >= len) return;
    _Stored_Data.discharge_mos_enabled = (bool)msg[index + 1];
    index += 2; 
}
//0xad, Current calibration, 2 Bytes, 100-20000 mA
void JikongMessenger::_current_calibration_offset(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.current_calibration_offset = (uint16_t)((msg[index + 1] << 8) | msg[index + 2]);
    index += 3;
} 
//0xae, Protection board address, 1 Byte, is reserved for use when cascading
void JikongMessenger::_P_board_address(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 1 >= len) return;
    _Stored_Data.P_board_address = (uint8_t)msg[index + 1];
    index += 2;
} 
//0xaf, Battery Type, 1 Byte, 0 (lithium iron phosphate), 1 (ternary), 2 (lithium titanate)
void JikongMessenger::_battery_type(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 1 >= len) return;
    _Stored_Data.battery_type; //*char
    switch ((uint8_t)msg[index + 1])
    {
    case 0:
        _Stored_Data.battery_type = "lithium iron phosphate";
        break;

    case 1:
        _Stored_Data.battery_type = "ternary";
        break;

    case 2:
        _Stored_Data.battery_type = "lithium titanate";
        break;

    default:
        _Stored_Data.battery_type = "unkown cell type";
        break;
    }
    index += 2;
}
//RW, 0xb0, Sleep waiting time, 2 Bytes, second data, temporarily for reference. 
void JikongMessenger::_sleep_delay_s(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 2 >= len) return;
    _Stored_Data.sleep_delay_s = (uint16_t)((msg[index + 1] << 8) | msg[index + 2]);
    index += 3;

}
//0xb1, Low volume alarm value, 1 Byte, 0-80 %
void JikongMessenger::_low_cap_alarmVol_pct(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 1 > len) return;
    _Stored_Data.low_cap_alarmVol_pct = (uint8_t)msg[index + 1];
    index += 2;
}
//0xb2, Modify parameter password, 10 Bytes, is temporarily used as a reference, fix a password.
void JikongMessenger::_parameter_password(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 10 >= len) return;
    memcpy(_Stored_Data.parameter_password, &msg[index + 1], 10);
    _Stored_Data.parameter_password[9] = '\0';
    index += 11;
} 
//0xb3, Dedicated charger switch, 1 Byte, 0 (off), 1 (on)
void JikongMessenger::_dedicated_charger_enabled(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 1 >= len) return;
    _Stored_Data.dedicated_charger_enabled = (bool)msg[index + 1]; 
    index += 2;
}
//0xb4, Device ID code, 8 Bytes, Example 60300001 (60-nominal voltage level: defined by the voltage level, for example, 60 is 60V. Series 48 is 48V series; 3-material system: according to the system definition of battery materials such as iron. 
//Lithium code is 1 manganic acid code 2 ternary code 3; 00001-production serial number: according to manufacturing. The Nth group of the model produced by the manufacturer that month is numbered N (for example: a certain type The first group of the number, then N is 00001)) characters
void JikongMessenger::_device_id(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 8 >= len) return;
    uint32_t dID; 

    for (int i = 0; i < 8; i++){
        dID = (dID << 8) | msg[index + i];
    }
    _Stored_Data.device_id = dID;
    index += 9;  
}
//0xb5, Date of manufacture, 4 Bytes, first 2 bytes give year (append onto 20) second two bytes give month. 
void JikongMessenger::_manufacture_date(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 4 >= len) return;
    _Stored_Data.manufacture_year = 2000 + (uint16_t)(msg[index +1] - '0')*10 + ((uint16_t)(msg[index + 2]) - '0');
    _Stored_Data.manufacture_month = ((uint8_t)(msg[index +3]) - '0')*10 + ((uint8_t)(msg[index + 4]) - '0'); 
    index += 5;
} 
//0xb6, System working hours, 4 Bytes, is cleared when leaving factory, unit minute 
void JikongMessenger::_runtime_hours(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 4 >= len) return;
    _Stored_Data.runtime_hours = 
    (uint32_t)(msg[index + 1] << 24) |
    (uint32_t)(msg[index + 2] << 16) |
    (uint32_t)(msg[index + 3] << 8) |
    (uint32_t)(msg[index + 4]);
    index += 5;
} 
//0xb7, Software version number, 15 Bytes, NW_1_0_0_200428
void JikongMessenger::_firmware_version(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 15 >= len) return;
    memcpy(_Stored_Data.firmware_version, &msg[index + 1], 15);
    _Stored_Data.firmware_version[15] = '\0';
    index += 16;
} 
//0xb8, Whether to start current calibration, 1 Byte, 1 (Start calibration) 0 (Close calibration)
void JikongMessenger::_current_calibration_enabled(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 1 >= len) return;
    _Stored_Data.current_calibration_enabled = (bool)(msg[index + 1]);
    index += 2;
} 
//0xb9, Actual battery capacity, 4 Bytes, Ah (Amp Hour)
void JikongMessenger::_actual_capacity_Ah(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 4 >= len) return;
    _Stored_Data.actual_capacity_Ah =
    ((uint32_t)msg[index + 1] << 24) |
    ((uint32_t)msg[index + 2] << 16) |
    ((uint32_t)msg[index + 3] << 8)  |
    ((uint32_t)msg[index + 4]);
    index += 5;
} 
//0xba, Manufacturer ID naming, 24 Bytes, Bitmask
void JikongMessenger::_manufacturer_id(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 24 >= len) return;
    char buffer[25];
    memcpy(_Stored_Data.manufacturer_id, &msg[index + 1], 24);
    _Stored_Data.manufacturer_id[24] = '\0'; 
    index += 25;
}
//0xc0, Protocol version number, 1 byte, Default value: 0x00 0x01: Redefine 0x84 current data, the unit is 10 mA, and the highest bit is 0 for discharging. 1 means charging If discharging 20A, the data transmitted will be 2000 (0x07D0) If charging 20A, the transmission data is 34768 (0x87D0) |
void JikongMessenger::_protocol_version_number(uint8_t* msg, uint16_t& index, uint16_t len){
    if (index + 1 >= len) return;
    _Stored_Data.protocol_version_number = (uint8_t)msg[index + 1];
    index += 2;
} 