#include "daikin_x10a.h"
#include "register_definitions.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "daikin_package.h"

#include <vector>
#include <string>
#include <set>
#include <cstdlib>

namespace esphome {
namespace daikin_x10a {

static const char *const TAG = "daikin_x10a";

DaikinX10A::~DaikinX10A() = default;

static constexpr uint32_t Serial_TimeoutInMilliseconds = 300;

void DaikinX10A::setup() {
  ESP_LOGI(TAG, "Daikin X10A component initialized with %u registers",
           (unsigned)registers_.size());
}

void DaikinX10A::dump_config() {
  ESP_LOGCONFIG(TAG, "Daikin X10A:");
  ESP_LOGCONFIG(TAG, "  Registers: %u", (unsigned)registers_.size());
  ESP_LOGCONFIG(TAG, "  Scan interval: %u ms", REGISTER_SCAN_INTERVAL_MS);
}

//__________ loop begin
void DaikinX10A::loop() {
  static uint32_t start = 0;
  static bool initialDelayDone = false;

  if (!initialDelayDone) {
    if (millis() >= 15000) {
      start = millis();
      initialDelayDone = true;
    }
    return;
  }

  if (millis() - start >= REGISTER_SCAN_INTERVAL_MS) {
    start = millis();
    this->FetchRegisters();
  }
}

//__________ FetchRegisters begin
// Called every REGISTER_SCAN_INTERVAL_MS milliseconds; loops over all registers
// to fetch values from the heat pump via UART.
// Both mode=0 and mode=1 registers are fetched; mode only controls HA sensor creation.
void DaikinX10A::FetchRegisters() {
  std::set<uint8_t> fetched_registries;

  for (const auto& selectedRegister : registers_) {
    // Skip if we've already fetched this registryID in this scan cycle
    if (fetched_registries.count(selectedRegister.registryID) > 0) {
      continue;
    }

    // Mark this registryID as fetched
    fetched_registries.insert(selectedRegister.registryID);

    auto MyDaikinRequestPackage = daikin_package::MakeRequest(selectedRegister.registryID);

    ESP_LOGD(TAG, "TX (%u): %s", (unsigned)MyDaikinRequestPackage.size(),
             MyDaikinRequestPackage.ToHexString().c_str());

    this->flush();
    for (uint8_t requestByte : MyDaikinRequestPackage.buffer()) this->write(requestByte);

    daikin_package MyDaikinPackage(daikin_package::Mode::RECEIVE);

    const uint32_t FetchRegistersStartMilliseconds = millis();
    size_t IncommingPackageSize = 3;
    uint8_t target_registry = selectedRegister.registryID;

    // NOTE: This busy-wait loop blocks the main loop for up to
    // Serial_TimeoutInMilliseconds per registry request. A non-blocking
    // state machine approach would be more ESPHome-idiomatic but requires
    // significant architectural changes.
    while (millis() - FetchRegistersStartMilliseconds < Serial_TimeoutInMilliseconds) {
      if (!this->available()) continue;

      uint8_t incomingByte;
      this->read_byte(&incomingByte);

      // Skip if not protocol marker (0x40)
      if (MyDaikinPackage.empty() && incomingByte != 0x40) continue;

      MyDaikinPackage.buffer_mut().push_back(incomingByte);

      // Once we have 2 bytes, check if it's the right registry
      if (MyDaikinPackage.size() == 2) {
        uint8_t received_registry = MyDaikinPackage.buffer()[1];
        if (received_registry != target_registry) {
          ESP_LOGD(TAG, "  Received registry 0x%02X (expected 0x%02X), discarding...",
                   received_registry, target_registry);
          MyDaikinPackage.clear();
          continue;
        }
      }

      // Once we have 3 bytes, we know the full length
      if (MyDaikinPackage.HasMinimalHeader()) {
        const size_t expectedSize = MyDaikinPackage.expected_size();
        if (expectedSize > 0) IncommingPackageSize = expectedSize;
      }

      // Early error detection
      if (MyDaikinPackage.is_error_frame()) {
        ESP_LOGW(TAG, "HP returned error frame: %s", MyDaikinPackage.ToHexString().c_str());
        return;
      }

      // Check if we have complete packet
      if (MyDaikinPackage.size() >= IncommingPackageSize) break;
    }

    if (MyDaikinPackage.empty()) {
      ESP_LOGD(TAG, "No valid response for registry 0x%02X (timeout)", target_registry);
      continue;
    }

    if (!MyDaikinPackage.Valid_CRC()) {
      ESP_LOGW(TAG, "CRC mismatch (%u): %s", (unsigned)MyDaikinPackage.size(),
               MyDaikinPackage.ToHexString().c_str());
      continue;
    }

    ESP_LOGD(TAG, "RX (%u): %s", (unsigned)MyDaikinPackage.size(),
             MyDaikinPackage.ToHexString().c_str());
    last_requested_registry_ = selectedRegister.registryID;
    this->process_frame_(MyDaikinPackage);
  }
}

//__________ process_frame_ begin
void DaikinX10A::process_frame_(daikin_package &pkg) {
  if (!pkg.is_valid_protocol() || !pkg.Valid_CRC() || pkg.is_error_frame()) return;

  const auto &buffer = pkg.buffer();
  if (buffer.size() < 6) return;

  uint8_t registry_id = buffer[1];

  ESP_LOGD(TAG, "Decoding registry 0x%02X, data starts at byte 3", registry_id);

  pkg.convert_registry_values(registry_id, registers_);

  last_successful_read_ms_ = millis();

  int count = 0;
  for (auto &registerEntry : registers_) {
    if ((uint8_t)registerEntry.registryID != registry_id) continue;
    if (registerEntry.asString[0] == '\0') continue;
    ESP_LOGD(TAG, "  0x%02X | %s = %s", registry_id, registerEntry.label, registerEntry.asString);

    // Auto-update dynamic sensors for mode=1 registers
    if (registerEntry.Mode == 1) {
      if (registerEntry.is_text_type()) {
        update_text_sensor(registerEntry.label, registerEntry.asString);
      } else {
        float value = std::atof(registerEntry.asString);
        update_sensor(registerEntry.label, value);
      }
    }

    count++;
  }

  ESP_LOGD(TAG, "Decoded %d values for registry 0x%02X", count, registry_id);

  // Fire update callbacks (used by climate entity, etc.)
  for (auto &cb : update_callbacks_) {
    cb();
  }
}

//__________ add_register begin
void DaikinX10A::add_register(int mode, int convid, int offset, int registryID,
                             int dataSize, int dataType, const char* label) {
  registers_.emplace_back(mode, convid, offset, registryID, dataSize, dataType, label);
}

//__________ get_register_value begin
std::string DaikinX10A::get_register_value(const std::string& label) const {
  for (const auto &reg : registers_) {
    if (reg.label && label == reg.label && reg.asString[0] != '\0') {
      return std::string(reg.asString);
    }
  }
  return "";
}

//__________ register_dynamic_sensor begin
void DaikinX10A::register_dynamic_sensor(const std::string& label, sensor::Sensor *sens) {
  dynamic_sensors_[label] = sens;
  ESP_LOGI(TAG, "Registered dynamic sensor: %s", label.c_str());
}

//__________ update_sensor begin
void DaikinX10A::update_sensor(const std::string& label, float value) {
  auto it = dynamic_sensors_.find(label);
  if (it != dynamic_sensors_.end() && it->second != nullptr) {
    it->second->publish_state(value);
    ESP_LOGV(TAG, "Updated sensor '%s' = %.1f", label.c_str(), value);
  }
}

//__________ register_dynamic_text_sensor begin
void DaikinX10A::register_dynamic_text_sensor(const std::string& label, text_sensor::TextSensor *sens) {
  dynamic_text_sensors_[label] = sens;
  ESP_LOGI(TAG, "Registered dynamic text sensor: %s", label.c_str());
}

//__________ update_text_sensor begin
void DaikinX10A::update_text_sensor(const std::string& label, const std::string& value) {
  auto it = dynamic_text_sensors_.find(label);
  if (it != dynamic_text_sensors_.end() && it->second != nullptr) {
    it->second->publish_state(value);
    ESP_LOGV(TAG, "Updated text sensor '%s' = %s", label.c_str(), value.c_str());
  }
}

}  // namespace daikin_x10a
}  // namespace esphome
