#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "remote_base.h"

#include <cinttypes>

namespace esphome {
namespace remote_base {

const uint8_t KELVINATOR_STATE_LENGTH = 16;

class KelvinatorData {
 public:
  KelvinatorData() {}
  KelvinatorData(std::vector<uint8_t> data) {
    std::copy_n(data.begin(), std::min(data.size(), this->data_.size()), this->data_.begin());
  }
  KelvinatorData(uint8_t *data, uint8_t size) {
    std::copy_n(data, std::min(size, KELVINATOR_STATE_LENGTH), this->data_);
  }
  uint8_t *data() { return this->data_.data(); }
  const uint8_t *data() const { return this->data_.data(); }
  uint8_t size() const { return this->data_.size(); }
  bool is_valid();
  bool operator==(const KelvinatorData &rhs) const { return this->data_ == rhs.data_; }
  uint8_t &operator[](size_t idx) { return this->data_[idx]; }
  const uint8_t &operator[](size_t idx) const { return this->data_[idx]; }
  std::string to_string() const { return format_hex_pretty(this->data_.data(), this->data_.size()); }

 protected:
  std::array<uint8_t, KELVINATOR_STATE_LENGTH> data_;
  uint8_t calc_block_checksum_(const uint8_t *block);
};

class KelvinatorProtocol : public RemoteProtocol<KelvinatorData> {
 public:
  void encode(RemoteTransmitData *dst, const KelvinatorData &data) override;
  optional<KelvinatorData> decode(RemoteReceiveData data) override;
  void dump(const KelvinatorData &data) override;

 protected:
  void encode_byte_(RemoteTransmitData *dst, uint8_t item);
  uint8_t decode_byte_(RemoteReceiveData src, uint8_t *data);
};

DECLARE_REMOTE_PROTOCOL(Kelvinator)

template<typename... Ts> class KelvinatorAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint8_t[KELVINATOR_STATE_LENGTH], data)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    KelvinatorData data{};
    data.data = this->data_.value(x...);
    KelvinatorProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
