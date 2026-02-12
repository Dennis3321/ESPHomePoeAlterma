#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "daikin_x10a.h"

namespace esphome {
namespace daikin_x10a {

class DaikinX10AClimate : public climate::Climate, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE - 1; }

  void set_parent(DaikinX10A *parent) { parent_ = parent; }
  void set_current_temperature_source(const std::string &source) { current_temp_source_ = source; }
  void set_heat_relay(switch_::Switch *sw) { heat_relay_ = sw; }
  void set_cool_relay(switch_::Switch *sw) { cool_relay_ = sw; }

 protected:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;
  void update_state_();

  DaikinX10A *parent_{nullptr};
  std::string current_temp_source_;
  switch_::Switch *heat_relay_{nullptr};
  switch_::Switch *cool_relay_{nullptr};
};

}  // namespace daikin_x10a
}  // namespace esphome
