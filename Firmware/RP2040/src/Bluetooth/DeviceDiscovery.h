#pragma once

#include <cstdint>
#include <array>
#include <cstring>
#include <atomic>
#include "Board/Config.h"

namespace bluetooth {

constexpr uint8_t MAX_DISCOVERED_DEVICES = 20;
constexpr uint8_t MAC_ADDRESS_LEN = 6;
constexpr uint32_t DEVICE_DISCOVERY_TIMEOUT_MS = 10000;
constexpr uint8_t MAX_FAVORITE_DEVICES = 8;

#pragma pack(push, 1)
struct BluetoothAddress {
    uint8_t addr[MAC_ADDRESS_LEN];
    
    bool operator==(const BluetoothAddress& other) const {
        return std::memcmp(addr, other.addr, MAC_ADDRESS_LEN) == 0;
    }
    
    bool is_valid() const {
        for (int i = 0; i < MAC_ADDRESS_LEN; ++i) {
            if (addr[i] != 0) return true;
        }
        return false;
    }
};

struct DiscoveredDevice {
    BluetoothAddress address;
    char name[32];
    int8_t rssi;
    uint16_t class_of_device;
    uint32_t last_seen_ms;
    uint8_t discovery_count;  // How many times we've seen this device
    bool is_favorite;
    
    DiscoveredDevice() : rssi(-127), class_of_device(0), last_seen_ms(0), 
                         discovery_count(0), is_favorite(false) {
        std::memset(name, 0, sizeof(name));
        std::memset(address.addr, 0, MAC_ADDRESS_LEN);
    }
};

struct FavoriteDevice {
    BluetoothAddress address;
    char name[32];
    uint8_t auto_connect;  // 1 = auto-connect, 0 = manual
    uint32_t last_connected_ms;
    
    FavoriteDevice() : auto_connect(0), last_connected_ms(0) {
        std::memset(name, 0, sizeof(name));
        std::memset(address.addr, 0, MAC_ADDRESS_LEN);
    }
};
#pragma pack(pop)

class DeviceDiscovery {
public:
    static DeviceDiscovery& get_instance() {
        static DeviceDiscovery instance;
        return instance;
    }
    
    // Discovery management
    void start_discovery();
    void stop_discovery();
    bool is_discovering() const { return discovering_.load(); }
    
    // Device management
    void add_discovered_device(const BluetoothAddress& addr, const char* name, 
                              int8_t rssi, uint16_t cod);
    const DiscoveredDevice* get_discovered_device(uint8_t index) const;
    uint8_t get_discovered_device_count() const { return discovered_count_; }
    void clear_discovered_devices();
    
    // Favorite devices management
    bool add_favorite_device(const BluetoothAddress& addr, const char* name);
    bool remove_favorite_device(const BluetoothAddress& addr);
    const FavoriteDevice* get_favorite_device(uint8_t index) const;
    uint8_t get_favorite_device_count() const { return favorite_count_; }
    bool is_favorite(const BluetoothAddress& addr) const;
    
    // Auto-connect management
    void set_auto_connect(const BluetoothAddress& addr, bool enabled);
    bool get_auto_connect(const BluetoothAddress& addr) const;
    const FavoriteDevice* get_next_auto_connect_device() const;
    
    // Device filtering
    bool is_gamepad_device(uint16_t cod) const;
    void set_rssi_threshold(int8_t threshold) { rssi_threshold_ = threshold; }
    
private:
    DeviceDiscovery() = default;
    ~DeviceDiscovery() = default;
    DeviceDiscovery(const DeviceDiscovery&) = delete;
    DeviceDiscovery& operator=(const DeviceDiscovery&) = delete;
    
    std::array<DiscoveredDevice, MAX_DISCOVERED_DEVICES> discovered_devices_;
    std::array<FavoriteDevice, MAX_FAVORITE_DEVICES> favorite_devices_;
    
    uint8_t discovered_count_ = 0;
    uint8_t favorite_count_ = 0;
    std::atomic<bool> discovering_{false};
    int8_t rssi_threshold_ = -100;  // dBm threshold for device discovery
    
    int8_t find_discovered_device(const BluetoothAddress& addr) const;
    int8_t find_favorite_device(const BluetoothAddress& addr) const;
};

} // namespace bluetooth
