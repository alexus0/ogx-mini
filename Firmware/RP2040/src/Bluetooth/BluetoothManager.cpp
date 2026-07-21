#include "Bluetooth/BluetoothManager.h"
#include "Board/ogxm_log.h"
#include "Board/board_api.h"
#include <cstring>

namespace bluetooth {

void BluetoothManager::initialize(Gamepad(&gamepads)[MAX_GAMEPADS]) {
    if (initialized_.load()) {
        OGXM_LOG("Bluetooth: Manager already initialized\n");
        return;
    }
    
    set_state(BluetoothState::INITIALIZING);
    
    gamepads_ = gamepads;
    gamepad_count_ = MAX_GAMEPADS;
    
    // Initialize subsystems
    discovery_.set_rssi_threshold(-100);
    auto_reconnect_.initialize();
    
    // Restore persisted settings
    restore_settings();
    
    set_state(BluetoothState::IDLE);
    initialized_.store(true);
    last_update_ms_ = board_api::ms_since_boot();
    
    OGXM_LOG("Bluetooth: Manager initialized successfully\n");
}

void BluetoothManager::shutdown() {
    if (!initialized_.load()) {
        return;
    }
    
    set_state(BluetoothState::UNINITIALIZED);
    discovery_.stop_discovery();
    disconnect_device();
    persist_settings();
    
    initialized_.store(false);
    OGXM_LOG("Bluetooth: Manager shutdown\n");
}

void BluetoothManager::update() {
    if (!initialized_.load()) {
        return;
    }
    
    uint32_t now_ms = board_api::ms_since_boot();
    
    // Update subsystems
    auto_reconnect_.update();
    
    // Handle discovery timeout
    if (is_discovering()) {
        if ((now_ms - discovery_start_ms_) > discovery_timeout_ms_) {
            stop_device_discovery();
        }
    }
    
    // Handle state-specific updates
    handle_discovery_update();
    handle_reconnection_update();
    
    last_update_ms_ = now_ms;
}

void BluetoothManager::start_device_discovery() {
    if (state_.load() == BluetoothState::DISCOVERING) {
        return;
    }
    
    set_state(BluetoothState::DISCOVERING);
    discovery_.clear_discovered_devices();
    discovery_.start_discovery();
    discovery_start_ms_ = board_api::ms_since_boot();
    
    OGXM_LOG("Bluetooth: Device discovery started\n");
}

void BluetoothManager::stop_device_discovery() {
    if (state_.load() != BluetoothState::DISCOVERING) {
        return;
    }
    
    discovery_.stop_discovery();
    set_state(BluetoothState::IDLE);
    
    OGXM_LOG("Bluetooth: Device discovery stopped (found %d devices)\n", 
             discovery_.get_discovered_device_count());
}

bool BluetoothManager::is_discovering() const {
    return discovery_.is_discovering();
}

uint8_t BluetoothManager::get_discovered_device_count() const {
    return discovery_.get_discovered_device_count();
}

const DiscoveredDevice* BluetoothManager::get_discovered_device(uint8_t index) const {
    return discovery_.get_discovered_device(index);
}

void BluetoothManager::connect_to_device(const BluetoothAddress& addr) {
    set_state(BluetoothState::CONNECTING);
    last_device_ = addr;
    
    OGXM_LOG("Bluetooth: Attempting to connect to device\n");
}

void BluetoothManager::disconnect_device() {
    if (is_connected()) {
        set_state(BluetoothState::IDLE);
        std::memset(connected_device_.addr, 0, MAC_ADDRESS_LEN);
        
        OGXM_LOG("Bluetooth: Device disconnected\n");
    }
}

void BluetoothManager::reconnect_last_device() {
    if (!last_device_.is_valid()) {
        OGXM_LOG("Bluetooth: No last device to reconnect\n");
        return;
    }
    
    enable_auto_reconnect(last_device_, max_retry_count_);
    OGXM_LOG("Bluetooth: Auto-reconnect enabled for last device\n");
}

bool BluetoothManager::add_favorite_device(const BluetoothAddress& addr, const char* name) {
    bool result = discovery_.add_favorite_device(addr, name);
    if (result) {
        persist_settings();
    }
    return result;
}

bool BluetoothManager::remove_favorite_device(const BluetoothAddress& addr) {
    bool result = discovery_.remove_favorite_device(addr);
    if (result) {
        persist_settings();
    }
    return result;
}

uint8_t BluetoothManager::get_favorite_device_count() const {
    return discovery_.get_favorite_device_count();
}

const FavoriteDevice* BluetoothManager::get_favorite_device(uint8_t index) const {
    return discovery_.get_favorite_device(index);
}

bool BluetoothManager::is_favorite(const BluetoothAddress& addr) const {
    return discovery_.is_favorite(addr);
}

void BluetoothManager::enable_auto_reconnect(const BluetoothAddress& addr, uint8_t max_retries) {
    auto_reconnect_.enable_auto_reconnect(addr, max_retries);
    persist_settings();
}

void BluetoothManager::disable_auto_reconnect(const BluetoothAddress& addr) {
    auto_reconnect_.disable_auto_reconnect(addr);
    persist_settings();
}

bool BluetoothManager::is_auto_reconnect_enabled(const BluetoothAddress& addr) const {
    return auto_reconnect_.is_auto_reconnect_enabled(addr);
}

uint8_t BluetoothManager::get_active_reconnect_count() const {
    return auto_reconnect_.get_active_reconnect_count();
}

void BluetoothManager::set_rssi_threshold(int8_t threshold) {
    discovery_.set_rssi_threshold(threshold);
}

void BluetoothManager::set_discovery_timeout(uint32_t timeout_ms) {
    discovery_timeout_ms_ = timeout_ms;
}

void BluetoothManager::set_connection_timeout(uint32_t timeout_ms) {
    auto_reconnect_.set_connection_timeout(timeout_ms);
}

void BluetoothManager::set_retry_interval(uint32_t interval_ms) {
    auto_reconnect_.set_retry_interval(interval_ms);
}

void BluetoothManager::set_max_retry_count(uint8_t max_retries) {
    max_retry_count_ = max_retries;
}

void BluetoothManager::on_device_discovered(const BluetoothAddress& addr, const char* name,
                                           int8_t rssi, uint16_t cod) {
    discovery_.add_discovered_device(addr, name, rssi, cod);
}

void BluetoothManager::on_device_connected(const BluetoothAddress& addr, const Gamepad* gamepad) {
    connected_device_ = addr;
    last_device_ = addr;
    set_state(BluetoothState::CONNECTED);
    auto_reconnect_.on_connection_established(addr);
    persist_settings();
    
    OGXM_LOG("Bluetooth: Device connected successfully\n");
}

void BluetoothManager::on_device_disconnected(const BluetoothAddress& addr) {
    if (connected_device_ == addr) {
        std::memset(connected_device_.addr, 0, MAC_ADDRESS_LEN);
    }
    
    // Check if auto-reconnect should be triggered
    if (is_auto_reconnect_enabled(addr)) {
        set_state(BluetoothState::RECONNECTING);
        auto_reconnect_.on_connection_lost(addr);
    } else {
        set_state(BluetoothState::IDLE);
    }
    
    OGXM_LOG("Bluetooth: Device disconnected\n");
}

void BluetoothManager::on_connection_failed(const BluetoothAddress& addr) {
    auto_reconnect_.on_connection_failed(addr);
    set_state(BluetoothState::RECONNECTING);
    
    OGXM_LOG("Bluetooth: Connection failed\n");
}

void BluetoothManager::log_status() const {
    OGXM_LOG("=== Bluetooth Manager Status ===\n");
    OGXM_LOG("State: %s\n", get_state_string());
    OGXM_LOG("Initialized: %d\n", initialized_.load());
    OGXM_LOG("Discovering: %d\n", is_discovering());
    OGXM_LOG("Connected: %d\n", is_connected());
    OGXM_LOG("Active Reconnects: %d\n", get_active_reconnect_count());
    OGXM_LOG("Discovered Devices: %d\n", get_discovered_device_count());
    OGXM_LOG("Favorite Devices: %d\n", get_favorite_device_count());
    OGXM_LOG("================================\n");
}

const char* BluetoothManager::get_state_string() const {
    switch (state_.load()) {
        case BluetoothState::UNINITIALIZED: return "UNINITIALIZED";
        case BluetoothState::INITIALIZING: return "INITIALIZING";
        case BluetoothState::IDLE: return "IDLE";
        case BluetoothState::DISCOVERING: return "DISCOVERING";
        case BluetoothState::CONNECTING: return "CONNECTING";
        case BluetoothState::CONNECTED: return "CONNECTED";
        case BluetoothState::RECONNECTING: return "RECONNECTING";
        case BluetoothState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void BluetoothManager::set_state(BluetoothState new_state) {
    BluetoothState old_state = state_.load();
    state_.store(new_state);
    
    if (old_state != new_state) {
        OGXM_LOG("Bluetooth: State change: %s -> %s\n", 
                 get_state_string(), 
                 BluetoothState new_state == BluetoothState::UNINITIALIZED ? "UNINITIALIZED" :
                 new_state == BluetoothState::INITIALIZING ? "INITIALIZING" :
                 new_state == BluetoothState::IDLE ? "IDLE" :
                 new_state == BluetoothState::DISCOVERING ? "DISCOVERING" :
                 new_state == BluetoothState::CONNECTING ? "CONNECTING" :
                 new_state == BluetoothState::CONNECTED ? "CONNECTED" :
                 new_state == BluetoothState::RECONNECTING ? "RECONNECTING" :
                 new_state == BluetoothState::ERROR ? "ERROR" : "UNKNOWN");
    }
}

void BluetoothManager::handle_discovery_update() {
    // Implemented in derived or specialized update logic
}

void BluetoothManager::handle_reconnection_update() {
    if (state_.load() == BluetoothState::RECONNECTING) {
        if (!auto_reconnect_.is_reconnecting()) {
            set_state(BluetoothState::IDLE);
        }
    }
}

void BluetoothManager::persist_settings() {
    // TODO: Implement NVS persistence for favorite devices and auto-reconnect settings
    OGXM_LOG("Bluetooth: Settings persisted (TODO: implement NVS)\n");
}

void BluetoothManager::restore_settings() {
    // TODO: Implement NVS restoration for favorite devices and auto-reconnect settings
    OGXM_LOG("Bluetooth: Settings restored (TODO: implement NVS)\n");
}

} // namespace bluetooth
