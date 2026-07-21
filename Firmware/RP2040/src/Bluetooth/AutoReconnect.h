#pragma once

#include <cstdint>
#include <atomic>
#include <array>
#include "Board/Config.h"
#include "Bluetooth/DeviceDiscovery.h"

namespace bluetooth {

constexpr uint32_t RECONNECT_RETRY_INTERVAL_MS = 5000;
constexpr uint32_t RECONNECT_MAX_RETRIES = 5;
constexpr uint32_t RECONNECT_BACKOFF_MULTIPLIER = 1500;  // ms
constexpr uint32_t CONNECTION_TIMEOUT_MS = 15000;

enum class ReconnectState : uint8_t {
    IDLE = 0,
    SEARCHING = 1,
    CONNECTING = 2,
    CONNECTED = 3,
    FAILED = 4,
    BACKOFF = 5,
    DISABLED = 6
};

#pragma pack(push, 1)
struct ReconnectProfile {
    BluetoothAddress address;
    ReconnectState state;
    uint8_t retry_count;
    uint8_t max_retries;
    uint32_t last_attempt_ms;
    uint32_t connection_timeout_ms;
    uint8_t enabled;  // 1 = enabled, 0 = disabled
    uint8_t priority;  // 0-255, higher = more priority
    
    ReconnectProfile() 
        : state(ReconnectState::IDLE), retry_count(0), 
          max_retries(RECONNECT_MAX_RETRIES),
          last_attempt_ms(0), connection_timeout_ms(CONNECTION_TIMEOUT_MS),
          enabled(0), priority(0) {
        std::memset(address.addr, 0, MAC_ADDRESS_LEN);
    }
};
#pragma pack(pop)

class AutoReconnect {
public:
    static AutoReconnect& get_instance() {
        static AutoReconnect instance;
        return instance;
    }
    
    // Initialize auto-reconnect system
    void initialize();
    
    // Enable/disable auto-reconnect
    void enable_auto_reconnect(const BluetoothAddress& addr, uint8_t max_retries = RECONNECT_MAX_RETRIES);
    void disable_auto_reconnect(const BluetoothAddress& addr);
    bool is_auto_reconnect_enabled(const BluetoothAddress& addr) const;
    
    // Update reconnection state (call periodically from main loop)
    void update();
    
    // Connection feedback
    void on_connection_established(const BluetoothAddress& addr);
    void on_connection_failed(const BluetoothAddress& addr);
    void on_connection_lost(const BluetoothAddress& addr);
    
    // Get current state
    ReconnectState get_state(const BluetoothAddress& addr) const;
    uint8_t get_retry_count(const BluetoothAddress& addr) const;
    const BluetoothAddress* get_current_reconnect_target() const;
    
    // Configuration
    void set_retry_interval(uint32_t interval_ms) { retry_interval_ms_ = interval_ms; }
    void set_backoff_multiplier(uint32_t multiplier_ms) { backoff_multiplier_ms_ = multiplier_ms; }
    void set_connection_timeout(uint32_t timeout_ms) { connection_timeout_ms_ = timeout_ms; }
    void set_priority(const BluetoothAddress& addr, uint8_t priority);
    
    // Debug/monitoring
    uint8_t get_active_reconnect_count() const;
    bool is_reconnecting() const { return reconnecting_.load(); }
    
private:
    AutoReconnect() = default;
    ~AutoReconnect() = default;
    AutoReconnect(const AutoReconnect&) = delete;
    AutoReconnect& operator=(const AutoReconnect&) = delete;
    
    static constexpr uint8_t MAX_RECONNECT_PROFILES = 4;
    
    std::array<ReconnectProfile, MAX_RECONNECT_PROFILES> profiles_;
    uint8_t profile_count_ = 0;
    
    std::atomic<bool> reconnecting_{false};
    uint32_t last_update_ms_ = 0;
    uint32_t retry_interval_ms_ = RECONNECT_RETRY_INTERVAL_MS;
    uint32_t backoff_multiplier_ms_ = RECONNECT_BACKOFF_MULTIPLIER;
    uint32_t connection_timeout_ms_ = CONNECTION_TIMEOUT_MS;
    
    BluetoothAddress current_target_;
    
    int8_t find_profile(const BluetoothAddress& addr) const;
    bool should_attempt_reconnect(const ReconnectProfile& profile, uint32_t now_ms) const;
    uint32_t calculate_backoff_delay(uint8_t retry_count) const;
    void update_profile_state(ReconnectProfile& profile, uint32_t now_ms);
    const ReconnectProfile* get_highest_priority_profile() const;
};

} // namespace bluetooth
