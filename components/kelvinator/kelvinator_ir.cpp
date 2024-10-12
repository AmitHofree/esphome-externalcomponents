#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "kelvinator_protocol.h"
#include "kelvinator_ir.h"
#include "kelvinator_data.h"

namespace esphome {
namespace kelvinator {

using remote_base::KelvinatorProtocol;

static const char *const TAG = "kelvinator.climate";

void log_state(const ClimateData& data) {
  auto rawData = data.get_raw();
  char buffer[KELVINATOR_STATE_LENGTH * 2 + 1] = {0};
  for (uint8_t i = 0; i < KELVINATOR_STATE_LENGTH; i++) {
    snprintf(buffer + strlen(buffer), 3, "%02X", rawData[i]);
  }
  ESP_LOGV(TAG, "Raw data: %s", buffer);
}

void KelvinatorClimate::transmit_state() {
  ClimateData data;
  log_state(data);
  data.set_power(true);
  data.set_light(true);
  log_state(data);

  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      data.set_mode(Mode::COOL);
      break;
    case climate::CLIMATE_MODE_HEAT:
      data.set_mode(Mode::HEAT);
      break;
    case climate::CLIMATE_MODE_HEAT_COOL:
      data.set_mode(Mode::AUTO);
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      data.set_mode(Mode::FAN);
      break;
    case climate::CLIMATE_MODE_DRY:
      data.set_mode(Mode::DRY);
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      data.set_power(false);
      break;
  }
  log_state(data);

  if (this->mode != climate::CLIMATE_MODE_DRY && this->mode != climate::CLIMATE_MODE_HEAT_COOL) {
    auto temp = static_cast<uint8_t>(roundf(this->target_temperature));
    data.set_temp(temp);
  } else {
    data.set_temp(KELVINATOR_TEMP_AUTO);
  }
  log_state(data);

  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_HIGH:
      data.set_fan(KELVINATOR_FAN_MAX);
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      data.set_fan(KELVINATOR_FAN_MEDIUM);
      break;
    case climate::CLIMATE_FAN_LOW:
      data.set_fan(KELVINATOR_FAN_MIN);
      break;
    case climate::CLIMATE_FAN_AUTO:
    default:
      data.set_fan(KELVINATOR_FAN_AUTO);
      break;
  }
  log_state(data);

  switch (this->swing_mode) {
    case climate::CLIMATE_SWING_VERTICAL:
      data.set_swing_vertical(true, SwingPosition::SWING_AUTO);
      break;
    case climate::CLIMATE_SWING_OFF:
    default:
      data.set_swing_vertical(false, SwingPosition::OFF);
      break;
  }
  log_state(data);

  data.fix();
  log_state(data);
  ESP_LOGV(TAG, "Sending Kelvinator code: 0x%s", data.to_string().c_str());
  this->transmit_<remote_base::KelvinatorProtocol>(data);
}

bool KelvinatorClimate::on_receive(remote_base::RemoteReceiveData data) {
  auto decoded = remote_base::KelvinatorProtocol().decode(data);
  if (!decoded.has_value())
    return false;

  ClimateData climate = static_cast<ClimateData>(*decoded);
  ESP_LOGV(TAG, "Decoded Kelvinator IR data: %s", climate.to_string().c_str());

  if (!climate.get_power()) {
    this->mode = climate::CLIMATE_MODE_OFF;
  }

  auto vertical_swing = climate.get_swing_vertical_position() != SwingPosition::OFF;
  auto horizontal_swing = climate.get_swing_horizontal();
  this->swing_mode = vertical_swing ? (horizontal_swing ? climate::CLIMATE_SWING_BOTH : climate::CLIMATE_SWING_VERTICAL)
                                    : climate::CLIMATE_SWING_OFF;

  switch (climate.get_mode()) {
    case Mode::COOL:
      this->mode = climate::CLIMATE_MODE_COOL;
      break;
    case Mode::HEAT:
      this->mode = climate::CLIMATE_MODE_HEAT;
      break;
      this->mode = climate::CLIMATE_MODE_HEAT_COOL;
      break;
    case Mode::FAN:
      this->mode = climate::CLIMATE_MODE_FAN_ONLY;
      break;
    case Mode::DRY:
      this->mode = climate::CLIMATE_MODE_DRY;
      break;
    case Mode::AUTO:
    default:
      this->mode = climate::CLIMATE_MODE_HEAT_COOL;
      break;
  }

  this->target_temperature = climate.get_temp();

  switch (climate.get_fan()) {
    case KELVINATOR_FAN_AUTO:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
    case KELVINATOR_FAN_MAX:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
    case KELVINATOR_FAN_MIN:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    default:
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
  }

  switch (climate.get_swing_vertical_auto()) {
    case true:
      this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
      break;
    case false:
      this->swing_mode = climate::CLIMATE_SWING_OFF;
      break;
  }

  this->publish_state();

  return true;
}

}  // namespace kelvinator
}  // namespace esphome
