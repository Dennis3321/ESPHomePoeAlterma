#include "daikin_x10a_climate.h"
#include "esphome/core/log.h"
#include <cstdlib>

namespace esphome {
namespace daikin_x10a {

static const char *const TAG = "daikin_x10a.climate";

void DaikinX10AClimate::setup() {
  // Register callback with parent to update state on each register scan
  parent_->add_on_update_callback([this]() { this->update_state_(); });
}

void DaikinX10AClimate::dump_config() {
  ESP_LOGCONFIG(TAG, "Daikin X10A Climate:");
  ESP_LOGCONFIG(TAG, "  Current temp source: %s", current_temp_source_.c_str());
}

climate::ClimateTraits DaikinX10AClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_COOL,
  });
  return traits;
}

void DaikinX10AClimate::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    auto new_mode = *call.get_mode();

    // Safety: turn off both relays first
    heat_relay_->turn_off();
    cool_relay_->turn_off();

    switch (new_mode) {
      case climate::CLIMATE_MODE_HEAT:
        heat_relay_->turn_on();
        break;
      case climate::CLIMATE_MODE_COOL:
        cool_relay_->turn_on();
        break;
      case climate::CLIMATE_MODE_OFF:
      default:
        break;
    }

    this->mode = new_mode;
  }

  this->publish_state();
}

void DaikinX10AClimate::update_state_() {
  // Update current temperature from register
  std::string val = parent_->get_register_value(current_temp_source_);
  if (!val.empty()) {
    float temp = std::atof(val.c_str());
    this->current_temperature = temp;
  }

  this->publish_state();
}

}  // namespace daikin_x10a
}  // namespace esphome
