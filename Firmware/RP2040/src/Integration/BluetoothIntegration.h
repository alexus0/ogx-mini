#pragma once

#include <cstdint>
#include <atomic>
#include "Gamepad/Gamepad.h"
#include "Bluetooth/BluetoothManager.h"
#include "Calibration/CalibrationManager.h"

namespace integration {

// Integration layer between Bluetooth and Gamepad systems
class BluetoothIntegration {
public:
    static BluetoothIntegration& get_instance() {
        static BluetoothIntegration instance;
        return instance;
    }
    
    // Lifecycle management
    void initialize(Gamepad(&gamepads)[MAX_GAMEPADS]);
    void shutdown();
    void update();
    
    // Device connection callbacks
    void on_device_discovered(const bluetooth::BluetoothAddress& addr, 
                             const char* name, int8_t rssi, uint16_t cod);
    void on_device_connected(const bluetooth::BluetoothAddress& addr);
    void on_device_disconnected(const bluetooth::BluetoothAddress& addr);
    void on_connection_failed(const bluetooth::BluetoothAddress& addr);
    
    // Gamepad data routing
    void on_gamepad_data_received(uint8_t gamepad_index, const Gamepad* gamepad);
    
    // Status queries
    bool is_initialized() const { return initialized_.load(); }
    bool is_device_connected() const;
    bluetooth::BluetoothState get_bluetooth_state() const;
    const char* get_state_string() const;
    
    // Configuration
    void set_discovery_timeout(uint32_t timeout_ms);
    void set_connection_timeout(uint32_t timeout_ms);
    void set_auto_reconnect_enabled(bool enabled);
    void set_max_retry_count(uint8_t max_retries);
    
    // Device management
    void start_discovery();
    void stop_discovery();
    void connect_to_device(uint8_t device_index);
    void disconnect_current_device();
    void reconnect_last_device();
    
    // Favorite devices
    void add_favorite_device(uint8_t device_index);
    void remove_favorite_device(uint8_t device_index);
    uint8_t get_favorite_device_count() const;
    
    // Calibration integration
    void start_calibration();
    void stop_calibration();
    bool is_calibrating() const;
    void finalize_calibration();
    
    // Logging and debugging
    void log_status() const;
    
private:
    BluetoothIntegration() = default;
    ~BluetoothIntegration() = default;
    BluetoothIntegration(const BluetoothIntegration&) = delete;
    BluetoothIntegration& operator=(const BluetoothIntegration&) = delete;
    
    // Helper methods
    void route_gamepad_input(uint8_t gamepad_index, const Gamepad* gamepad);
    void handle_state_transition(bluetooth::BluetoothState new_state);
    void perform_auto_calibration();
    void restore_calibration();
    
    // Member variables
    std::atomic<bool> initialized_{false};
    bluetooth::BluetoothManager& bt_manager_{bluetooth::BluetoothManager::get_instance()};
    calibration::CalibrationManager& calib_manager_{calibration::CalibrationManager::get_instance()};
    calibration::BluetoothCalibrationHelper& calib_helper_{
        calibration::BluetoothCalibrationHelper::get_instance()
    };
    
    Gamepad* gamepads_{nullptr};
    uint8_t gamepad_count_{0};
    uint8_t active_gamepad_index_{0};
    
    bool auto_reconnect_enabled_{true};
    uint32_t last_update_ms_{0};
};

// Simple event dispatcher for Bluetooth callbacks
class BluetoothEventDispatcher {
public:
    static BluetoothEventDispatcher& get_instance() {
        static BluetoothEventDispatcher instance;
        return instance;
    }
    
    // Event registration
    void register_integration(BluetoothIntegration* integration);
    void unregister_integration();
    
    // Event dispatching (called from Bluetooth HAL)
    void dispatch_device_discovered(const bluetooth::BluetoothAddress& addr, 
                                   const char* name, int8_t rssi, uint16_t cod);
    void dispatch_device_connected(const bluetooth::BluetoothAddress& addr);
    void dispatch_device_disconnected(const bluetooth::BluetoothAddress& addr);
    void dispatch_connection_failed(const bluetooth::BluetoothAddress& addr);
    void dispatch_gamepad_data(uint8_t gamepad_index, const Gamepad* gamepad);
    
private:
    BluetoothEventDispatcher() = default;
    ~BluetoothEventDispatcher() = default;
    BluetoothEventDispatcher(const BluetoothEventDispatcher&) = delete;
    BluetoothEventDispatcher& operator=(const BluetoothEventDispatcher&) = delete;
    
    BluetoothIntegration* integration_{nullptr};
};

} // namespace integration
