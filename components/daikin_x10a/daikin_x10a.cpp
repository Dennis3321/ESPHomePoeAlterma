#include "daikin_x10a.h"
#include "register_definitions.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "daikin_package.h"

#include <vector>
#include <string>

namespace esphome {
namespace daikin_x10a {

DaikinX10A::~DaikinX10A() = default;

static constexpr uint32_t Serial_TimeoutInMilliseconds = 300;


// =======================================================
// 3) Main loop / frame processing
// =======================================================

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






                 


void DaikinX10A::FetchRegisters() {
  //____________________________________
  for (const auto& selectedRegister : registers) {
    if (selectedRegister.Mode == 1) {
      auto MyDaikinRequestPackage = daikin_package::MakeRequest(selectedRegister.registryID);

      ESP_LOGI("ESPoeDaikin", "TX (%u): %s", (unsigned)MyDaikinRequestPackage.size(), MyDaikinRequestPackage.hex_dump().c_str());

      this->flush();
      for (uint8_t b : MyDaikinRequestPackage.buffer()) this->write(b);  // for each every byte in the buffer, send the request bytes via de UART

      daikin_package MyDaikinPackage(daikin_package::Mode::RECEIVE);  // create a new receive package

      const uint32_t start_ms = millis();

      size_t IncommingPackageSize = 3; // until we know expected_size()
      while (millis() - start_ms < Serial_TimeoutInMilliseconds) {
        while (this->available()) {
          uint8_t b;
          this->read_byte(&b);

          // your original rule: first byte must be 0x40 (otherwise skip)
          if (MyDaikinPackage.empty() && b != 0x40) continue;

          MyDaikinPackage.buffer_mut().push_back(b);

          // once we have 3 bytes, we know full length
          if (MyDaikinPackage.has_min_header()) {
            const size_t exp = MyDaikinPackage.expected_size();
            if (exp > 0) IncommingPackageSize = exp;
          }

          // early error detection (optional)
          if (MyDaikinPackage.is_error_frame()) {
            ESP_LOGI("ESPoeDaikin", "HP returned error frame: %s", MyDaikinPackage.hex_dump().c_str());
            return;
          }

          if (MyDaikinPackage.size() >= IncommingPackageSize) break;
        }

        if (MyDaikinPackage.size() >= IncommingPackageSize) break;
      }

      if (MyDaikinPackage.empty()) return;

      if (!MyDaikinPackage.crc_ok()) {
        ESP_LOGI("ESPoeDaikin", "CRC mismatch (%u): %s", (unsigned)MyDaikinPackage.size(), MyDaikinPackage.hex_dump().c_str());
        delay(250);
        return;
      }

      ESP_LOGI("ESPoeDaikin", "MyDaikinPackage (%u): %s", (unsigned)MyDaikinPackage.size(), MyDaikinPackage.hex_dump().c_str());
      last_requested_registry_ = selectedRegister.registryID;
      this->process_frame_(MyDaikinPackage, selectedRegister); // of beter: process_frame_(MyDaikinPackage) met overload
    }
  }
  //____________________________________
}

void DaikinX10A::process_frame_(daikin_package &pkg, const Register& selectedRegister) {
  if (!pkg.is_valid_protocol() || !pkg.crc_ok() || pkg.is_error_frame()) return;

  const auto &b = pkg.buffer();
  if (b.size() < 6) return;

  uint8_t registry_id = selectedRegister.registryID;

  ESP_LOGI("ESPoeDaikin", "Decode registry_id=%d (0x%02X) base_offset=4", (int)registry_id, registry_id);

  pkg.convert_registry_values(registry_id);  // pass the protocol convid

  // log alle regels die bij deze registry horen (en non-empty asString hebben)
  int count = 0;
  for (auto &r : registers) {
    if ((uint8_t)r.registryID != registry_id) continue;
    if (r.asString[0] == '\0') continue;
    ESP_LOGI("ESPoeDaikin", "0x%02X | %s = %s", registry_id, r.label, r.asString);
    count++;
  }

  ESP_LOGI("ESPoeDaikin", "Decoded %d values for registry 0x%02X", count, registry_id);
}



void DaikinX10A::add_register(int mode, int convid, int offset, int registryID,
                             int dataSize, int dataType, const char* label) {
  registers.emplace_back(mode, convid, offset, registryID, dataSize, dataType, label);
}


}  // namespace daikin_x10a
}  // namespace esphome
