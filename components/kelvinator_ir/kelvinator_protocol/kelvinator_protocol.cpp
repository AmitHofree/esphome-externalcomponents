#include "kelvinator_protocol.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.kelvinator";

static const uint16_t KELVINATOR_TICK = 85;
static const uint16_t KELVINATOR_HDR_MARK_TICKS = 106;
static const uint16_t KELVINATOR_HDR_MARK = KELVINATOR_HDR_MARK_TICKS * KELVINATOR_TICK;
static const uint16_t KELVINATOR_HDR_SPACE_TICKS = 53;
static const uint16_t KELVINATOR_HDR_SPACE = KELVINATOR_HDR_SPACE_TICKS * KELVINATOR_TICK;
static const uint16_t KELVINATOR_BIT_MARK_TICKS = 8;
static const uint16_t KELVINATOR_BIT_MARK = KELVINATOR_BIT_MARK_TICKS * KELVINATOR_TICK;
static const uint16_t KELVINATOR_ONE_SPACE_TICKS = 18;
static const uint16_t KELVINATOR_ONE_SPACE = KELVINATOR_ONE_SPACE_TICKS * KELVINATOR_TICK;
static const uint16_t KELVINATOR_ZERO_SPACE_TICKS = 6;
static const uint16_t KELVINATOR_ZERO_SPACE = KELVINATOR_ZERO_SPACE_TICKS * KELVINATOR_TICK;
static const uint16_t KELVINATOR_GAP_SPACE_TICKS = 235;
static const uint16_t KELVINATOR_GAP_SPACE = KELVINATOR_GAP_SPACE_TICKS * KELVINATOR_TICK;
static const uint8_t KELVINATOR_CHECKSUM_START = 10;

void KelvinatorProtocol::encode(RemoteTransmitData *dst, const KelvinatorData &data) {
  dst->set_carrier_frequency(38000);
  dst->reserve(2 * (2 * 6 + 2 * 8 * 8));  // Each message can carry 8 bytes, and requires 6 extra signals

  // There are two messages back-to-back in a full Kelvinator IR message
  // sequence.
  for (uint8_t i = 0, offset = 0; i < 2; i++, offset += 8) {
    // Header + Data Block #1 (4 bytes)
    dst->item(KELVINATOR_HDR_MARK, KELVINATOR_HDR_SPACE);
    for (uint8_t j = 0; j < 4; j++) {
      this->encode_byte_(dst, data[offset + j]);
    }

    // Command data footer (3 bits, b010) + footer
    dst->item(KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE);
    dst->item(KELVINATOR_BIT_MARK, KELVINATOR_ONE_SPACE);
    dst->item(KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE);
    dst->item(KELVINATOR_BIT_MARK, KELVINATOR_GAP_SPACE);

    // Data block #2 (4 bytes) + footer
    for (uint8_t k = 0; k < 4; k++) {
      this->encode_byte_(dst, data[offset + k + 4]);
    }
    dst->item(KELVINATOR_BIT_MARK, 2 * KELVINATOR_GAP_SPACE);
  }
}

void KelvinatorProtocol::encode_byte_(RemoteTransmitData *dst, uint8_t item) {
  for (uint8_t bit = 0; bit < 8; bit++, item >>= 1) {
    if (item & 1) {
      dst->item(KELVINATOR_BIT_MARK, KELVINATOR_ONE_SPACE);
    } else {
      dst->item(KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE);
    }
  }
}

optional<KelvinatorData> KelvinatorProtocol::decode(RemoteReceiveData src) {
  KelvinatorData out;

  // There are two messages back-to-back in a full Kelvinator IR message
  // sequence.
  for (uint8_t i = 0, offset = 0; i < 2; i++, offset += 8) {
    // Header + Data Block #1 (4 bytes)
    if (!src.expect_item(KELVINATOR_HDR_MARK, KELVINATOR_HDR_SPACE))
      return {};
    for (uint8_t j = 0; j < 4; j++) {
      if (this->decode_byte_(src, out.data() + offset + j) != 1)
        return {};
    }

    // Command data footer (3 bits, b010) + footer
    if (!src.expect_item(KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE))
      return {};
    if (!src.expect_item(KELVINATOR_BIT_MARK, KELVINATOR_ONE_SPACE))
      return {};
    if (!src.expect_item(KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE))
      return {};
    if (!src.expect_item(KELVINATOR_BIT_MARK, KELVINATOR_GAP_SPACE))
      return {};

    // Data block #2 (4 bytes) + footer
    for (uint8_t k = 0; k < 4; k++) {
      if (this->decode_byte_(src, out.data() + offset + k + 4) != 1)
        return {};
    }
    if (!src.expect_item(KELVINATOR_BIT_MARK, 2 * KELVINATOR_GAP_SPACE))
      return {};
  }

  if (!out.is_valid())
    return {};
  return out;
}

uint8_t KelvinatorProtocol::decode_byte_(RemoteReceiveData src, uint8_t *data) {
  uint8_t item = 0;
  for (uint8_t bit = 0; bit < 8; bit++, item >>= 1) {
    if (src.expect_item(KELVINATOR_BIT_MARK, KELVINATOR_ONE_SPACE)) {
      item = (item << 1) | 1;
    } else if (src.expect_item(KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE)) {
      item = (item << 1) | 0;
    } else {
      return 0;
    }
  }
  *data = item;
  return 1;
}

void KelvinatorProtocol::dump(const KelvinatorData &data) {
  ESP_LOGI(TAG, "Received Kelvinator: data=0x%s", data.to_string().c_str());
}

uint8_t KelvinatorData::calc_block_checksum_(const uint8_t *block) {
  uint8_t sum = KELVINATOR_CHECKSUM_START;
  // Sum the lower half of the first 4 bytes of this block.
  for (uint8_t i = 0; i < 4 && i < 8 - 1; i++, block++)
    sum += (*block & 0b1111);
  // then sum the upper half of the next 3 bytes.
  for (uint8_t i = 4; i < 7; i++, block++)
    sum += (*block >> 4);
  // Trim it down to fit into the 4 bits allowed. i.e. Mod 16.
  return sum & 0b1111;
}

bool KelvinatorData::is_valid() {
  for (uint16_t offset = 0; offset + 7 < 16; offset += 8) {
    // Top 4 bits of the last byte in the block is the block's checksum.
    auto checksum_bits = ((*this)[offset + 7] & 0xF0) >> 4;
    if (checksum_bits != this->calc_block_checksum_(this->data() + offset))
      return false;
  }
  return true;
}

}  // namespace remote_base
}  // namespace esphome
