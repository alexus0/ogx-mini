#include "Calibration/CalibrationManager.h"
#include "Board/ogxm_log.h"
#include "Board/board_api.h"
#include <cstring>
#include <algorithm>

namespace calibration {

void CalibrationManager::initialize() {
    std::memset(&current_calibration_, 0, sizeof(CalibrationData));
    std::memset(&working_calibration_, 0, sizeof(CalibrationData));
    calibrating_ = false;
    
    OGXM_LOG("Calibration: Manager initialized\n");
}

void CalibrationManager::start_calibration(Gamepad& gamepad) {
    if (calibrating_) {
        OGXM_LOG("Calibration: Already calibrating\n");
        return;
    }
    
    calibrating_ = true;
    std::memset(&working_calibration_, 0, sizeof(CalibrationData));
    
    // Initialize min/max values for calibration
    working_calibration_.left_stick_x_min = 32767;
    working_calibration_.left_stick_x_max = -32768;
    working_calibration_.left_stick_y_min = 32767;
    working_calibration_.left_stick_y_max = -32768;
    
    working_calibration_.right_stick_x_min = 32767;
    working_calibration_.right_stick_x_max = -32768;
    working_calibration_.right_stick_y_min = 32767;
    working_calibration_.right_stick_y_max = -32768;
    
    working_calibration_.trigger_l_min = 32767;
    working_calibration_.trigger_l_max = -32768;
    working_calibration_.trigger_r_min = 32767;
    working_calibration_.trigger_r_max = -32768;
    
    OGXM_LOG("Calibration: Started for gamepad\n");
}

void CalibrationManager::stop_calibration() {
    if (!calibrating_) {
        return;
    }
    
    calibrating_ = false;
    OGXM_LOG("Calibration: Stopped\n");
}

void CalibrationManager::update_calibration(const Gamepad& gamepad) {
    if (!calibrating_) {
        return;
    }
    
    // Update stick calibration ranges
    working_calibration_.left_stick_x_min = std::min(
        working_calibration_.left_stick_x_min, 
        gamepad.get_left_stick_x()
    );
    working_calibration_.left_stick_x_max = std::max(
        working_calibration_.left_stick_x_max, 
        gamepad.get_left_stick_x()
    );
    
    working_calibration_.left_stick_y_min = std::min(
        working_calibration_.left_stick_y_min, 
        gamepad.get_left_stick_y()
    );
    working_calibration_.left_stick_y_max = std::max(
        working_calibration_.left_stick_y_max, 
        gamepad.get_left_stick_y()
    );
    
    working_calibration_.right_stick_x_min = std::min(
        working_calibration_.right_stick_x_min, 
        gamepad.get_right_stick_x()
    );
    working_calibration_.right_stick_x_max = std::max(
        working_calibration_.right_stick_x_max, 
        gamepad.get_right_stick_x()
    );
    
    working_calibration_.right_stick_y_min = std::min(
        working_calibration_.right_stick_y_min, 
        gamepad.get_right_stick_y()
    );
    working_calibration_.right_stick_y_max = std::max(
        working_calibration_.right_stick_y_max, 
        gamepad.get_right_stick_y()
    );
    
    // Update trigger calibration ranges
    working_calibration_.trigger_l_min = std::min(
        working_calibration_.trigger_l_min, 
        gamepad.get_trigger_l()
    );
    working_calibration_.trigger_l_max = std::max(
        working_calibration_.trigger_l_max, 
        gamepad.get_trigger_l()
    );
    
    working_calibration_.trigger_r_min = std::min(
        working_calibration_.trigger_r_min, 
        gamepad.get_trigger_r()
    );
    working_calibration_.trigger_r_max = std::max(
        working_calibration_.trigger_r_max, 
        gamepad.get_trigger_r()
    );
}

void CalibrationManager::finalize_calibration() {
    if (!calibrating_) {
        return;
    }
    
    working_calibration_.is_calibrated = 1;
    working_calibration_.checksum = calculate_checksum(working_calibration_);
    
    if (is_calibration_valid(working_calibration_)) {
        current_calibration_ = working_calibration_;
        OGXM_LOG("Calibration: Finalized successfully\n");
    } else {
        OGXM_LOG("Calibration: Validation failed\n");
    }
    
    calibrating_ = false;
}

bool CalibrationManager::load_calibration(Gamepad& gamepad) {
    // TODO: Load from NVS storage
    OGXM_LOG("Calibration: Loading from NVS (TODO: implement)\n");
    return false;
}

bool CalibrationManager::save_calibration(const Gamepad& gamepad) {
    if (!is_calibration_valid(current_calibration_)) {
        OGXM_LOG("Calibration: Cannot save invalid calibration\n");
        return false;
    }
    
    // TODO: Save to NVS storage
    OGXM_LOG("Calibration: Saving to NVS (TODO: implement)\n");
    return true;
}

bool CalibrationManager::reset_calibration(Gamepad& gamepad) {
    std::memset(&current_calibration_, 0, sizeof(CalibrationData));
    current_calibration_.is_calibrated = 0;
    
    OGXM_LOG("Calibration: Reset to defaults\n");
    return true;
}

void CalibrationManager::apply_calibration(Gamepad& gamepad) {
    if (current_calibration_.is_calibrated == 0) {
        return; // No calibration to apply
    }
    
    // Apply stick calibration
    int16_t left_x_range = current_calibration_.left_stick_x_max - 
                           current_calibration_.left_stick_x_min;
    int16_t left_y_range = current_calibration_.left_stick_y_max - 
                           current_calibration_.left_stick_y_min;
    int16_t right_x_range = current_calibration_.right_stick_x_max - 
                            current_calibration_.right_stick_x_min;
    int16_t right_y_range = current_calibration_.right_stick_y_max - 
                            current_calibration_.right_stick_y_min;
    
    if (left_x_range != 0) {
        int16_t calibrated_x = (gamepad.get_left_stick_x() - 
                               current_calibration_.left_stick_x_min) * 
                               32767 / left_x_range - 16384;
        // Apply calibrated value
    }
    
    if (left_y_range != 0) {
        int16_t calibrated_y = (gamepad.get_left_stick_y() - 
                               current_calibration_.left_stick_y_min) * 
                               32767 / left_y_range - 16384;
        // Apply calibrated value
    }
    
    // Similar for right stick and triggers
}

bool CalibrationManager::is_calibration_valid(const CalibrationData& data) const {
    if (data.is_calibrated == 0) {
        return false;
    }
    
    // Verify checksum
    uint16_t calculated_checksum = calculate_checksum(data);
    if (calculated_checksum != data.checksum) {
        OGXM_LOG("Calibration: Checksum mismatch\n");
        return false;
    }
    
    // Verify ranges are sensible
    if (data.left_stick_x_min >= data.left_stick_x_max ||
        data.left_stick_y_min >= data.left_stick_y_max ||
        data.right_stick_x_min >= data.right_stick_x_max ||
        data.right_stick_y_min >= data.right_stick_y_max) {
        OGXM_LOG("Calibration: Invalid ranges\n");
        return false;
    }
    
    return true;
}

uint16_t CalibrationManager::calculate_checksum(const CalibrationData& data) const {
    uint16_t checksum = 0;
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&data);
    
