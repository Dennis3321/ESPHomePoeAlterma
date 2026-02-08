#pragma once
#include "esphome/core/component.h"
#include "esphome/core/application.h"      // App
#include "esphome/core/entity_base.h"      // EntityCategory
#include "esphome/components/uart/uart.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <vector>
#include "daikin_package.h"
#include "register_definitions.h"

namespace esphome {
namespace daikin_x10a {

class DaikinX10A : public uart::UARTDevice, public Component {
 public:
  explicit DaikinX10A(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}
  virtual ~DaikinX10A();
    void loop() override;
    void FetchRegisters();
    void add_register(int mode, int convid, int offset, int registryID, int dataSize, int dataType, const char* label);
    void set_text_sensor_for_register(const std::string& label, text_sensor::TextSensor *sensor);
    
    // Get register value by label name
    std::string get_register_value(const std::string& label) const;

 protected:
  std::vector<uint8_t> buffer_;
  uint8_t last_requested_registry_{0};
  // Map from register label to text sensor
  std::vector<std::pair<std::string, text_sensor::TextSensor*>> register_sensors_;

  void process_frame_(daikin_package &pkg);
};

}  // namespace daikin_x10a
}  // namespace esphome
