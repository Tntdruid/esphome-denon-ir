# Changelog

## 1.0.0 - 2026-11-17

First release, tested on a Denon AVR-1912 with an Athom AR01 IR blaster.

- `denon_ir` component for the Denon/Sharp 15-bit IR protocol.
- Sends each code in the normal + inverted frame pair Denon receivers expect.
- `denon_ir.transmit` action, using either `code` or `address` + `command`, with optional `repeat`.
- `on_code` trigger for received codes.
- `binary_sensor` platform that pulses when a given code is received.
- English example config for the Athom AR01, with power, volume, input and sound mode controls.
