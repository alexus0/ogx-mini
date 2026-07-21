#include "Bluetooth/AutoReconnect.h"
#include "Board/board_api.h"
#include "Board/ogxm_log.h"
#include <algorithm>
#include <cstring>

namespace bluetooth {

void AutoReconnect::initialize() {
    profile_count_ = 0;
    reconnecting_.store(false);
    last_update_ms_ = board_api::ms_since_boot();
    OGXM_LOG("Bluetooth: Auto-reconnect system initialized\n");
}

void AutoReconnect::enable_auto_reconnect(const BluetoothAddress& addr, uint8_t max_retries) {
    int8_t idx = find_profile(addr);
    
    if (idx >= 0) {
        // Update existing profile
        profiles_[idx].enabled = 1;
        profiles_[idx].max_retries = max_retries;
        profiles_[idx].retry_count = 0;
        profiles_[idx].state = ReconnectState::SEARCHING;
        profiles_[idx].last_attempt_ms = board_api::ms_since_boot();
        
        OGXM_LOG("Bluetooth: Auto-reconnect enabled for existing device (Max retries: %d)\n", max_retries);
    } else if (profile_count_ < MAX_RECONNECT_PROFILES) {
        // Add new profile
        ReconnectProfile& profile = profiles_[profile_count_];
        profile.address = addr;
        profile.enabled = 1;
        profile.max_retries = max_retries;
        profile.retry_count = 0;
        profile.state = ReconnectState::SEARCHING;
        profile.last_attempt_ms = board_api::ms_since_boot();
        profile.priority = 0;
        
        profile_count_++;
        reconnecting_.store(true);
        
        OGXM_LOG("Bluetooth: Auto-reconnect enabled for new device (Max retries: %d)\n", max_retries);
    } else {
        OGXM_LOG("Bluetooth: Auto-reconnect profiles list is full\n");
    }
}

void AutoReconnect::disable_auto_reconnect(const BluetoothAddress& addr) {
    int8_t idx = find_profile(addr);
    if (idx >= 0) {
        profiles_[idx].enabled = 0;
        profiles_[idx].state = ReconnectState::DISABLED;
        OGXM_LOG("Bluetooth: Auto-reconnect disabled\n");
    }
}

bool AutoReconnect::is_auto_reconnect_enabled(const BluetoothAddress& addr) const {
    int8_t idx = find_profile(addr);
    if (idx >= 0) {
        return profiles_[idx].enabled == 1;
    }
    return false;
}

void AutoReconnect::update() {
    uint32_t now_ms = board_api::ms_since_boot();
    
    // Update all profiles
    bool any_active = false;
    for (uint8_t i = 0; i < profile_count_; ++i) {
        if (profiles_[i].enabled == 1) {
            update_profile_state(profiles_[i], now_ms);
            if (profiles_[i].state != ReconnectState::DISABLED) {
                any_active = true;
            }
        }
    }
    
    reconnecting_.store(any_active);
    last_update_ms_ = now_ms;
}

void AutoReconnect::on_connection_established(const BluetoothAddress& addr) {
    int8_t idx = find_profile(addr);
    if (idx >= 0) {
        profiles_[idx].state = ReconnectState::CONNECTED;
        profiles_[idx].retry_count = 0;
        profiles_[idx].last_attempt_ms = board_api::ms_since_boot();
        
        OGXM_LOG("Bluetooth: Connection established\n");
    }
}

void AutoReconnect::on_connection_failed(const BluetoothAddress& addr) {
    int8_t idx = find_profile(addr);
    if (idx >= 0) {
        profiles_[idx].retry_count++;
        
        if (profiles_[idx].retry_count >= profiles_[idx].max_retries) {
            profiles_[idx].state = ReconnectState::FAILED;
            OGXM_LOG("Bluetooth: Connection failed - max retries exceeded\n");
        } else {
            profiles_[idx].state = ReconnectState::BACKOFF;
            profiles_[idx].last_attempt_ms = board_api::ms_since_boot();
            OGXM_LOG("Bluetooth: Connection failed - retry %d/%d\n", 
                     profiles_[idx].retry_count, profiles_[idx].max_retries);
        }
    }
}

void AutoReconnect::on_connection_lost(const BluetoothAddress& addr) {
    int8_t idx = find_profile(addr);
    if (idx >= 0) {
        if (profiles_[idx].enabled == 1) {
            profiles_[idx].state = ReconnectState::SEARCHING;
            profiles_[idx].retry_count = 0;
            profiles_[idx].last_attempt_ms = board_api::ms_since_boot();
            
            OGXM_LOG("Bluetooth: Connection lost - attempting to reconnect\n");
        }
    }
}

ReconnectState AutoReconnect::get_state(const BluetoothAddress& addr) const {
    int8_t idx = find_profile(addr);
    if (idx >= 0) {
        return profiles_[idx].state;
    }
    return ReconnectState::IDLE;
}

uint8_t AutoReconnect::get_retry_count(const BluetoothAddress& addr) const {
    int8_t idx = find_profile(addr);
    if (idx >= 0) {
        return profiles_[idx].retry_count;
    }
    return 0;
}

const BluetoothAddress* AutoReconnect::get_current_reconnect_target() const {
    const ReconnectProfile* profile = get_highest_priority_profile();
    if (profile != nullptr && profile->state == ReconnectState::SEARCHING) {
        return &profile->address;
    }
    return nullptr;
}

void AutoReconnect::set_priority(const BluetoothAddress& addr, uint8_t priority) {
    int8_t idx = find_profile(addr);
    if (idx >= 0) {
        profiles_[idx].priority = priority;
    }
}

uint8_t AutoReconnect::get_active_reconnect_count() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < profile_count_; ++i) {
        if (profiles_[i].enabled == 1 && 
            profiles_[i].state != ReconnectState::CONNECTED &&
            profiles_[i].state != ReconnectState::DISABLED) {
            count++;
        }
    }
    return count;
}

