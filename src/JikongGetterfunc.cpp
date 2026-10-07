#include "Jikong_Handler.h"

JikongMessenger::Cell_Voltages* JikongMessenger::get_cell_voltage_mV() const{
  return _Stored_Data.cell_voltage_mV;
}

// ---- Temperatures ----
int16_t JikongMessenger::get_power_tube_temp_dC(){
  return _Stored_Data.power_tube_temp_dC ? _Stored_Data.power_tube_temp_dC : 0;

}

int16_t JikongMessenger::get_battery_temp_dC(){
  return _Stored_Data.battery_temp_dC ? _Stored_Data.battery_temp_dC : 0;

}

int16_t JikongMessenger::get_battery_box_temp_dC(){
  return _Stored_Data.battery_box_temp_dC ? _Stored_Data.battery_box_temp_dC : 0;

}

    // ---- Electrical ----
    uint16_t JikongMessenger::get_total_voltage_mV(){
      return _Stored_Data.total_voltage_mV ? _Stored_Data.total_voltage_mV : 0;

    }
    int16_t JikongMessenger::get_current_dA(){
      return _Stored_Data.current_dA ? _Stored_Data.current_dA : 0;

    }
    uint8_t JikongMessenger::get_remaining_capacity_pct(){
      return _Stored_Data.remaining_capacity_pct ? _Stored_Data.remaining_capacity_pct : 0;

    }
    uint8_t JikongMessenger::get_temp_sensor_count_n(){
      return _Stored_Data.temp_sensor_count_n ? _Stored_Data.temp_sensor_count_n : 0;

    }

    uint16_t JikongMessenger::get_cycle_count(){
      return _Stored_Data.cycle_count ? _Stored_Data.cycle_count : 0;

    }
    uint64_t JikongMessenger::get_total_cycle_capacity_Ah(){
      return _Stored_Data.total_cycle_capacity_Ah ? _Stored_Data.total_cycle_capacity_Ah : 0;

    }
    uint8_t JikongMessenger::get_string_count_n(){
      return _Stored_Data.string_count_n ? _Stored_Data.string_count_n : 0;

    }

    // ---- Status & warnings ----
    const JikongMessenger::Warning_Flags* JikongMessenger::get_warning_flags() const{
      return &_Stored_Data.warning_flags;
    }
