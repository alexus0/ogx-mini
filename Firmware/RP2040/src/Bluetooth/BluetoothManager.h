#pragma once

#include <cstdint>
#include <atomic>
#include "Bluetooth/DeviceDiscovery.h"
#include "Bluetooth/AutoReconnect.h"
#include "Gamepad/Gamepad.h"
#include "Board/Config.h"

namespace bluetooth {

enum class BluetoothState : uint8_t {
    UNINITIALIZED = 0,
    INITIALIZING = 1,
    IDLE = 2,
    DISCOVERING = 3,
    CONNECTING = 4,
    CONNECTED = 5,
    RECONNECTING = 6,
    ERROR = 7
};

class BluetoothManager {
public:
    static BluetoothManager& get_instance() {
        static BluetoothManager instance;
        return instance;
    }
    
    // Initialization and control
    void initialize(Gamepad(&gamepads)[MAX_GAMEPADS]);
    void shutdown();
    void update();
    
    // State management
    BluetoothState get_state() const { return state_.load(); }
    bool is_initialized() const { return initialized_.load(); }
    bool is_connected() const { return state_.load() == BluetoothState::CONNECTED; }
    
    // Discovery operations
    void start_device_discovery();
    void stop_device_discovery();
    bool is_discovering() const;
    uint8_t get_discovered_device_count() const;
    const DiscoveredDevice* get_discovered_device(uint8_t index) const;
    
    // Connection operations
    void connect_to_device(const BluetoothAddress& addr);
    void disconnect_device();
    void reconnect_last_device();
    
    // Favorite devices
    bool add_favorite_device(const BluetoothAddress& addr, const char* name);
    bool remove_favorite_device(const BluetoothAddress& addr);
    uint8_t get_favorite_device_count() const;
    const FavoriteDevice* get_favorite_device(uint8_t index) const;
    bool is_favorite(const BluetoothAddress& addr) const;
    
    // Auto-reconnect
    void enable_auto_reconnect(const BluetoothAddress& addr, uint8_t max_retries = 5);
    void disable_auto_reconnect(const BluetoothAddress& addr);
    bool is_auto_reconnect_enabled(const BluetoothAddress& addr) const;
    uint8_t get_active_reconnect_count() const;
    
    // Configuration
    void set_rssi_threshold(int8_t threshold);
    void set_discovery_timeout(uint32_t timeout_ms);
    void set_connection_timeout(uint32_t timeout_ms);
    void set_retry_interval(uint32_t interval_ms);
    void set_max_retry_count(uint8_t max_retries);
    
    // Device callbacks (called by Bluepad32/BLEServer)
    void on_device_discovered(const BluetoothAddress& addr, const char* name,
                             int8_t rssi, uint16_t cod);
    void on_device_connected(const BluetoothAddress& addr, const Gamepad* gamepad);
    void on_device_disconnected(const BluetoothAddress& addr);
    void on_connection_failed(const BluetoothAddress& addr);
    
    // Debug/monitoring
    void log_status() const;
    const char* get_state_string() const;
    
private:
    BluetoothManager() = default;
    ~BluetoothManager() = default;
    BluetoothManager(const BluetoothManager&) = delete;
    BluetoothManager& operator=(const BluetoothManager&) = delete;
    
    std::atomic<BluetoothState> state_{BluetoothState::UNINITIALIZED};
    std::atomic<bool> initialized_{false};
    
    DeviceDiscovery& discovery_{DeviceDiscovery::get_instance()};
    AutoReconnect& auto_reconnect_{AutoReconnect::get_instance()};
    
    Gamepad* gamepads_{nullptr};
    uint8_t gamepad_count_{0};
    
    BluetoothAddress connected_device_;
    BluetoothAddress last_device_;
    
    uint32_t discovery_start_ms_{0};
    uint32_t discovery_timeout_ms_{10000};
    uint32_t last_update_ms_{0};
    
    uint8_t max_retry_count_{5};
    
    void set_state(BluetoothState new_state);
    void handle_discovery_update();
    void handle_reconnection_update();
    void persist_settings();
    void restore_settings();
};

} // namespace bluetooth
