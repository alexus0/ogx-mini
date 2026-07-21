#pragma once

#include <cstdint>
#include "Gamepad/Gamepad.h"
#include "Bluetooth/BluetoothManager.h"

namespace calibration {

// Calibration data structure
struct CalibrationData {
    int16_t left_stick_x_min;
    int16_t left_stick_x_max;
    int16_t left_stick_y_min;
    int16_t left_stick_y_max;
    
    int16_t right_stick_x_min;
    int16_t right_stick_x_max;
    int16_t right_stick_y_min;
    int16_t right_stick_y_max;
    
    int16_t trigger_l_min;
    int16_t trigger_l_max;
    int16_t trigger_r_min;
    int16_t trigger_r_max;
    
    uint8_t is_calibrated;
    uint16_t checksum;
};

class CalibrationManager {
public:
    static CalibrationManager& get_instance() {
        static CalibrationManager instance;
        return instance;
    }
    
    // Initialize calibration system
    void initialize();
    
    // Calibration operations
    void start_calibration(Gamepad& gamepad);
    void stop_calibration();
    bool is_calibrating() const { return calibrating_; }
    
    // Update during calibration
    void update_calibration(const Gamepad& gamepad);
    void finalize_calibration();
    
    // Get/Set calibration data
    bool load_calibration(Gamepad& gamepad);
    bool save_calibration(const Gamepad& gamepad);
    bool reset_calibration(Gamepad& gamepad);
    
    // Apply calibration to gamepad values
    void apply_calibration(Gamepad& gamepad);
    
    // Validation
    bool is_calibration_valid(const CalibrationData& data) const;
    uint16_t calculate_checksum(const CalibrationData& data) const;
    
private:
    CalibrationManager() = default;
    ~CalibrationManager() = default;
    CalibrationManager(const CalibrationManager&) = delete;
    CalibrationManager& operator=(const CalibrationManager&) = delete;
    
    bool calibrating_{false};
    CalibrationData current_calibration_;
    CalibrationData working_calibration_;
};

// Bluetooth-aware calibration helper
class BluetoothCalibrationHelper {
public:
    static BluetoothCalibrationHelper& get_instance() {
        static BluetoothCalibrationHelper instance;
        return instance;
    }
    
    // Auto-calibrate on device connection
    void on_device_connected(const Gamepad* gamepad);
    
    // Store calibration after disconnection
    void on_device_disconnected(const Gamepad* gamepad);
    
    // Check if device needs calibration
    bool needs_calibration(const Gamepad* gamepad) const;
    
    // Get calibration status
    const char* get_calibration_status(const Gamepad* gamepad) const;
    
private:
    BluetoothCalibrationHelper() = default;
    ~BluetoothCalibrationHelper() = default;
    BluetoothCalibrationHelper(const BluetoothCalibrationHelper&) = delete;
    BluetoothCalibrationHelper& operator=(const BluetoothCalibrationHelper&) = delete;
};

} // namespace calibration