// converts func to retunr const Status_Flags*
    const JikongMessenger::Status_Flags* JikongMessenger::get_status_flags() const{
      return &_Stored_Data.status_flags;
    }

    // ---- Voltage protections ----
    uint16_t JikongMessenger::get_pack_overvoltage_mV(){
      return _Stored_Data.pack_overvoltage_mV;

    }
    uint16_t JikongMessenger::get_pack_undervoltage_mV(){
      return _Stored_Data.pack_undervoltage_mV;

    }
    uint16_t JikongMessenger::get_cell_overvoltage_mV(){
      return _Stored_Data.cell_overvoltage_mV;

    }
    uint16_t JikongMessenger::get_cell_overvoltage_recovery_mV(){
      return _Stored_Data.cell_overvoltage_recovery_mV; 

    }
    uint16_t JikongMessenger::get_cell_overvoltage_delay_s(){
      return _Stored_Data.cell_overvoltage_delay_s;

    }
    uint16_t JikongMessenger::get_cell_undervoltage_mV(){
      return _Stored_Data.cell_undervoltage_mV ? _Stored_Data.cell_undervoltage_mV : 0;

    }
    uint16_t JikongMessenger::get_monomer_undervoltage_release_mV(){
      return _Stored_Data.Monomer_undervoltage_release_mV ? _Stored_Data.Monomer_undervoltage_release_mV : 0;

    }
    uint16_t JikongMessenger::get_cell_undervoltage_delay_s(){
      return _Stored_Data.cell_undervoltage_delay_s ? _Stored_Data.cell_undervoltage_delay_s : 0;

    }
    uint16_t JikongMessenger::get_cell_pressure_diff_protection_mV(){
      return _Stored_Data.cell_pressure_diff_protection_mV? _Stored_Data.cell_pressure_diff_protection_mV : 0;

    }

    // ---- Current protections ----
    uint16_t JikongMessenger::get_discharge_overcurrent_A(){
      return _Stored_Data.discharge_overcurrent_A ? _Stored_Data.discharge_overcurrent_A : 0;
    }
    uint16_t JikongMessenger::get_discharge_overcurrent_delay_s(){
      return _Stored_Data.discharge_overcurrent_delay_s ? _Stored_Data.discharge_overcurrent_delay_s : 0;

    }
    uint16_t JikongMessenger::get_charge_overcurrent_A(){
      return _Stored_Data.charge_overcurrent_A ? _Stored_Data.charge_overcurrent_A : 0;

    }
    uint16_t JikongMessenger::get_charge_overcurrent_delay_s(){
      return _Stored_Data.charge_overcurrent_delay_s ? _Stored_Data.charge_overcurrent_delay_s : 0;

    }

    // ---- Balancing ----
    uint16_t JikongMessenger::get_balance_start_voltage_mV(){
      return _Stored_Data.balance_start_voltage_mV ? _Stored_Data.balance_start_voltage_mV : 0;

    }
    uint16_t JikongMessenger::get_balance_diff_mV(){
      return _Stored_Data.balance_diff_mV ? _Stored_Data.balance_diff_mV : 0;

    }
    bool JikongMessenger::is_active_balance_enabled(){
      return _Stored_Data.active_balance_enabled;
    }


    // ---- Temperature protections ----
    uint16_t JikongMessenger::get_power_tube_temp_protection_value_dC(){
      return _Stored_Data.Power_tube_temp_protection_value_dC ? _Stored_Data.Power_tube_temp_protection_value_dC : 0;

    }
    uint16_t JikongMessenger::get_power_tube_temp_recovery_value_dC(){
      return _Stored_Data.Power_tube_temp_recovery_value_dC ? _Stored_Data.Power_tube_temp_recovery_value_dC : 0;

    }
    uint16_t JikongMessenger::get_box_high_temp_dC(){
      return _Stored_Data.box_high_temp_dC ? _Stored_Data.box_high_temp_dC : 0;

    }
    uint16_t JikongMessenger::get_box_temp_recovery_dC(){
      return _Stored_Data.box_temp_recovery_dC ? _Stored_Data.box_temp_recovery_dC : 0;

    }
    uint16_t JikongMessenger::get_battery_temp_diff_dC(){
      return _Stored_Data.battery_temp_diff_dC ? _Stored_Data.battery_temp_diff_dC : 0;

    }
    uint16_t JikongMessenger::get_charge_high_temp_dC(){
      return _Stored_Data.charge_high_temp_dC ? _Stored_Data.charge_high_temp_dC: 0;

    }
    int16_t JikongMessenger::get_discharge_high_temp_dC(){
      return _Stored_Data.discharge_high_temp_dC ? _Stored_Data.discharge_high_temp_dC : 0;

    }
    int16_t JikongMessenger::get_charge_low_temp_dC(){
      return _Stored_Data.charge_low_temp_dC ? _Stored_Data.charge_low_temp_dC : 0;

    }
    float JikongMessenger::get_charge_low_temp_recovery_dC(){
      return _Stored_Data.charge_low_temp_recovery_dC ? _Stored_Data.charge_low_temp_recovery_dC : 0;

    }
    int16_t JikongMessenger::get_discharge_low_temp_dC(){
      return _Stored_Data.discharge_low_temp_dC ? _Stored_Data.discharge_low_temp_dC : 0;

    }
    int16_t JikongMessenger::get_discharge_low_temp_recovery_dC(){
      return _Stored_Data.discharge_low_temp_recovery_dC ? _Stored_Data.discharge_low_temp_recovery_dC : 0;

    }

    // ---- Configuration ----//
    uint32_t JikongMessenger::get_battery_string_setting(){
      return _Stored_Data.battery_string_setting ? _Stored_Data.battery_string_setting : 0;
    }
    uint32_t JikongMessenger::get_battery_capacity_setting(){
      return _Stored_Data.battery_capacity_setting ? _Stored_Data.battery_capacity_setting : 0;
    }
    uint32_t JikongMessenger::get_battery_capacity_Ah(){
      return _Stored_Data.battery_capacity_Ah ? _Stored_Data.battery_capacity_Ah : 0;
    }
    bool JikongMessenger::is_charge_mos_enabled(){
      return _Stored_Data.charge_mos_enabled;
    }
    bool JikongMessenger::is_discharge_mos_enabled(){
      return _Stored_Data.discharge_mos_enabled;
    }
    uint16_t JikongMessenger::get_current_calibration_offset(){
      return _Stored_Data.current_calibration_offset ? _Stored_Data.current_calibration_offset : 0;
    }
    uint8_t JikongMessenger::get_P_board_address(){
      return _Stored_Data.P_board_address ? _Stored_Data.P_board_address : 0;
    }
    const char* JikongMessenger::get_battery_type() const{
      return _Stored_Data.battery_type && _Stored_Data.battery_type[0] != '\0' ? _Stored_Data.battery_type : NULL;
    }
    uint16_t JikongMessenger::get_sleep_delay_s(){
      return _Stored_Data.discharge_low_temp_recovery_dC ? _Stored_Data.discharge_low_temp_recovery_dC : 0;
    }
    uint8_t JikongMessenger::get_low_cap_alarmVol_pct(){
      return _Stored_Data.low_cap_alarmVol_pct ? _Stored_Data.low_cap_alarmVol_pct : 0;
    }
    const char* JikongMessenger::get_parameter_password () const{
      return _Stored_Data.parameter_password && _Stored_Data.parameter_password[0] != '\0' ? _Stored_Data.parameter_password : nullptr;
    }
    bool JikongMessenger::is_dedicated_charger_enabled(){
      return _Stored_Data.dedicated_charger_enabled;

    }
    uint64_t JikongMessenger::get_device_id(){
      return _Stored_Data.device_id ? _Stored_Data.device_id : 0;

    }
    uint16_t JikongMessenger::get_manufacture_year(){
      return _Stored_Data.manufacture_year ? _Stored_Data.manufacture_year : 0;

    }
    uint8_t JikongMessenger::get_manufacture_month(){
      return _Stored_Data.manufacture_month ? _Stored_Data.manufacture_month : 0;
    }
    uint32_t JikongMessenger::get_runtime_hours(){
      return _Stored_Data.runtime_hours ? _Stored_Data.runtime_hours : 0;
    }
    const char* JikongMessenger::get_firmware_version() const{
      return _Stored_Data.firmware_version && _Stored_Data.firmware_version[0] != '\0' ? _Stored_Data.firmware_version : nullptr;
    }
    bool JikongMessenger::is_current_calibration_enabled(){
      return _Stored_Data.current_calibration_enabled;
    }
    uint32_t JikongMessenger::get_actual_capacity_Ah(){
      return _Stored_Data.actual_capacity_Ah ? _Stored_Data.actual_capacity_Ah : 0;
    }
    const char* JikongMessenger::get_manufacturer_id() const{
      return (_Stored_Data.manufacturer_id[0] != '\0') ? _Stored_Data.manufacturer_id : nullptr;
    }
    uint8_t JikongMessenger::get_protocol_version_number(){
      return _Stored_Data.protocol_version_number ? _Stored_Data.protocol_version_number : 0;     
    }