int8_t AutoReconnect::find_profile(const BluetoothAddress& addr) const {
    for (uint8_t i = 0; i < profile_count_; ++i) {
        if (profiles_[i].address == addr) {
            return i;
        }
    }
    return -1;
}

bool AutoReconnect::should_attempt_reconnect(const ReconnectProfile& profile, uint32_t now_ms) const {
    if (profile.enabled == 0 || profile.retry_count >= profile.max_retries) {
        return false;
    }
    
    uint32_t backoff_delay = calculate_backoff_delay(profile.retry_count);
    return (now_ms - profile.last_attempt_ms) >= backoff_delay;
}

uint32_t AutoReconnect::calculate_backoff_delay(uint8_t retry_count) const {
    // Exponential backoff: retry_interval * (2 ^ retry_count)
    uint32_t delay = retry_interval_ms_;
    for (uint8_t i = 0; i < retry_count; ++i) {
        delay = (delay * 2 > backoff_multiplier_ms_) ? backoff_multiplier_ms_ : delay * 2;
    }
    return delay;
}

void AutoReconnect::update_profile_state(ReconnectProfile& profile, uint32_t now_ms) {
    switch (profile.state) {
        case ReconnectState::SEARCHING:
            if (should_attempt_reconnect(profile, now_ms)) {
                profile.state = ReconnectState::CONNECTING;
                profile.last_attempt_ms = now_ms;
                OGXM_LOG("Bluetooth: Attempting to connect (attempt %d)\n", profile.retry_count + 1);
            }
            break;
            
        case ReconnectState::CONNECTING:
            // Check if connection attempt has timed out
            if ((now_ms - profile.last_attempt_ms) > profile.connection_timeout_ms) {
                profile.retry_count++;
                if (profile.retry_count >= profile.max_retries) {
                    profile.state = ReconnectState::FAILED;
                    OGXM_LOG("Bluetooth: Connection attempt timed out - max retries exceeded\n");
                } else {
                    profile.state = ReconnectState::BACKOFF;
                    OGXM_LOG("Bluetooth: Connection attempt timed out - will retry\n");
                }
            }
            break;
            
        case ReconnectState::BACKOFF:
            if (should_attempt_reconnect(profile, now_ms)) {
                profile.state = ReconnectState::SEARCHING;
                OGXM_LOG("Bluetooth: Backoff period completed - resuming search\n");
            }
            break;
            
        case ReconnectState::CONNECTED:
        case ReconnectState::FAILED:
        case ReconnectState::DISABLED:
        case ReconnectState::IDLE:
        default:
            break;
    }
}

const ReconnectProfile* AutoReconnect::get_highest_priority_profile() const {
    const ReconnectProfile* highest = nullptr;
    uint8_t max_priority = 0;
    
    for (uint8_t i = 0; i < profile_count_; ++i) {
        if (profiles_[i].enabled == 1 && 
            profiles_[i].state == ReconnectState::SEARCHING &&
            profiles_[i].priority >= max_priority) {
            highest = &profiles_[i];
            max_priority = profiles_[i].priority;
        }
    }
    
    return highest;
}

} // namespace bluetooth
