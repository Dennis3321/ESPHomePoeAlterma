#pragma once
#include "esphome/core/component.h"
#include "esphome/core/application.h"      // App
#include "esphome/core/entity_base.h"      // EntityCategory
#include "esphome/components/uart/uart.h"
#include "esphome/components/switch/switch.h"
#include <vector>
#include "daikin_package.h"

namespace esphome {
namespace daikin_x10a {

class LabelDef {public:
  uint8_t  registryID{0};
  uint8_t  offset{0};
  uint8_t  dataSize{0};
  uint16_t convid{0};
  uint8_t  dataType{0};     
  const char *label{nullptr};
  char asString[64]{0};

  LabelDef() = default;
  LabelDef(uint8_t registryIDp, uint8_t offsetp, uint16_t convidp,
           uint8_t dataSizep, uint8_t dataTypep, const char *labelp)
  : registryID(registryIDp), offset(offsetp), dataSize(dataSizep),
    convid(convidp), dataType(dataTypep), label(labelp) {}};  

    
class DaikinX10A;  // forward decl blijft staan

class DaikinX10A : public uart::UARTDevice, public Component {
 public:
  explicit DaikinX10A(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}
  virtual ~DaikinX10A();
    void loop() override;
    void FetchRegisters();
    void add_register(int mode, int convid, int offset, int registryID,int dataSize, int dataType, const char* label);

 protected:
  std::vector<uint8_t> buffer_;
  uint8_t last_requested_registry_{0};

  void FetchRegisters(const Register& selectedRegister);
  void process_frame_(daikin_package &pkg, const Register& selectedRegister);
};

}  // namespace daikin_x10a
}  // namespace esphome
