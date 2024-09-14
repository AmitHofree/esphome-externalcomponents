#pragma once

#include "esphome/components/climate_ir/climate_ir.h"
#include "kelvinator_data.h"

#include <cinttypes>

namespace esphome {
namespace kelvinator {

class KelvinatorClimate : public climate_ir::ClimateIR {
 public:
  KelvinatorClimate()
      : climate_ir::ClimateIR(KELVINATOR_TEMP_MIN, KELVINATOR_TEMP_MAX, 1.0f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL}) {}

 protected:
  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;
  /// Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
};

}  // namespace kelvinator
}  // namespace esphome
