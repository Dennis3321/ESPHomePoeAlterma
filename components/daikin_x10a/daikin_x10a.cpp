#include "daikin_x10a.h"
#include "register_definitions.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "daikin_package.h"

#include <vector>
#include <string>
#include <cstdlib>  // for atof

namespace esphome {
namespace daikin_x10a {

DaikinX10A::~DaikinX10A() = default;

static constexpr uint32_t Serial_TimeoutInMilliseconds = 300;

//__________________________________________________________________________________________________________________________ loop begin
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
//________________________________________________________________ loop end

//__________________________________________________________________________________________________________________________ FetchRegisters begin
// FetchRegisters() is called every REGISTER_SCAN_INTERVAL_MS milliseconds, and loops over all registers with Mode==1 (read) to fetch their values from the HP via UART
void DaikinX10A::FetchRegisters() {
  
  for (const auto& selectedRegister : registers) {  //____________________________________ loop over all registers
    if (selectedRegister.Mode == 1) {
      auto MyDaikinRequestPackage = daikin_package::MakeRequest(selectedRegister.registryID);

      ESP_LOGI("ESPoeDaikin", "TX (%u): %s", (unsigned)MyDaikinRequestPackage.size(), MyDaikinRequestPackage.ToHexString().c_str());

      this->flush();  // Clear the serial buffer before sending
      for (uint8_t requestByte : MyDaikinRequestPackage.buffer()) this->write(requestByte);

      daikin_package MyDaikinPackage(daikin_package::Mode::RECEIVE);

      const uint32_t FetchRegistersStartMilliseconds = millis();
      size_t IncommingPackageSize = 3;
      uint8_t target_registry = selectedRegister.registryID;
      
      // Retry loop: keep reading until we get the CORRECT registry response or timeout
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
            // Wrong registry - discard this packet and wait for the right one
            ESP_LOGI("ESPoeDaikin", "  Received registry 0x%02X (expected 0x%02X), discarding...", received_registry, target_registry);
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
          ESP_LOGI("ESPoeDaikin", "HP returned error frame: %s", MyDaikinPackage.ToHexString().c_str());
          return;
        }

        // Check if we have complete packet
        if (MyDaikinPackage.size() >= IncommingPackageSize) break;
      }

      if (MyDaikinPackage.empty()) {
        ESP_LOGI("ESPoeDaikin", "No valid response for registry 0x%02X (timeout)", target_registry);
        continue;
      }

      if (!MyDaikinPackage.Valid_CRC()) {
        ESP_LOGI("ESPoeDaikin", "CRC mismatch (%u): %s", (unsigned)MyDaikinPackage.size(), MyDaikinPackage.ToHexString().c_str());
        continue;
      }

      ESP_LOGI("ESPoeDaikin", "MyDaikinPackage (%u): %s", (unsigned)MyDaikinPackage.size(), MyDaikinPackage.ToHexString().c_str());
      last_requested_registry_ = selectedRegister.registryID;
      this->process_frame_(MyDaikinPackage);
    } // if mode==1

  } //____________________________________ end for loop loop over all registers
  
}
//________________________________________________________________ FetchRegisters end

//__________________________________________________________________________________________________________________________ process_frame_ begin
void DaikinX10A::process_frame_(daikin_package &pkg) {
  if (!pkg.is_valid_protocol() || !pkg.Valid_CRC() || pkg.is_error_frame()) return;

  const auto &buffer = pkg.buffer();
  if (buffer.size() < 6) return;

  // Registry ID is at byte 1
  uint8_t registry_id = buffer[1];

  ESP_LOGI("ESPoeDaikin", "Decode registry_id=%d (0x%02X), protocol_header=3_bytes (0x40, regID, length), data_starts_at_byte_3", (int)registry_id, registry_id);

  pkg.convert_registry_values(registry_id);  // pass the protocol convid

  // log alle regels die bij deze registry horen (en non-empty asString hebben)
  int count = 0;
  for (auto &registerEntry : registers) {
    if ((uint8_t)registerEntry.registryID != registry_id) continue;
    if (registerEntry.asString[0] == '\0') continue;
    ESP_LOGI("ESPoeDaikin", "0x%02X | %s = %s", registry_id, registerEntry.label, registerEntry.asString);

    // AUTO-UPDATE DYNAMIC SENSORS for mode=1 registers
    if (registerEntry.Mode == 1) {
      float value = std::atof(registerEntry.asString);
      update_sensor(registerEntry.label, value);
    }

    count++;
  }

  ESP_LOGI("ESPoeDaikin", "Decoded %d values for registry 0x%02X", count, registry_id);
}
//________________________________________________________________ process_frame_ end

//__________________________________________________________________________________________________________________________ add_register begin
void DaikinX10A::add_register(int mode, int convid, int offset, int registryID,
                             int dataSize, int dataType, const char* label) {
  registers.emplace_back(mode, convid, offset, registryID, dataSize, dataType, label);
}
//________________________________________________________________ add_register end

//__________________________________________________________________________________________________________________________ get_register_value begin
std::string DaikinX10A::get_register_value(const std::string& label) const {
  for (const auto &reg : registers) {
    if (reg.label && reg.label == label && reg.asString[0] != '\0') {
      return std::string(reg.asString);
    }
  }
  return "";  // Return empty string if register not found or value is empty
}
//________________________________________________________________ get_register_value end

//__________________________________________________________________________________________________________________________ register_dynamic_sensor begin
void DaikinX10A::register_dynamic_sensor(const std::string& label, sensor::Sensor *sens) {
  dynamic_sensors_[label] = sens;
  ESP_LOGI("ESPoeDaikin", "Registered dynamic sensor: %s", label.c_str());
}
//________________________________________________________________ register_dynamic_sensor end

//__________________________________________________________________________________________________________________________ update_sensor begin
void DaikinX10A::update_sensor(const std::string& label, float value) {
  auto it = dynamic_sensors_.find(label);
  if (it != dynamic_sensors_.end() && it->second != nullptr) {
    it->second->publish_state(value);
    ESP_LOGD("ESPoeDaikin", "Updated sensor '%s' = %.1f", label.c_str(), value);
  }
}
//________________________________________________________________ update_sensor end

}  // namespace daikin_x10a
}  // namespace esphome
