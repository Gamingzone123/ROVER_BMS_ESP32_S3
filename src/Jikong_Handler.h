#pragma once

#define Jikong_Handler_H
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

class JikongMessenger{
  
  // define data structure to hold unprocessed messages from the BMS
  typedef struct{
    uint16_t index;
    byte *body;
  }Dynamic_Array;

public:

  // define a data structure to hold data from the Warning Flags field in the BMS reply frame, 1 (alarm), 0 (normal)
    typedef struct{
    bool low_capacity;       
    bool MOS_tube_OT;
    bool charging_OV; 
    bool discharge_UV; 
    bool battery_OT;
    bool charging_OC;
    bool discharge_OC;
    bool Cell_pressure_differential;
    bool BB_OT;
    bool battery_low_temp;
    bool monomer_OV;          
    bool monomer_UV;         
    bool protection_309A;                                     
    }Warning_Flags;
  
    //define data structure to hold data from the Status Flags field of the BMS reply frame
  typedef struct {
    bool charging_MOS_status; //1: Charging MOSFET ON, 0: Charging MOSFET OFF
    bool discharge_MOS_status; //1: Discharge MOSFET ON, 0: Discharge MOSFET OFF
    bool balance_switch_state;    //1=Balance switch  ON, 0=Balance switch 0 OFF
    bool battery_conn_status; //1: Battery connected / normal, 0: Battery offline / disconnected
  }Status_Flags;

  // define a data structure to index and store the voltage state of each battery cell
  typedef struct{
    uint8_t cellNum;
    uint32_t cellVoltage; 
  }Cell_Voltages;

    // alias for pointers of a generic type that will form the parsing functions 
    using Handler = void (JikongMessenger::*)(uint8_t*, uint16_t&, uint16_t);

    // define class constructor 
    JikongMessenger(HardwareSerial* serial, uint16_t Reply_timeout, uint8_t NumberCells);

    // Initialize the BMS communication over "GPS" port
    void begin(uint32_t baudRate);

    // Set the state of the internal MOSFETS, 0 = OFF, 1 = OFF.
    bool setMOS_state(bool enable_discharge, bool enable_charge);

    // Initiates the process of requesting data from the BMS ans parsing the response
    void request_data();
    
    //======================= Getter Functions ===================================//
      // ---- Cell voltages ----
    Cell_Voltages* get_cell_voltage_mV() const;

    // ---- Temperatures ---- 

    int16_t get_power_tube_temp_dC();
    int16_t get_battery_box_temp_dC();
    int16_t get_battery_temp_dC();

    // ---- Electrical ----

    uint16_t get_total_voltage_mV();
    int16_t get_current_dA();
    uint8_t get_remaining_capacity_pct();
    uint8_t get_temp_sensor_count_n();
    uint16_t get_cycle_count();
    uint64_t get_total_cycle_capacity_Ah();
    uint8_t get_string_count_n();

    // ---- Status & warnings ----

    const Warning_Flags* get_warning_flags() const;
    const Status_Flags* get_status_flags() const;

    // ---- Voltage protections ----

    uint16_t get_pack_overvoltage_mV();
    uint16_t get_pack_undervoltage_mV();
    uint16_t get_cell_overvoltage_mV();
    uint16_t get_cell_overvoltage_recovery_mV();
    uint16_t get_cell_overvoltage_delay_s();
    uint16_t get_cell_undervoltage_mV();
    uint16_t get_monomer_undervoltage_release_mV();
    uint16_t get_cell_undervoltage_delay_s();
    uint16_t get_cell_pressure_diff_protection_mV();

    // ---- Current protections ---- 

    uint16_t get_discharge_overcurrent_A();
    uint16_t get_discharge_overcurrent_delay_s();
    uint16_t get_charge_overcurrent_A();
    uint16_t get_charge_overcurrent_delay_s();

    // ---- Balancing ---- 

    uint16_t get_balance_start_voltage_mV();
    uint16_t get_balance_diff_mV();
    bool is_active_balance_enabled();

    // ---- Temperature protections ----

