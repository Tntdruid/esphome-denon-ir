# ESPHome Denon IR

An [ESPHome](https://esphome.io) external component for the **Denon/Sharp 15-bit IR protocol**. It sends and receives the codes used by Denon AV receivers. It was written for [esphome/feature-requests#1134](https://github.com/esphome/feature-requests/issues/1134).

It's tested on a **Denon AVR-1912** with an **Athom AR01** IR blaster (ESP8285) on ESPHome 2026.10.2.

## Why raw replay doesn't work

The protocol sends each code **twice**: the normal frame, then after about 43.6 ms the same frame with the 10 command bits inverted. The receiver ignores anything that doesn't contain both halves with the right timing.

| | |
|---|---|
| Carrier | 38 kHz |
| Frame | 5 address bits + 10 command bits, MSB first, + stop mark |
| Bit mark | 263 µs |
| "0" / "1" space | 789 µs / 1841 µs |
| Gap before inverted frame | 43572 µs |

The timings come from Denon's official Pronto codes ("master IR hex" spreadsheet). The code values are the same as Tasmota's `DENON` protocol with `Bits: 15` (the `Data` field).

## Installation

```yaml
external_components:
  - source: github://Tntdruid/esphome-denon-ir@v1.0.0   # or @main for the latest
    components: [denon_ir]

remote_transmitter:
  id: ir_tx
  pin: GPIO4
  carrier_duty_percent: 50%

remote_receiver:          # optional, only needed for receiving
  id: ir_rx
  pin: { number: GPIO5, inverted: true }
  idle: 10ms

denon_ir:
  transmitter_id: ir_tx
  receiver_id: ir_rx      # optional
```

## Configuration

### `denon_ir`

| Option | Description |
|---|---|
| `id` | Optional ID, needed if you configure more than one instance |
| `transmitter_id` | `remote_transmitter` to send with |
| `receiver_id` | `remote_receiver` to listen on (optional) |
| `on_code` | Automation that runs on each received code. `x` is the 15-bit code (`uint16_t`) |

Each received code is logged as `denon_ir: Received code=0x2278 (address=8, command=0x278)`. That makes it easy to learn codes from the original remote. The inverted repeat frame is filtered out.

### Action `denon_ir.transmit`

```yaml
- denon_ir.transmit:
    code: 0x2278          # or:
    # address: 8
    # command: 0x278
    repeat: 1             # optional, 1-20
```

Every option is templatable, for example `code: !lambda "return my_code;"`.

### Binary sensor

It sends a short ON pulse when the given code is received.

```yaml
binary_sensor:
  - platform: denon_ir
    name: "Remote Volume Up"
    code: 0x223C
```

## Kaseikyo (48-bit) codes

Denon receivers from the xx08 series onward also use 48-bit Kaseikyo codes for newer functions such as Quick Select, Net/USB and some surround modes. ESPHome already sends these with `transmit_panasonic` and Denon's vendor address `0x2A4C`:

```yaml
- remote_transmitter.transmit_panasonic:
    address: 0x2A4C
    command: 0x028248C8   # Quick Select 1
```

## AVR main zone codes

| Function | Protocol | Code |
|---|---|---|
| Main Zone On / Off | Sharp | `0x2278` / `0x2178` |
| Volume Up / Down / Mute | Sharp | `0x223C` / `0x213C` / `0x203C` |
| CBL/SAT / DVD / Blu-ray | Sharp | `0x204C` / `0x231C` / `0x214C` |
| Game / Media Player / TV Audio | Sharp | `0x22CC` / `0x21CC` / `0x224C` |
| AUX1 / Tuner / CD | Sharp | `0x20CC` / `0x228C` / `0x208C` |
| Pure Direct / Stereo | Sharp | `0x2158` / `0x1AE4` |
| Music / Game / Standard (Dolby/DTS) | Sharp | `0x11A4` / `0x13A4` / `0x209C` |
| Multi Ch Stereo | Sharp | `0x1854` |
| AUX2 / Net/USB | Kaseikyo | `0x0285B433` / `0x028E34B8` |
| Quick Select 1 / 2 / 3 / 4 | Kaseikyo | `0x028248C8` / `0x028A48C0` / `0x028648CC` / `0x028E48C4` |
| Movie / Auto / Direct-Stereo | Kaseikyo | `0x028928A3` / `0x028228A8` / `0x0288C842` |
| Sleep | Kaseikyo | `0x02822CAC` |

[`examples/athom-ar01-denon-avr.yaml`](examples/athom-ar01-denon-avr.yaml) is a complete configuration with power buttons, volume, an input selector and a sound mode selector.

## License

MIT
