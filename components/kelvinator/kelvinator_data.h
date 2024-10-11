#pragma once

#include "esphome/components/remote_base/kelvinator_protocol.h"
#include "esphome/components/climate/climate_mode.h"

namespace esphome {
namespace kelvinator {

using climate::ClimateMode;
using climate::ClimateFanMode;
using remote_base::KelvinatorData;
using remote_base::KELVINATOR_STATE_LENGTH;

const uint8_t KELVINATOR_TEMP_MAX = 30;
const uint8_t KELVINATOR_TEMP_MIN = 16;
const uint8_t KELVINATOR_TEMP_AUTO = 25;
const uint8_t KELVINATOR_FAN_AUTO = 0;
const uint8_t KELVINATOR_FAN_MIN = 1;
const uint8_t KELVINATOR_FAN_MEDIUM = 3;
const uint8_t KELVINATOR_FAN_MAX = 5;
const uint8_t KELVINATOR_FAN_BASIC_MAX = 3;

enum Mode : uint8_t {
  AUTO = 0,
  COOL = 1,
  DRY = 2,
  FAN = 3,
  HEAT = 4,
};

enum SwingPosition : uint8_t {
  OFF = 0b0000,           // 0
  AUTO = 0b0001,          // 1
  HIGHEST = 0b0010,       // 2
  UPPER_MIDDLE = 0b0011,  // 3
  MIDDLE = 0b0100,        // 4
  LOWER_MIDDLE = 0b0101,  // 5
  LOWEST = 0b0110,        // 6
  LOW_AUTO = 0b0111,      // 7
  MIDDLE_AUTO = 0b1001,   // 9
  HIGH_AUTO = 0b1011      // 11
};

struct ClimateBitField {
  // Byte 0
  uint8_t Mode : 3;
  uint8_t Power : 1;
  uint8_t BasicFan : 2;
  uint8_t SwingAuto : 1;
  uint8_t : 1;  // Sleep Modes 1 & 3 (1 = On, 0 = Off)
  // Byte 1
  uint8_t Temp : 4;  // Degrees C.
  uint8_t : 4;
  // Byte 2
  uint8_t : 4;
  uint8_t Turbo : 1;
  uint8_t Light : 1;
  uint8_t IonFilter : 1;
  uint8_t XFan : 1;
  // Byte 3
  uint8_t : 4;
  uint8_t : 2;  // (possibly timer related) (Typically 0b01)
  uint8_t : 2;  // End of command block (B01)
  // (B010 marker and a gap of 20ms)
  // Byte 4
  uint8_t SwingV : 4;
  uint8_t SwingH : 1;
  uint8_t : 3;
  // Byte 5~6
  uint8_t Pad0[2];  // Timer related. Typically 0 except when timer in use.
  // Byte 7
  uint8_t : 4;       // (Used in Timer mode)
  uint8_t Sum1 : 4;  // checksum of the previous bytes (0-6)
  // (gap of 40ms)
  // (header mark and space)
  // Byte 8~10
  uint8_t Pad1[3];  // Repeat of byte 0~2
  // Byte 11
  uint8_t : 4;
  uint8_t : 2;  // (possibly timer related) (Typically 0b11)
  uint8_t : 2;  // End of command block (B01)
  // (B010 marker and a gap of 20ms)
  // Byte 12
  uint8_t : 1;  // Sleep mode 2 (1 = On, 0=Off)
  uint8_t : 6;  // (Used in Sleep Mode 3, Typically 0b000000)
  uint8_t Quiet : 1;
  // Byte 13
  uint8_t : 8;  // (Sleep Mode 3 related, Typically 0x00)
  // Byte 14
  uint8_t : 4;  // (Sleep Mode 3 related, Typically 0b0000)
  uint8_t Fan : 3;
  // Byte 15
  uint8_t : 4;
  uint8_t Sum2 : 4;  // checksum of the previous bytes (8-14)
};

class ClimateData : public KelvinatorData {
 public:
  ClimateData() : KelvinatorData() {}
  ClimateData(const KelvinatorData &data) : KelvinatorData(data) {}

