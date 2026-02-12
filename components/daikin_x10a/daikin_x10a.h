#pragma once
#include "esphome/core/component.h"
#include "esphome/core/application.h"
#include "esphome/core/entity_base.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <vector>
#include <string>
#include <map>
#include <functional>
#include "daikin_package.h"
#include "register_definitions.h"

namespace esphome {
namespace daikin_x10a {

class DaikinX10A : public uart::UARTDevice, public Component {
 public:
  explicit DaikinX10A(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}
  virtual ~DaikinX10A();

  void setup() override;
  void dump_config() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void FetchRegisters();
  void add_register(int mode, int convid, int offset, int registryID, int dataSize, int dataType, const char* label);

  // Get register value by label name (for template sensors)
  std::string get_register_value(const std::string& label) const;

  // Dynamic numeric sensor management
  void register_dynamic_sensor(const std::string& label, sensor::Sensor *sens);
  void update_sensor(const std::string& label, float value);

  // Dynamic text sensor management
  void register_dynamic_text_sensor(const std::string& label, text_sensor::TextSensor *sens);
  void update_text_sensor(const std::string& label, const std::string& value);

  // Callback fired after each registry scan completes
  void add_on_update_callback(std::function<void()> &&callback) { update_callbacks_.push_back(std::move(callback)); }

  // Last successful communication timestamp (for diagnostics)
  uint32_t last_successful_read_ms() const { return last_successful_read_ms_; }

 protected:
  std::vector<Register> registers_;
  std::vector<uint8_t> buffer_;
  uint8_t last_requested_registry_{0};
  uint32_t last_successful_read_ms_{0};

  std::map<std::string, sensor::Sensor*> dynamic_sensors_;
  std::map<std::string, text_sensor::TextSensor*> dynamic_text_sensors_;
  std::vector<std::function<void()>> update_callbacks_;

  void process_frame_(daikin_package &pkg);
};

}  // namespace daikin_x10a
}  // namespace esphome
