#include "denon_ir.h"
#include "esphome/core/log.h"

namespace esphome {
namespace denon_ir {

static const char *const TAG = "denon_ir";

// Receiver tolerances (us). Wide on purpose: cheap IR receivers stretch marks.
static constexpr int32_t MIN_MARK_US = 150;
static constexpr int32_t MAX_MARK_US = 500;
static constexpr int32_t MIN_ZERO_US = 500;
static constexpr int32_t MAX_ZERO_US = 1200;
static constexpr int32_t MIN_ONE_US = 1400;
static constexpr int32_t MAX_ONE_US = 2400;

void DenonIR::dump_config() {
  ESP_LOGCONFIG(TAG, "Denon IR:");
  ESP_LOGCONFIG(TAG, "  Transmitter: %s", this->transmitter_ != nullptr ? "yes" : "no");
}

void DenonIR::encode_frame(remote_base::RemoteTransmitData *dst, uint16_t code) {
  for (int8_t bit = NBITS - 1; bit >= 0; bit--) {
    dst->mark(BIT_MARK_US);
    dst->space(((code >> bit) & 1) ? ONE_SPACE_US : ZERO_SPACE_US);
  }
  dst->mark(BIT_MARK_US);
}

void DenonIR::transmit(uint16_t code, uint32_t repeat) {
  if (this->transmitter_ == nullptr) {
    ESP_LOGW(TAG, "Cannot send 0x%04X: no transmitter_id configured", code);
    return;
  }
  code &= 0x7FFF;
  ESP_LOGD(TAG, "Sending code=0x%04X (address=%u, command=0x%03X)", code, address_of(code), command_of(code));
  auto call = this->transmitter_->transmit();
  auto *dst = call.get_data();
  dst->set_carrier_frequency(CARRIER_HZ);
  dst->reserve(4 * (NBITS + 1));
  encode_frame(dst, code);
  dst->space(FRAME_GAP_US);
  encode_frame(dst, code ^ INVERT_MASK);
  call.set_send_times(repeat == 0 ? 1 : repeat);
  call.set_send_wait(FRAME_GAP_US);
  call.perform();
}

bool DenonIR::decode(remote_base::RemoteReceiveData &data, uint16_t &code) {
  code = 0;
  uint8_t bits = 0;
  for (uint32_t i = 0; data.is_valid(i + 1); i++) {
    const int32_t mark = data.peek(i);
    const int32_t space = -data.peek(i + 1);
    if (mark <= 0 || space <= 0)
      continue;
    if (mark < MIN_MARK_US || mark > MAX_MARK_US) {
      bits = 0;
      code = 0;
      continue;
    }
    if (space > MIN_ZERO_US && space < MAX_ZERO_US) {
      code <<= 1;
    } else if (space > MIN_ONE_US && space < MAX_ONE_US) {
      code = (code << 1) | 1;
    } else {
      bits = 0;
      code = 0;
      continue;
    }
    if (++bits == NBITS)
      return true;
  }
  return false;
}

bool DenonIR::on_receive(remote_base::RemoteReceiveData data) {
  uint16_t code;
  if (!decode(data, code))
    return false;
  if (is_inverted_frame(code))
    return true;  // second half of a pair, already handled

  ESP_LOGI(TAG, "Received code=0x%04X (address=%u, command=0x%03X)", code, address_of(code), command_of(code));
  this->callbacks_.call(code);
#ifdef USE_BINARY_SENSOR
  for (auto *sensor : this->binary_sensors_) {
    if (sensor->get_code() == code) {
      sensor->publish_state(true);
      sensor->publish_state(false);
    }
  }
#endif
  return true;
}

}  // namespace denon_ir
}  // namespace esphome