    // Calculate checksum over all bytes except the checksum field itself
    size_t checksum_offset = offsetof(CalibrationData, checksum);
    
    for (size_t i = 0; i < checksum_offset; i++) {
        checksum += bytes[i];
        checksum = (checksum << 1) | (checksum >> 15); // Rotate left
    }
    
    return checksum;
}

// BluetoothCalibrationHelper implementation

void BluetoothCalibrationHelper::on_device_connected(const Gamepad* gamepad) {
    if (!gamepad) {
        return;
    }
    
    OGXM_LOG("Calibration: Device connected, checking calibration status\n");
    
    CalibrationManager& calib_mgr = CalibrationManager::get_instance();
    
    if (calib_mgr.load_calibration(const_cast<Gamepad&>(*gamepad))) {
        OGXM_LOG("Calibration: Applied stored calibration\n");
    } else {
        OGXM_LOG("Calibration: No stored calibration found\n");
    }
}

void BluetoothCalibrationHelper::on_device_disconnected(const Gamepad* gamepad) {
    if (!gamepad) {
        return;
    }
    
    OGXM_LOG("Calibration: Device disconnected, saving calibration\n");
    
    CalibrationManager& calib_mgr = CalibrationManager::get_instance();
    calib_mgr.save_calibration(const_cast<Gamepad&>(*gamepad));
}

bool BluetoothCalibrationHelper::needs_calibration(const Gamepad* gamepad) const {
    if (!gamepad) {
        return false;
    }
    
    CalibrationManager& calib_mgr = CalibrationManager::get_instance();
    // Check if device has valid calibration data
    // TODO: Implement proper check
    return false;
}

const char* BluetoothCalibrationHelper::get_calibration_status(const Gamepad* gamepad) const {
    if (!gamepad) {
        return "Invalid";
    }
    
    if (needs_calibration(gamepad)) {
        return "Needs Calibration";
    }
    
    return "Calibrated";
}

} // namespace calibration
