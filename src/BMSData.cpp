#include "BMSData.h"

#include <ArduinoJson.h>
#include "Display.h"
#include "settings.h"

extern void setLEDStripColour(LEDStripColourEnum colour);

void getBMSData()
{
    JKMessenger.request_data();

    const auto *warningFlags = JKMessenger.get_warning_flags();
    BMSData.error.clear();

    // map values to labels
    struct WarningEntry
    {
        const char *label;
        bool JikongMessenger::Warning_Flags::*member;
    };

    const WarningEntry warningEntries[] = {
        {"Low capacity", &JikongMessenger::Warning_Flags::low_capacity},
        {"MOS tube overtemp", &JikongMessenger::Warning_Flags::MOS_tube_OT},
        {"Charging overvoltage", &JikongMessenger::Warning_Flags::charging_OV},
        {"Discharge undervoltage", &JikongMessenger::Warning_Flags::discharge_UV},
        {"Battery overtemp", &JikongMessenger::Warning_Flags::battery_OT},
        {"Charging overcurrent", &JikongMessenger::Warning_Flags::charging_OC},
        {"Discharge overcurrent", &JikongMessenger::Warning_Flags::discharge_OC},
        {"Cell pressure differential", &JikongMessenger::Warning_Flags::Cell_pressure_differential},
        {"Battery box overtemp", &JikongMessenger::Warning_Flags::BB_OT},
        {"Battery low temp", &JikongMessenger::Warning_Flags::battery_low_temp},
        {"Cell overvoltage", &JikongMessenger::Warning_Flags::monomer_OV},
        {"Cell undervoltage", &JikongMessenger::Warning_Flags::monomer_UV},
        {"Protection 309A", &JikongMessenger::Warning_Flags::protection_309A},
    };

    for (const auto &entry : warningEntries) // for entry in warningEntries
    {
        if (warningFlags && warningFlags->*entry.member)
        {
            if (!BMSData.error.empty())
            {
                BMSData.error += "; ";
            }
            BMSData.error += entry.label;
        }
    }

    // error checking
    if (!BMSData.error.empty())
    {
        setLEDStripColour(RED_ERROR);
#if DISABLE_DISCHARGE_ON_ERROR
        JKMessenger.setMOS_state(false, false);
#endif
        displayBMSError(BMSData.error);
    }

    BMSData.batteryLife = JKMessenger.get_remaining_capacity_pct();
    const JikongMessenger::Cell_Voltages *cellVoltages = JKMessenger.get_cell_voltage_mV();
    for (size_t i = 0; i < numCells; i++)
    {
        BMSData.cellVoltages[i] = cellVoltages[i].cellVoltage;
    }
    BMSData.currentDraw = JKMessenger.get_current_dA();
    BMSData.MOSStatus[0] = JKMessenger.get_status_flags()->charging_MOS_status;
    BMSData.MOSStatus[1] = JKMessenger.get_status_flags()->discharge_MOS_status;
    BMSData.packTemp = JKMessenger.get_battery_temp_dC();
    BMSData.totalVoltage = JKMessenger.get_total_voltage_mV();
}

