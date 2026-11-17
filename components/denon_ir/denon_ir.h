#pragma once

#include <vector>

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/remote_base/remote_base.h"

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

namespace esphome {
namespace denon_ir {

// Timings taken from Denon's official Pronto codes ("master IR hex" spreadsheet).
static constexpr uint32_t CARRIER_HZ = 38000;
static constexpr uint32_t BIT_MARK_US = 263;
static constexpr uint32_t ZERO_SPACE_US = 789;
static constexpr uint32_t ONE_SPACE_US = 1841;
static constexpr uint32_t FRAME_GAP_US = 43572;
static constexpr uint8_t NBITS = 15;
static constexpr uint16_t INVERT_MASK = 0x3FF;  // the 10 command bits

inline uint8_t address_of(uint16_t code) { return (code >> 10) & 0x1F; }
inline uint16_t command_of(uint16_t code) { return code & 0x3FF; }
inline uint16_t make_code(uint8_t address, uint16_t command) {
  return (uint16_t(address & 0x1F) << 10) | (command & 0x3FF);
}
// The second (inverted) frame of a pair ends in ...11, the first in ...00.
inline bool is_inverted_frame(uint16_t code) { return (code & 0x3) == 0x3; }

#ifdef USE_BINARY_SENSOR
class DenonIRBinarySensor : public binary_sensor::BinarySensor {
 public:
  void set_code(uint16_t code) { this->code_ = code; }
  uint16_t get_code() const { return this->code_; }

 protected:
  uint16_t code_{0};
};
#endif

class DenonIR : public Component, public remote_base::RemoteReceiverListener {
 public:
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_transmitter(remote_base::RemoteTransmitterBase *transmitter) { this->transmitter_ = transmitter; }
  void transmit(uint16_t code, uint32_t repeat = 1);

  bool on_receive(remote_base::RemoteReceiveData data) override;
  void add_on_code_callback(std::function<void(uint16_t)> &&callback) { this->callbacks_.add(std::move(callback)); }
#ifdef USE_BINARY_SENSOR
  void register_binary_sensor(DenonIRBinarySensor *sensor) { this->binary_sensors_.push_back(sensor); }
#endif

  /// Decode the first 15-bit frame in data. Returns true and sets code on success.
  static bool decode(remote_base::RemoteReceiveData &data, uint16_t &code);
  static void encode_frame(remote_base::RemoteTransmitData *dst, uint16_t code);

 protected:
  remote_base::RemoteTransmitterBase *transmitter_{nullptr};
  CallbackManager<void(uint16_t)> callbacks_;
#ifdef USE_BINARY_SENSOR
  std::vector<DenonIRBinarySensor *> binary_sensors_;
#endif
};

class CodeTrigger : public Trigger<uint16_t> {
 public:
  explicit CodeTrigger(DenonIR *parent) {
    parent->add_on_code_callback([this](uint16_t code) { this->trigger(code); });
  }
};

template<typename... Ts> class TransmitAction : public Action<Ts...>, public Parented<DenonIR> {
 public:
  TEMPLATABLE_VALUE(uint16_t, code)
  TEMPLATABLE_VALUE(uint8_t, address)
  TEMPLATABLE_VALUE(uint16_t, command)
  TEMPLATABLE_VALUE(uint32_t, repeat)

  void play(const Ts &...x) override {
    uint16_t code = this->code_.has_value() ? this->code_.value(x...)
                                            : make_code(this->address_.value(x...), this->command_.value(x...));
    this->parent_->transmit(code, this->repeat_.value(x...));
  }
};

}  // namespace denon_ir
}  // namespace esphome