    uint16_t get_power_tube_temp_protection_value_dC();
    uint16_t get_power_tube_temp_recovery_value_dC();
    uint16_t get_box_high_temp_dC();
    uint16_t get_box_temp_recovery_dC();
    uint16_t get_battery_temp_diff_dC();
    uint16_t get_charge_high_temp_dC();
    int16_t get_discharge_high_temp_dC();
    int16_t get_charge_low_temp_dC();
    float get_charge_low_temp_recovery_dC();
    int16_t get_discharge_low_temp_dC();
    int16_t get_discharge_low_temp_recovery_dC();

    // ---- Configuration ----

    uint32_t get_battery_string_setting();
    uint32_t get_battery_capacity_setting();
    uint32_t get_battery_capacity_Ah();
    bool is_charge_mos_enabled();
    bool is_discharge_mos_enabled();
    uint16_t get_current_calibration_offset();
    uint8_t get_P_board_address();
    const char* get_battery_type() const;
    uint16_t get_sleep_delay_s();
    uint8_t get_low_cap_alarmVol_pct();
    const char* get_parameter_password() const;
    bool is_dedicated_charger_enabled();
    uint64_t get_device_id();
    uint16_t get_manufacture_year();
    uint8_t get_manufacture_month();
    uint32_t get_runtime_hours();
    const char* get_firmware_version() const;
    bool is_current_calibration_enabled();
    uint32_t get_actual_capacity_Ah();
    const char* get_manufacturer_id() const;
    uint8_t get_protocol_version_number();

    private:
    // type declairation for main struct internal to the class that houses the parsed data 
    typedef struct
    {
        /* ---------------- Cell Voltages ---------------- */
        Cell_Voltages* cell_voltage_mV;          // mV per cell (1 mV resolution)
        /* ---------------- Temperatures ---------------- */
        int16_t  power_tube_temp_dC;            // °C ×10
        int16_t  battery_box_temp_dC;           // °C ×10
        int16_t  battery_temp_dC;               // °C ×10

        /* ---------------- Electrical ---------------- */
        uint16_t total_voltage_mV;              // mV
        int16_t  current_dA;                    // A ×10 (signed)
        uint8_t remaining_capacity_pct;     // Ah ×10
        uint8_t  temp_sensor_count_n;             // count
        uint16_t cycle_count;                   // cycles
        uint64_t total_cycle_capacity_Ah;       // Ah
        uint8_t  string_count_n;                  // series strings
        /* ---------------- Status & Warnings ---------------- */
        Warning_Flags warning_flags;                 // bitfield
        Status_Flags status_flags;                  // bitfield
        /* ---------------- Voltage Protections ---------------- */
        uint16_t pack_overvoltage_mV;
        uint16_t pack_undervoltage_mV;
        uint16_t cell_overvoltage_mV;
        uint16_t cell_overvoltage_recovery_mV;
        uint16_t cell_overvoltage_delay_s; 
        uint16_t cell_undervoltage_mV;
        uint16_t Monomer_undervoltage_release_mV;
        uint16_t cell_undervoltage_delay_s;
        uint16_t cell_pressure_diff_protection_mV;

        /* ---------------- Current Protections ---------------- */
        uint16_t discharge_overcurrent_A;
        uint16_t discharge_overcurrent_delay_s;
        uint16_t charge_overcurrent_A;
        uint16_t charge_overcurrent_delay_s;
        /* ---------------- Balancing ---------------- */
        uint16_t balance_start_voltage_mV;
        uint16_t balance_diff_mV;
        bool active_balance_enabled;
        /* ---------------- Temperature Protections ---------------- */
        uint16_t  Power_tube_temp_protection_value_dC;
        uint16_t  Power_tube_temp_recovery_value_dC;
        uint16_t  box_high_temp_dC;
        uint16_t  box_temp_recovery_dC;
        uint16_t  battery_temp_diff_dC;
        uint16_t  charge_high_temp_dC;
        int16_t  discharge_high_temp_dC;
        int16_t  charge_low_temp_dC;
        float  charge_low_temp_recovery_dC;
        int16_t  discharge_low_temp_dC;
        int16_t  discharge_low_temp_recovery_dC;
        /* ---------------- Configuration ---------------- */
        uint32_t  battery_string_setting;
        uint32_t battery_capacity_setting;
        uint32_t battery_capacity_Ah;
        bool charge_mos_enabled;
        bool discharge_mos_enabled;
        uint16_t  current_calibration_offset;
        uint8_t  P_board_address;
        const char*  battery_type;
        uint16_t sleep_delay_s;
        uint8_t low_cap_alarmVol_pct;
        char parameter_password[10];
        bool dedicated_charger_enabled;
        uint64_t device_id;
        uint16_t manufacture_year;
        uint8_t manufacture_month;
        uint32_t runtime_hours;
        char firmware_version[16];
        bool current_calibration_enabled;
        uint32_t actual_capacity_Ah;
        char manufacturer_id[25];
        uint8_t protocol_version_number;
    } JK_BMS_Parsed_t;

