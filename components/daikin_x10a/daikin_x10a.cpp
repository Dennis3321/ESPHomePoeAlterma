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
      for (uint8_t requestByte : MyDaikinRequestPackage.buffer()) this->write(requestByte);  // for each every byte in the buffer, send the request bytes via de UART

      daikin_package MyDaikinPackage(daikin_package::Mode::RECEIVE);  // create a new receive MyDaikinPackage

      const uint32_t FetchRegistersStartMilliseconds = millis();

      size_t IncommingPackageSize = 3; // until we know expected_size()
      while (millis() - FetchRegistersStartMilliseconds < Serial_TimeoutInMilliseconds) {
        if (!this->available()) continue;  // No data available, try again on next loop
        
        uint8_t incomingByte;  // create a variable to hold the incoming byte
        this->read_byte(&incomingByte);  // read a byte from the UART into variable incomingByte

        // your original rule: first byte must be 0x40 (otherwise skip)
        if (MyDaikinPackage.empty() && incomingByte != 0x40) continue;

        MyDaikinPackage.buffer_mut().push_back(incomingByte);

        // once we have 3 bytes, we know full length
        if (MyDaikinPackage.HasMinimalHeader()) {
          const size_t expectedSize = MyDaikinPackage.expected_size();
          if (expectedSize > 0) IncommingPackageSize = expectedSize;
        }

        // early error detection (optional)
        if (MyDaikinPackage.is_error_frame()) {
          ESP_LOGI("ESPoeDaikin", "HP returned error frame: %s", MyDaikinPackage.ToHexString().c_str());
          return;
        }

        if (MyDaikinPackage.size() >= IncommingPackageSize) break;
      }

      if (MyDaikinPackage.empty()) return;

      if (!MyDaikinPackage.Valid_CRC()) {
        ESP_LOGI("ESPoeDaikin", "CRC mismatch (%u): %s", (unsigned)MyDaikinPackage.size(), MyDaikinPackage.ToHexString().c_str());
        delay(250);
        return;
      }

      ESP_LOGI("ESPoeDaikin", "MyDaikinPackage (%u): %s", (unsigned)MyDaikinPackage.size(), MyDaikinPackage.ToHexString().c_str());
      last_requested_registry_ = selectedRegister.registryID;
      this->process_frame_(MyDaikinPackage, selectedRegister); // of beter: process_frame_(MyDaikinPackage) met overload
    } // if mode==1

  } //____________________________________ end for loop loop over all registers
  
}
//________________________________________________________________ FetchRegisters end

//__________________________________________________________________________________________________________________________ process_frame_ begin
void DaikinX10A::process_frame_(daikin_package &pkg, const Register& selectedRegister) {
  if (!pkg.is_valid_protocol() || !pkg.Valid_CRC() || pkg.is_error_frame()) return;

  const auto &buffer = pkg.buffer();
  if (buffer.size() < 6) return;

  uint8_t registry_id = selectedRegister.registryID;

  ESP_LOGI("ESPoeDaikin", "Decode registry_id=%d (0x%02X), protocol_header=3_bytes (0x40, regID, length), data_starts_at_byte_3", (int)registry_id, registry_id);

  pkg.convert_registry_values(registry_id);  // pass the protocol convid

  // log alle regels die bij deze registry horen (en non-empty asString hebben)
  int count = 0;
  for (auto &registerEntry : registers) {
    if ((uint8_t)registerEntry.registryID != registry_id) continue;
    if (registerEntry.asString[0] == '\0') continue;
    ESP_LOGI("ESPoeDaikin", "0x%02X | %s = %s", registry_id, registerEntry.label, registerEntry.asString);
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

}  // namespace daikin_x10a
}  // namespace esphome
