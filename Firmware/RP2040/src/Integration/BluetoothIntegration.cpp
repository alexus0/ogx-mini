#include "Integration/BluetoothIntegration.h"
#include "Board/ogxm_log.h"
#include "Board/board_api.h"
#include <cstring>

namespace integration {

void BluetoothIntegration::initialize(Gamepad(&gamepads)[MAX_GAMEPADS]) {
    if (initialized_.load()) {
        OGXM_LOG("Integration: Already initialized\n");
        return;
    }
    
    gamepads_ = gamepads;
    gamepad_count_ = MAX_GAMEPADS;
    active_gamepad_index_ = 0;
    
    // Initialize Bluetooth manager
    bt_manager_.initialize();
    
    // Initialize calibration manager
    calib_manager_.initialize();
    
    // Try to restore previous calibration
    restore_calibration();
    
    // Register event dispatcher
    BluetoothEventDispatcher::get_instance().register_integration(this);
    
    initialized_.store(true);
    OGXM_LOG("Integration: Initialized successfully\n");
}

void BluetoothIntegration::shutdown() {
    if (!initialized_.load()) {
        return;
    }
    
    // Disconnect current device
    disconnect_current_device();
    
    // Unregister event dispatcher
    BluetoothEventDispatcher::get_instance().unregister_integration();
    
    // Shutdown managers
    bt_manager_.shutdown();
    
    initialized_.store(false);
    OGXM_LOG("Integration: Shutdown complete\n");
}

void BluetoothIntegration::update() {
    if (!initialized_.load()) {
        return;
    }
    
    uint32_t current_ms = board_api_get_ms();
    
    // Update Bluetooth manager
    bt_manager_.update();
    
    // Check for auto-reconnect
    if (auto_reconnect_enabled_ && !is_device_connected()) {
        // TODO: Implement auto-reconnect logic
    }
    
    last_update_ms_ = current_ms;
}

void BluetoothIntegration::on_device_discovered(const bluetooth::BluetoothAddress& addr, 
                                                const char* name, int8_t rssi, uint16_t cod) {
    OGXM_LOG("Integration: Device discovered - %s (RSSI: %d)\n", name, rssi);
    
    // Add device to manager's list
    bt_manager_.add_discovered_device(addr, name, rssi, cod);
}

void BluetoothIntegration::on_device_connected(const bluetooth::BluetoothAddress& addr) {
    OGXM_LOG("Integration: Device connected\n");
    
    // Find gamepad for this device
    for (uint8_t i = 0; i < gamepad_count_; i++) {
        if (gamepads_[i].is_connected()) {
            active_gamepad_index_ = i;
            
            // Trigger auto-calibration if needed
            if (calib_helper_.needs_calibration(&gamepads_[i])) {
                perform_auto_calibration();
            } else {
                // Restore existing calibration
                restore_calibration();
            }
            
            // Notify calibration helper
            calib_helper_.on_device_connected(&gamepads_[i]);
            
            OGXM_LOG("Integration: Gamepad %d connected\n", i);
            break;
        }
    }
}

void BluetoothIntegration::on_device_disconnected(const bluetooth::BluetoothAddress& addr) {
    OGXM_LOG("Integration: Device disconnected\n");
    
    // Save calibration data
    if (active_gamepad_index_ < gamepad_count_) {
        calib_helper_.on_device_disconnected(&gamepads_[active_gamepad_index_]);
        calib_manager_.save_calibration(gamepads_[active_gamepad_index_]);
    }
}

void BluetoothIntegration::on_connection_failed(const bluetooth::BluetoothAddress& addr) {
    OGXM_LOG("Integration: Connection failed\n");
    
    // Increment retry count
    // TODO: Implement retry logic with exponential backoff
}

void BluetoothIntegration::on_gamepad_data_received(uint8_t gamepad_index, const Gamepad* gamepad) {
    if (gamepad_index >= gamepad_count_) {
        return;
    }
    
    // Apply calibration if available
    calib_manager_.apply_calibration(gamepads_[gamepad_index]);
    
    // Update during calibration
    if (calib_manager_.is_calibrating()) {
        calib_manager_.update_calibration(*gamepad);
    }
    
    route_gamepad_input(gamepad_index, gamepad);
}

bool BluetoothIntegration::is_device_connected() const {
    return bt_manager_.is_connected();
}

bluetooth::BluetoothState BluetoothIntegration::get_bluetooth_state() const {
    return bt_manager_.get_state();
}

const char* BluetoothIntegration::get_state_string() const {
    switch (get_bluetooth_state()) {
        case bluetooth::BluetoothState::IDLE:
            return "Idle";
        case bluetooth::BluetoothState::DISCOVERING:
            return "Discovering";
        case bluetooth::BluetoothState::CONNECTING:
            return "Connecting";
        case bluetooth::BluetoothState::CONNECTED:
            return "Connected";
        case bluetooth::BluetoothState::DISCONNECTING:
            return "Disconnecting";
        case bluetooth::BluetoothState::ERROR:
            return "Error";
        default:
            return "Unknown";
    }
}

void BluetoothIntegration::set_discovery_timeout(uint32_t timeout_ms) {
    bt_manager_.set_discovery_timeout(timeout_ms);
}

void BluetoothIntegration::set_connection_timeout(uint32_t timeout_ms) {
    bt_manager_.set_connection_timeout(timeout_ms);
}

void BluetoothIntegration::set_auto_reconnect_enabled(bool enabled) {
    auto_reconnect_enabled_ = enabled;
}

void BluetoothIntegration::set_max_retry_count(uint8_t max_retries) {
    bt_manager_.set_max_retry_count(max_retries);
}

void BluetoothIntegration::start_discovery() {
    OGXM_LOG("Integration: Starting device discovery\n");
    bt_manager_.start_discovery();
}

void BluetoothIntegration::stop_discovery() {
    OGXM_LOG("Integration: Stopping device discovery\n");
    bt_manager_.stop_discovery();
}

void BluetoothIntegration::connect_to_device(uint8_t device_index) {
    OGXM_LOG("Integration: Connecting to device %d\n", device_index);
    bt_manager_.connect_to_device(device_index);
}

void BluetoothIntegration::disconnect_current_device() {
    OGXM_LOG("Integration: Disconnecting current device\n");
    bt_manager_.disconnect();
}

void BluetoothIntegration::reconnect_last_device() {
    OGXM_LOG("Integration: Reconnecting to last device\n");
    bt_manager_.reconnect_last_device();
}

void BluetoothIntegration::add_favorite_device(uint8_t device_index) {
    bt_manager_.add_favorite_device(device_index);
}

void BluetoothIntegration::remove_favorite_device(uint8_t device_index) {
    bt_manager_.remove_favorite_device(device_index);
}

uint8_t BluetoothIntegration::get_favorite_device_count() const {
    return bt_manager_.get_favorite_device_count();
}

void BluetoothIntegration::start_calibration() {
    if (active_gamepad_index_ < gamepad_count_) {
        OGXM_LOG("Integration: Starting calibration for gamepad %d\n", active_gamepad_index_);
        calib_manager_.start_calibration(gamepads_[active_gamepad_index_]);
    }
}

void BluetoothIntegration::stop_calibration() {
    OGXM_LOG("Integration: Stopping calibration\n");
    calib_manager_.stop_calibration();
}

bool BluetoothIntegration::is_calibrating() const {
    return calib_manager_.is_calibrating();
}

void BluetoothIntegration::finalize_calibration() {
    OGXM_LOG("Integration: Finalizing calibration\n");
    calib_manager_.finalize_calibration();
    
    // Save calibration
    if (active_gamepad_index_ < gamepad_count_) {
        calib_manager_.save_calibration(gamepads_[active_gamepad_index_]);
    }
}

void BluetoothIntegration::log_status() const {
    OGXM_LOG("=== Bluetooth Integration Status ===\n");
    OGXM_LOG("Initialized: %s\n", initialized_.load() ? "Yes" : "No");
    OGXM_LOG("State: %s\n", get_state_string());
    OGXM_LOG("Connected: %s\n", is_device_connected() ? "Yes" : "No");
    OGXM_LOG("Active Gamepad: %d\n", active_gamepad_index_);
    OGXM_LOG("Calibrating: %s\n", is_calibrating() ? "Yes" : "No");
    OGXM_LOG("Auto-reconnect: %s\n", auto_reconnect_enabled_ ? "Enabled" : "Disabled");
    OGXM_LOG("==================================\n");
}

void BluetoothIntegration::route_gamepad_input(uint8_t gamepad_index, const Gamepad* gamepad) {
    // TODO: Route gamepad input to appropriate system
    // This could be HID, custom protocol, or game-specific routing
}

void BluetoothIntegration::handle_state_transition(bluetooth::BluetoothState new_state) {
    OGXM_LOG("Integration: State transition to %s\n", 
             new_state == bluetooth::BluetoothState::CONNECTED ? "Connected" : "Disconnected");
}

void BluetoothIntegration::perform_auto_calibration() {
    OGXM_LOG("Integration: Performing auto-calibration\n");
    start_calibration();
    // TODO: Implement automatic calibration procedure
}

void BluetoothIntegration::restore_calibration() {
    if (active_gamepad_index_ < gamepad_count_) {
        if (calib_manager_.load_calibration(gamepads_[active_gamepad_index_])) {
            OGXM_LOG("Integration: Calibration restored\n");
        }
    }
}

// BluetoothEventDispatcher implementation

void BluetoothEventDispatcher::register_integration(BluetoothIntegration* integration) {
    integration_ = integration;
    OGXM_LOG("EventDispatcher: Integration registered\n");
}

void BluetoothEventDispatcher::unregister_integration() {
    integration_ = nullptr;
    OGXM_LOG("EventDispatcher: Integration unregistered\n");
}

void BluetoothEventDispatcher::dispatch_device_discovered(const bluetooth::BluetoothAddress& addr, 
                                                         const char* name, int8_t rssi, uint16_t cod) {
    if (integration_) {
        integration_->on_device_discovered(addr, name, rssi, cod);
    }
}

void BluetoothEventDispatcher::dispatch_device_connected(const bluetooth::BluetoothAddress& addr) {
    if (integration_) {
        integration_->on_device_connected(addr);
    }
}

void BluetoothEventDispatcher::dispatch_device_disconnected(const bluetooth::BluetoothAddress& addr) {
    if (integration_) {
        integration_->on_device_disconnected(addr);
    }
}

void BluetoothEventDispatcher::dispatch_connection_failed(const bluetooth::BluetoothAddress& addr) {
    if (integration_) {
        integration_->on_connection_failed(addr);
    }
}

void BluetoothEventDispatcher::dispatch_gamepad_data(uint8_t gamepad_index, const Gamepad* gamepad) {
    if (integration_) {
        integration_->on_gamepad_data_received(gamepad_index, gamepad);
    }
}

} // namespace integration