  void set_power(const bool on) { this->bit_field_.Power = on; }
  bool get_power(void) const { this->bit_field_.Power; }

  void set_temp(const uint8_t degrees) {
    uint8_t temp = std::max(KELVINATOR_TEMP_MIN, degrees);
    temp = std::min(KELVINATOR_TEMP_MAX, degrees);
    this->bit_field_.Temp = temp - KELVINATOR_TEMP_MIN;
  }
  uint8_t get_temp(void) const { this->bit_field_.Temp + KELVINATOR_TEMP_MIN; }

  void set_fan(const uint8_t speed) {
    uint8_t fan = std::min(KELVINATOR_FAN_MAX, speed);  // Bounds check

    // Only change things if we need to.
    if (fan != this->bit_field_.Fan) {
      // Set the basic fan values.
      this->bit_field_.BasicFan = std::min(KELVINATOR_FAN_BASIC_MAX, fan);
      // Set the advanced(?) fan value.
      this->bit_field_.Fan = fan;
      // Turbo mode is turned off if we change the fan settings.
      this->set_turbo(false);
    }
  }
  uint8_t get_fan(void) const { this->bit_field_.Fan; }

  void set_mode(const Mode mode) {
    if (mode == Mode::AUTO || mode == Mode::DRY) {
      this->set_temp(KELVINATOR_TEMP_AUTO);
    }
    this->bit_field_.Mode = mode;
  }
  Mode get_mode(void) const {
    if (this->bit_field_.Mode <= Mode::HEAT)
      return static_cast<Mode>(this->bit_field_.Mode);
    return Mode::AUTO;
  }

  void set_swing_vertical(const bool automatic, const SwingPosition position) {
    this->bit_field_.SwingV = position;
    this->bit_field_.SwingAuto = (automatic || this->bit_field_.SwingH);
  };
  bool get_swing_vertical_auto(void) const { this->bit_field_.SwingV & 0b0001; };
  SwingPosition get_swing_vertical_position(void) const {
    uint8_t value = this->bit_field_.SwingV;
    if (value <= LOW_AUTO || value == MIDDLE_AUTO || value == HIGH_AUTO)
      return static_cast<SwingPosition>(value);
    return SwingPosition::OFF;  // Default value if out of range
  };

  void set_swing_horizontal(const bool on) {
    this->bit_field_.SwingH = on;
    this->bit_field_.SwingAuto = (on || (this->bit_field_.SwingV & 0b0001));
  }
  bool get_swing_horizontal(void) const { this->bit_field_.SwingH; }

  void set_quiet(const bool on) { this->bit_field_.Quiet = on; }
  bool get_quiet(void) const { this->bit_field_.Quiet; }

  void set_ion_filter(const bool on) { this->bit_field_.IonFilter = on; }
  bool get_ion_filter(void) const { this->bit_field_.IonFilter; }

  void set_light(const bool on) { this->bit_field_.Light = on; }
  bool get_light(void) const { this->bit_field_.Light; }

  void set_x_fan(const bool on) { this->bit_field_.XFan = on; }
  bool get_x_fan(void) const { this->bit_field_.XFan; }

  void set_turbo(const bool on) { this->bit_field_.Turbo = on; }
  bool get_turbo(void) const { this->bit_field_.Turbo; }

  void fix() {
    if (this->bit_field_.Mode != Mode::COOL && this->bit_field_.Mode != Mode::DRY) {
      this->set_x_fan(false);
    }

    this->data_[3] = 0x50;
    this->data_[8] = this->data_[0];
    this->data_[9] = this->data_[1];
    this->data_[10] = this->data_[2];
    this->data_[11] = 0x70;
    this->update_checksum_();
  }

 protected:
  union {
    std::array<uint8_t, KELVINATOR_STATE_LENGTH> data_;
    ClimateBitField bit_field_;
  };

  void update_checksum_() {
    this->bit_field_.Sum1 = KelvinatorData::calc_block_checksum_(this->data_.data());
    this->bit_field_.Sum2 = calc_block_checksum_(this->data_.data() + 8);
  }
};

}  // namespace kelvinator
}  // namespace esphome