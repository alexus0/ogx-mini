#include "Bluetooth/DeviceDiscovery.h"
#include "Board/ogxm_log.h"
#include <algorithm>
#include <cstring>

namespace bluetooth {

void DeviceDiscovery::start_discovery() {
    discovering_.store(true);
    discovered_count_ = 0;
    OGXM_LOG("Bluetooth: Device discovery started\n");
}

void DeviceDiscovery::stop_discovery() {
    discovering_.store(false);
    OGXM_LOG("Bluetooth: Device discovery stopped\n");
}

void DeviceDiscovery::add_discovered_device(const BluetoothAddress& addr, const char* name,
                                           int8_t rssi, uint16_t cod) {
    // Check RSSI threshold
    if (rssi < rssi_threshold_) {
        return;
    }
    
    // Check if it's a gamepad device
    if (!is_gamepad_device(cod)) {
        return;
    }
    
    // Look for existing device
    int8_t existing_idx = find_discovered_device(addr);
    
    if (existing_idx >= 0) {
        // Update existing device
        DiscoveredDevice& device = discovered_devices_[existing_idx];
        device.rssi = rssi;
        device.last_seen_ms = board_api::ms_since_boot();
        device.discovery_count++;
        
        OGXM_LOG("Bluetooth: Updated device %s (RSSI: %d dBm, Count: %d)\n", 
                 name, rssi, device.discovery_count);
    } else if (discovered_count_ < MAX_DISCOVERED_DEVICES) {
        // Add new device
        DiscoveredDevice& device = discovered_devices_[discovered_count_];
        device.address = addr;
        device.rssi = rssi;
        device.class_of_device = cod;
        device.last_seen_ms = board_api::ms_since_boot();
        device.discovery_count = 1;
        device.is_favorite = is_favorite(addr);
        
        std::strncpy(device.name, name, sizeof(device.name) - 1);
        device.name[sizeof(device.name) - 1] = '\0';
        
        discovered_count_++;
        
        OGXM_LOG("Bluetooth: Discovered new device %s (RSSI: %d dBm)\n", name, rssi);
    }
}

const DiscoveredDevice* DeviceDiscovery::get_discovered_device(uint8_t index) const {
    if (index >= discovered_count_) {
        return nullptr;
    }
    return &discovered_devices_[index];
}

void DeviceDiscovery::clear_discovered_devices() {
    discovered_count_ = 0;
    OGXM_LOG("Bluetooth: Discovered devices cleared\n");
}

bool DeviceDiscovery::add_favorite_device(const BluetoothAddress& addr, const char* name) {
    if (favorite_count_ >= MAX_FAVORITE_DEVICES) {
        OGXM_LOG("Bluetooth: Favorite devices list is full\n");
        return false;
    }
    
    // Check if already in favorites
    if (find_favorite_device(addr) >= 0) {
        OGXM_LOG("Bluetooth: Device already in favorites\n");
        return false;
    }
    
    FavoriteDevice& device = favorite_devices_[favorite_count_];
    device.address = addr;
    std::strncpy(device.name, name, sizeof(device.name) - 1);
    device.name[sizeof(device.name) - 1] = '\0';
    device.auto_connect = 0;
    device.last_connected_ms = 0;
    
    favorite_count_++;
    
    OGXM_LOG("Bluetooth: Added favorite device %s\n", name);
    return true;
}

bool DeviceDiscovery::remove_favorite_device(const BluetoothAddress& addr) {
    int8_t idx = find_favorite_device(addr);
    if (idx < 0) {
        return false;
    }
    
    // Shift remaining devices
    for (int8_t i = idx; i < static_cast<int8_t>(favorite_count_ - 1); ++i) {
        favorite_devices_[i] = favorite_devices_[i + 1];
    }
    
    favorite_count_--;
    OGXM_LOG("Bluetooth: Removed favorite device\n");
    return true;
}

const FavoriteDevice* DeviceDiscovery::get_favorite_device(uint8_t index) const {
    if (index >= favorite_count_) {
        return nullptr;
    }
    return &favorite_devices_[index];
}

bool DeviceDiscovery::is_favorite(const BluetoothAddress& addr) const {
    return find_favorite_device(addr) >= 0;
}

void DeviceDiscovery::set_auto_connect(const BluetoothAddress& addr, bool enabled) {
    int8_t idx = find_favorite_device(addr);
    if (idx >= 0) {
        favorite_devices_[idx].auto_connect = enabled ? 1 : 0;
        OGXM_LOG("Bluetooth: Auto-connect %s for favorite device\n", 
                 enabled ? "enabled" : "disabled");
    }
}

bool DeviceDiscovery::get_auto_connect(const BluetoothAddress& addr) const {
    int8_t idx = find_favorite_device(addr);
    if (idx >= 0) {
        return favorite_devices_[idx].auto_connect == 1;
    }
    return false;
}

const FavoriteDevice* DeviceDiscovery::get_next_auto_connect_device() const {
    for (uint8_t i = 0; i < favorite_count_; ++i) {
        if (favorite_devices_[i].auto_connect == 1) {
            return &favorite_devices_[i];
        }
    }
    return nullptr;
}

bool DeviceDiscovery::is_gamepad_device(uint16_t cod) const {
    // Class of Device bit filtering
    // Gamepad devices typically have these CoD classes
    uint8_t minor_class = (cod >> 2) & 0x3F;
    uint8_t major_class = (cod >> 8) & 0x1F;
    
    // Major class 5 = Peripheral
    // Minor classes: gamepad (0x08), joystick (0x04), etc.
    if (major_class == 5) {
        // Accept gamepads, joysticks, and other peripherals
        return (minor_class & 0x0C) != 0;  // Gamepad or Joystick bits
    }
    
    return false;
}

int8_t DeviceDiscovery::find_discovered_device(const BluetoothAddress& addr) const {
    for (uint8_t i = 0; i < discovered_count_; ++i) {
        if (discovered_devices_[i].address == addr) {
            return i;
        }
    }
    return -1;
}

int8_t DeviceDiscovery::find_favorite_device(const BluetoothAddress& addr) const {
    for (uint8_t i = 0; i < favorite_count_; ++i) {
        if (favorite_devices_[i].address == addr) {
            return i;
        }
    }
    return -1;
}

} // namespace bluetooth