    // allocates pointers to parsing functions to memory spaces in the dispatch table
    static void _initHandlers();
    // initilizes data in the warning flags field and sets all flags to false
    void _clearWarningFlags(); 
    // clears data in the parsed data storage struct
    void _clearData();
    // houses and calls the parsing loop for Jikong reply frames 
    void _parseData(uint8_t* message, uint16_t length);
    // initilizes and sets all cell voltages to 0
    void _initCellVoltage_mV();
    // initilizes and clears all the status flags
    void _clearStatusFlags();
    // calculates the check sum of a data frame 
    uint16_t _getChecksum(byte* frame, uint8_t len);
    // sends a data frame the BMS over via serial transmission 
    void _sendFrame(Dynamic_Array &reply, byte *msg, uint8_t len);
    // creates frames to control MOS state
    bool _createMOSframe(byte *command, uint8_t size);
    // checkes if ste MOS state command has been acknowleged by BMS 
    bool _MOScommandACK(uint8_t* msg, uint16_t length);

    //-------------------  Parsing Functions  -------------------------------
    void _cell_voltage_mV(uint8_t* msg, uint16_t& index, uint16_t len);          // voltage of each cell value (1 mV resolution) 

    /* ---------------- Temperatures ---------------- */
    void _handle_power_tube_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len);            // °C ×10
    void _handle_battery_box_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len);           // °C ×10
    void _handle_battery_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len);               // °C ×10

    /* ---------------- Electrical ---------------- */
    void _handle_total_voltage_mV(uint8_t* msg, uint16_t& index, uint16_t len);              // mV
    void _handle_current_dA(uint8_t* msg, uint16_t& index, uint16_t len);                    // A ×10 (signed)
    void _handle_remaining_capacity_pct(uint8_t* msg, uint16_t& index, uint16_t len);           // Ah ×10
    void _temp_sensor_count_n(uint8_t* msg, uint16_t& index, uint16_t len);                     // count
    void _handle_cycle_count(uint8_t* msg, uint16_t& index, uint16_t len);                      // cycles
    void _handle_total_cycle_capacity_Ah(uint8_t* msg, uint16_t& index, uint16_t len);          // Ah
    void _handle_string_count_n(uint8_t* msg, uint16_t& index, uint16_t len);                   // series strings

    /* ---------------- Status & Warnings ---------------- */
    void _handle_warning_flags(uint8_t* msg, uint16_t& index, uint16_t len);                    // bitfield
    void _handle_status_flags(uint8_t* msg, uint16_t& index, uint16_t len);                     // bitfield

    /* ---------------- Voltage Protections ---------------- */
    void _handle_pack_overvoltage_mV(uint8_t* msg, uint16_t& index, uint16_t len);              // mV
    void _handle_pack_undervoltage_mV(uint8_t* msg, uint16_t& index, uint16_t len);             // mV
    void _handle_cell_overvoltage_mV(uint8_t* msg, uint16_t& index, uint16_t len);              //mV
    void _handle_cell_overvoltage_recovery_mV(uint8_t* msg, uint16_t& index, uint16_t len);     //mV
    void _handle_cell_overvoltage_delay_s(uint8_t* msg, uint16_t& index, uint16_t len);         // seconds
    void _handle_cell_undervoltage_mV(uint8_t* msg, uint16_t& index, uint16_t len);             // mV
    void _handle_Monomer_undervoltage_release_mV(uint8_t* msg, uint16_t& index, uint16_t len);  // mV 
    void _handle_cell_undervoltage_delay_s(uint8_t* msg, uint16_t& index, uint16_t len);        //seconds

    void _handle_cell_pressure_diff_protection_mV(uint8_t* msg, uint16_t& index, uint16_t len); //mV 

    /* ---------------- Current Protections ---------------- */
    void _handle_discharge_overcurrent_A(uint8_t* msg, uint16_t& index, uint16_t len);          //Amps
    void _handle_discharge_overcurrent_delay_s(uint8_t* msg, uint16_t& index, uint16_t len);    //Seconds

    void _handle_charge_overcurrent_A(uint8_t* msg, uint16_t& index, uint16_t len);             //Amps
    void _handle_charge_overcurrent_delay_s(uint8_t* msg, uint16_t& index, uint16_t len);       //seconds

    /* ---------------- Balancing ---------------- */
    void _handle_balance_start_voltage_mV(uint8_t* msg, uint16_t& index, uint16_t len);         //mV 
    void _handle_balance_diff_mV(uint8_t* msg, uint16_t& index, uint16_t len);                  //mV
    void _handle_active_balance_enabled(uint8_t* msg, uint16_t& index, uint16_t len);           //true/false 

    /* ---------------- Temperature Protections ---------------- */
    void _handle_Power_tube_temp_protection_value_dC(uint8_t* msg, uint16_t& index, uint16_t len); //degrees celsius 
    void _handle_Power_tube_temp_recovery_value_dC(uint8_t* msg, uint16_t& index, uint16_t len);   //degrees celsius
    void _handle_box_high_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len);                    //degrees celsius
    void _handle_box_temp_recovery_dC(uint8_t* msg, uint16_t& index, uint16_t len);                //degrees celsius 
    void _battery_temp_diff_dC(uint8_t* msg, uint16_t& index, uint16_t len);                       //degrees celsius
    void _charge_high_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len);                        //degrees celsius
    void _discharge_high_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len);                     //degrees celsius 
    void _charge_low_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len);                         //degrees celsius
    void _charge_low_temp_recovery_dC(uint8_t* msg, uint16_t& index, uint16_t len);                //degrees celsius 
    void _discharge_low_temp_dC(uint8_t* msg, uint16_t& index, uint16_t len);                      //degrees celsius
    void _discharge_low_temp_recovery_dC(uint8_t* msg, uint16_t& index, uint16_t len);             //degrees celsius 

    /* ---------------- Configuration ---------------- */
    void _battery_string_setting(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _battery_capacity_Ah(uint8_t* msg, uint16_t& index, uint16_t len);                        //amp hours
    void _charge_mos_enabled(uint8_t* msg, uint16_t& index, uint16_t len);                         // ON/OFF
    void _discharge_mos_enabled(uint8_t* msg, uint16_t& index, uint16_t len);                      //ON/OFF 
    void _current_calibration_offset(uint8_t* msg, uint16_t& index, uint16_t len);                  
    void _P_board_address(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _battery_type(uint8_t* msg, uint16_t& index, uint16_t len);                              
    void _sleep_delay_s(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _low_cap_alarmVol_pct(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _parameter_password(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _dedicated_charger_enabled(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _device_id(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _manufacture_date(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _runtime_hours(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _firmware_version(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _current_calibration_enabled(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _actual_capacity_Ah(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _manufacturer_id(uint8_t* msg, uint16_t& index, uint16_t len); 
    void _protocol_version_number(uint8_t* msg, uint16_t& index, uint16_t len); 

    // defining serial instance used for BMS communcations 
    HardwareSerial* _serial;

    // class variable to store message user reply timeout
    uint8_t _timeout; 
    // dispatch table to hold pointers to the parsing functions
    static Handler _dispatchTable[256];
    // creation instance of internal data structure to house parsed data
    JK_BMS_Parsed_t _Stored_Data;
    // number of cells user has connected via balance leads  
    uint8_t _numCells;
    // maxiumum number of cells possible to connect
    uint8_t _maxCells = 24; 
    // voltage data for each battery cell 
    Cell_Voltages _cells[24];

}; 