String getVerboseBMS()
{
    JKMessenger.request_data();

    JsonDocument document;
    JsonObject measurements = document["measurements"].to<JsonObject>();
    JsonArray cellVoltages = measurements["cellVoltages"].to<JsonArray>();
    const JikongMessenger::Cell_Voltages *cells = JKMessenger.get_cell_voltage_mV();
    if (cells)
    {
        for (size_t i = 0; i < numCells; ++i)
        {
            JsonObject cell = cellVoltages.add<JsonObject>();
            cell["cellNumber"] = cells[i].cellNum;
            cell["voltage_mV"] = cells[i].cellVoltage;
        }
    }
    measurements["powerTubeTemp_dC"] = JKMessenger.get_power_tube_temp_dC();
    measurements["batteryBoxTemp_dC"] = JKMessenger.get_battery_box_temp_dC();
    measurements["batteryTemp_dC"] = JKMessenger.get_battery_temp_dC();
    measurements["totalVoltage_mV"] = JKMessenger.get_total_voltage_mV();
    measurements["current_dA"] = JKMessenger.get_current_dA();
    measurements["remainingCapacity_pct"] = JKMessenger.get_remaining_capacity_pct();
    measurements["temperatureSensorCount"] = JKMessenger.get_temp_sensor_count_n();
    measurements["cycleCount"] = JKMessenger.get_cycle_count();
    measurements["totalCycleCapacity_Ah"] = JKMessenger.get_total_cycle_capacity_Ah();
    measurements["stringCount"] = JKMessenger.get_string_count_n();

    const JikongMessenger::Warning_Flags *warningFlags = JKMessenger.get_warning_flags();
    JsonObject warnings = document["warnings"].to<JsonObject>();
    warnings["lowCapacity"] = warningFlags->low_capacity;
    warnings["mosTubeOvertemperature"] = warningFlags->MOS_tube_OT;
    warnings["chargingOvervoltage"] = warningFlags->charging_OV;
    warnings["dischargeUndervoltage"] = warningFlags->discharge_UV;
    warnings["batteryOvertemperature"] = warningFlags->battery_OT;
    warnings["chargingOvercurrent"] = warningFlags->charging_OC;
    warnings["dischargeOvercurrent"] = warningFlags->discharge_OC;
    warnings["cellPressureDifferential"] = warningFlags->Cell_pressure_differential;
    warnings["batteryBoxOvertemperature"] = warningFlags->BB_OT;
    warnings["batteryLowTemperature"] = warningFlags->battery_low_temp;
    warnings["cellOvervoltage"] = warningFlags->monomer_OV;
    warnings["cellUndervoltage"] = warningFlags->monomer_UV;
    warnings["protection309A"] = warningFlags->protection_309A;

    const JikongMessenger::Status_Flags *statusFlags = JKMessenger.get_status_flags();
    JsonObject status = document["status"].to<JsonObject>();
    status["chargingMosOn"] = statusFlags->charging_MOS_status;
    status["dischargeMosOn"] = statusFlags->discharge_MOS_status;
    status["balancingOn"] = statusFlags->balance_switch_state;
    status["batteryConnected"] = statusFlags->battery_conn_status;

    JsonObject voltageProtections = document["voltageProtections"].to<JsonObject>();
    voltageProtections["packOvervoltage_mV"] = JKMessenger.get_pack_overvoltage_mV();
    voltageProtections["packUndervoltage_mV"] = JKMessenger.get_pack_undervoltage_mV();
    voltageProtections["cellOvervoltage_mV"] = JKMessenger.get_cell_overvoltage_mV();
    voltageProtections["cellOvervoltageRecovery_mV"] = JKMessenger.get_cell_overvoltage_recovery_mV();
    voltageProtections["cellOvervoltageDelay_s"] = JKMessenger.get_cell_overvoltage_delay_s();
    voltageProtections["cellUndervoltage_mV"] = JKMessenger.get_cell_undervoltage_mV();
    voltageProtections["cellUndervoltageRelease_mV"] = JKMessenger.get_monomer_undervoltage_release_mV();
    voltageProtections["cellUndervoltageDelay_s"] = JKMessenger.get_cell_undervoltage_delay_s();
    voltageProtections["cellPressureDifference_mV"] = JKMessenger.get_cell_pressure_diff_protection_mV();

    JsonObject currentProtections = document["currentProtections"].to<JsonObject>();
    currentProtections["dischargeOvercurrent_A"] = JKMessenger.get_discharge_overcurrent_A();
    currentProtections["dischargeOvercurrentDelay_s"] = JKMessenger.get_discharge_overcurrent_delay_s();
    currentProtections["chargeOvercurrent_A"] = JKMessenger.get_charge_overcurrent_A();
    currentProtections["chargeOvercurrentDelay_s"] = JKMessenger.get_charge_overcurrent_delay_s();

    JsonObject balancing = document["balancing"].to<JsonObject>();
    balancing["startVoltage_mV"] = JKMessenger.get_balance_start_voltage_mV();
    balancing["voltageDifference_mV"] = JKMessenger.get_balance_diff_mV();
    balancing["activeBalanceEnabled"] = JKMessenger.is_active_balance_enabled();

    JsonObject temperatureProtections = document["temperatureProtections"].to<JsonObject>();
    temperatureProtections["powerTubeProtection_dC"] = JKMessenger.get_power_tube_temp_protection_value_dC();
    temperatureProtections["powerTubeRecovery_dC"] = JKMessenger.get_power_tube_temp_recovery_value_dC();
    temperatureProtections["boxHigh_dC"] = JKMessenger.get_box_high_temp_dC();
    temperatureProtections["boxRecovery_dC"] = JKMessenger.get_box_temp_recovery_dC();
    temperatureProtections["batteryDifference_dC"] = JKMessenger.get_battery_temp_diff_dC();
    temperatureProtections["chargeHigh_dC"] = JKMessenger.get_charge_high_temp_dC();
    temperatureProtections["dischargeHigh_dC"] = JKMessenger.get_discharge_high_temp_dC();
    temperatureProtections["chargeLow_dC"] = JKMessenger.get_charge_low_temp_dC();
    temperatureProtections["chargeLowRecovery_dC"] = JKMessenger.get_charge_low_temp_recovery_dC();
    temperatureProtections["dischargeLow_dC"] = JKMessenger.get_discharge_low_temp_dC();
    temperatureProtections["dischargeLowRecovery_dC"] = JKMessenger.get_discharge_low_temp_recovery_dC();

    JsonObject configuration = document["configuration"].to<JsonObject>();
    configuration["batteryStringSetting"] = JKMessenger.get_battery_string_setting();
    configuration["batteryCapacitySetting"] = JKMessenger.get_battery_capacity_setting();
    configuration["batteryCapacity_Ah"] = JKMessenger.get_battery_capacity_Ah();
    configuration["chargeMosEnabled"] = JKMessenger.is_charge_mos_enabled();
    configuration["dischargeMosEnabled"] = JKMessenger.is_discharge_mos_enabled();
    configuration["currentCalibrationOffset"] = JKMessenger.get_current_calibration_offset();
    configuration["pBoardAddress"] = JKMessenger.get_P_board_address();
    configuration["batteryType"] = JKMessenger.get_battery_type();
    configuration["sleepDelay_s"] = JKMessenger.get_sleep_delay_s();
    configuration["lowCapacityAlarm_pct"] = JKMessenger.get_low_cap_alarmVol_pct();
    configuration["parameterPassword"] = JKMessenger.get_parameter_password();
    configuration["dedicatedChargerEnabled"] = JKMessenger.is_dedicated_charger_enabled();
    configuration["deviceId"] = JKMessenger.get_device_id();
    configuration["manufactureYear"] = JKMessenger.get_manufacture_year();
    configuration["manufactureMonth"] = JKMessenger.get_manufacture_month();
    configuration["runtimeHours"] = JKMessenger.get_runtime_hours();
    configuration["firmwareVersion"] = JKMessenger.get_firmware_version();
    configuration["currentCalibrationEnabled"] = JKMessenger.is_current_calibration_enabled();
    configuration["actualCapacity_Ah"] = JKMessenger.get_actual_capacity_Ah();
    configuration["manufacturerId"] = JKMessenger.get_manufacturer_id();
    configuration["protocolVersion"] = JKMessenger.get_protocol_version_number();

    String json;
    serializeJson(document, json);
#if DEBUG_ENABLED
    Serial.println(json);
#endif
    return json;
}

#if not BMS_ENABLED
void getDummyBMS()
{
    LastBMSData.batteryLife = 30;
    LastBMSData.MOSStatus[0] = 1;
    LastBMSData.MOSStatus[1] = 1;
    LastBMSData.packTemp = 30;
    for (size_t i = 0; i < 12; i++)
    {
        LastBMSData.cellVoltages[i] = 1;
    }
    LastBMSData.currentDraw = 1;
    LastBMSData.totalVoltage = 10;
    LastBMSData.error = "";
}
#endif