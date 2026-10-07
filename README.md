# Magnum Energy Inverter ESPHome Component

An ESPHome custom component for monitoring and controlling Magnum Energy inverter/chargers (MS, ME, MM, RD, MMS series) via the RS485 network port.

## Supported Devices

This component reads data from all devices on the Magnum RS485 network:

- **Inverter/Charger** — MS4024PAE, MS4024, MS2012, ME2012, ME3112, and all other Magnum models
- **ME-BMK** — Battery Monitor Kit (state of charge, voltage, amps, min/max, cumulative Ah)
- **ME-ARC / ME-ARTR** — Remote control settings (charge parameters, LBCO, float/absorb/EQ voltages)
- **ME-RTR** — Router (revision info)

## Hardware Required

- **ESP32 board** — M5Stack Atom, ESP32 DevKit, or similar
- **RS485 transceiver** — M5Stack Atomic RS485 Base, MAX485 module, or similar
- **RJ-11 cable** — to connect to the inverter's network port

### Wiring (RJ-11 on Inverter)

The Magnum inverter uses an RJ-11 connector with this pinout (top to bottom):

| Pin | Signal | Connect to |
|-----|--------|------------|
| 1 | RS485 "B" | RS485 module B |
| 2 | +14V | Do NOT connect |
| 3 | GND | RS485 module GND |
| 4 | RS485 "A" | RS485 module A |

> **Warning:** Pin 2 carries +14V from the inverter. Do not connect this to your ESP32.

### Communication Settings

- Baud rate: 19200
- Data bits: 8
- Parity: None
- Stop bits: 1
- Protocol: Half-duplex RS485

## Installation

Add this to your ESPHome YAML:

```yaml
external_components:
  - source: github://kkrankall/magnum-inverter-esphome
    components: [magnum_inverter]
```

See [example.yaml](example.yaml) for a complete configuration with all available sensors.

The component builds with both ESP32 frameworks: ESP-IDF (the ESPHome default since 2026.1) and Arduino.

### Options

| Option | Default | Description |
|--------|---------|-------------|
| `uart_id` | | The UART bus (19200 baud, 8N1). Optional if there is only one. |
| `time_id` | | Time source, used to timestamp `last_fault_time`. |
| `stale_timeout` | `10s` | When no inverter (or BMK) frames arrive for this long, those sensors change to unknown in Home Assistant instead of holding their last value. The `connected` binary sensor turns off. |

## Quick Start

Minimal configuration:

```yaml
external_components:
  - source: github://kkrankall/magnum-inverter-esphome
    components: [magnum_inverter]

uart:
  id: magnum_uart
  tx_pin: GPIO19
  rx_pin: GPIO22
  baud_rate: 19200

magnum_inverter:
  uart_id: magnum_uart

sensor:
  - platform: magnum_inverter
    battery_voltage:
      name: "Battery Voltage"
    dc_amps:
      name: "DC Amps"
    ac_out_voltage:
      name: "AC Output Voltage"
    frequency:
      name: "Frequency"
    bmk_soc:
      name: "State of Charge"

text_sensor:
  - platform: magnum_inverter
    mode:
      name: "Inverter Mode"
    inverter_on:
      name: "Inverter On"
```

## Available Sensors

### Inverter Sensors (numeric)

| Sensor | Description | Unit |
|--------|-------------|------|
| `battery_voltage` | DC battery voltage | V |
| `dc_amps` | DC current | A |
| `ac_out_voltage` | AC output voltage (RMS) | V |
| `ac_in_voltage` | AC input voltage as reported by the inverter (peak, not RMS) | V |
| `ac_out_amps` | AC output current | A |
| `ac_in_amps` | AC input current | A |
| `ac_out_watts` | AC output power (derived: volts × amps, so apparent power in 1 A steps) | W |
| `battery_watts` | Battery power (derived) | W |
| `frequency` | AC output frequency | Hz |
| `battery_temp` | Battery temperature | °C |
| `fet_temp` | FET temperature | °C |
| `xfmr_temp` | Transformer temperature | °C |
| `inverter_fault_code` | Active fault code (raw) | — |
| `inverter_fault_active` | 1 if fault active, 0 if none | — |
| `inverter_revision` | Inverter firmware version | — |

### Inverter Text Sensors

| Sensor | Description | Example Values |
|--------|-------------|----------------|
| `mode` | Current operating mode | Invert, Float, Absorb, Bulk, EQ, Search, Off, Standby |
| `inverter_fault` | Fault description | None (0x00), Low battery, Over temp, etc. |
| `inverter_model` | Inverter model name | MS4024PAE, MS2012, ME3112, etc. |
| `inverter_stackmode` | Stacking configuration | Stand Alone, Parallel master, Parallel slave, etc. |
| `inverter_led` | Inverter LED state (prefer the binary sensor below) | On, Off |
| `charger_led` | Charger LED state (prefer the binary sensor below) | On, Off |
| `inverter_on` | System active state | On (any mode except Off), Off |

### Binary Sensors

| Sensor | Description |
|--------|-------------|
| `connected` | On while frames are arriving from the Magnum network |
| `inverter_led` | Inverter LED (on = inverter enabled) |
| `charger_led` | Charger LED |
| `inverter_fault` | On while the inverter reports a fault |

### BMK Sensors (Battery Monitor)

| Sensor | Description | Unit |
|--------|-------------|------|
| `bmk_soc` | State of charge | % |
| `bmk_voltage` | Battery voltage (high precision) | V |
| `bmk_amps` | Battery current (+ charge, - discharge) | A |
| `bmk_min_voltage` | Recorded minimum voltage | V |
| `bmk_max_voltage` | Recorded maximum voltage | V |
| `bmk_ah_inout` | Net amp-hours in/out | Ah |
| `bmk_ah_trip` | Amp-hour trip meter | Ah |
| `bmk_cumulative_ah` | Cumulative amp-hours out | Ah |
| `bmk_revision` | BMK firmware version | — |
| `bmk_watts` | Battery power (derived: V × A) | W |

### BMK Text Sensors

| Sensor | Description | Example Values |
|--------|-------------|----------------|
| `bmk_fault` | BMK fault status | Normal, Reserved, Fault start |

### Remote/ARTR Settings Sensors

These reflect the current configuration settings from the ME-ARC or ME-ARTR remote:

| Sensor | Description | Unit |
|--------|-------------|------|
| `remote_searchwatts` | Search mode watt threshold | W |
| `remote_chargeramps` | Charger current limit | % |
| `remote_absorb` | Absorb voltage setpoint (custom battery type only; the presets do not send it, so it is unknown) | V |
| `remote_float` | Float voltage setpoint | V |
| `remote_eq` | EQ voltage (absorb + EQ offset; custom battery type only) | V |
| `remote_lbco` | Low battery cutout voltage | V |
| `remote_battery_size` | Configured battery bank size | Ah |
| `remote_shore_amps` | AC shore power limit | A |
| `remote_vac_cutout` | VAC cutout threshold (raw) | — |
| `remote_absorb_time` | Absorption charge time | hrs |

### Remote Text Sensors

| Sensor | Description | Example Values |
|--------|-------------|----------------|
| `remote_battery_type` | Configured battery type | Gel, Flooded, AGM, AGM2, Custom |

### RTR Sensors

| Sensor | Description |
|--------|-------------|
| `rtr_revision` | ME-RTR firmware version |
| `rtr_frames` | RTR frame count |
| `rtr_model` | RTR model string (text sensor) |

### Diagnostics

| Sensor | Description |
|--------|-------------|
| `inverter_frames` | Total inverter frames received |
| `bmk_frames` | Total BMK frames received |
| `remote_frames` | Total remote frames received |
| `rejected_frames` | Rejected/unrecognized bytes |
| `last_frame_age` | Seconds since last valid frame |

The frame counters and `last_frame_age` update every 10 seconds; everything else updates every second.
| `last_fault_code` | Last latched fault code |
| `last_fault_text` | Last fault description (text sensor) |
| `last_fault_time` | Timestamp of last fault (text sensor) |

## Buttons

| Button | Description |
|--------|-------------|
| `record_packets` | Records all raw RS485 packets for 15 seconds (visible in ESPHome logs) |
| `toggle_inverter` | Sends inverter on/off toggle command (experimental) |

### Raw Packet Recording

The `record_packets` button logs all raw RS485 bus traffic for 15 seconds, 32 bytes per line, as it is received. This is useful for debugging, reverse engineering, or verifying bus communication.

### Toggle Inverter (Experimental)

The `toggle_inverter` button attempts to send the on/off toggle command to the inverter. This works by transmitting a modified remote packet with byte 0 bit 0 set (the "momentary high" toggle per the Magnum protocol spec).

> **Note:** When a physical remote (ME-ARC/ME-ARTR) is connected to the same RS485 bus, the toggle command may not work reliably due to bus contention. The inverter only accepts one remote response per 100ms cycle, and the physical remote occupies that slot. For reliable control, consider disconnecting the physical remote and using the ESP32 as the sole remote device.

## Protocol Reference

This component implements the Magnum Energy RS485 network protocol as documented in the official "Magnum Networking Communications Protocol" specification (2003-2009). Key protocol details:

- **Bus speed:** 19200 baud, 8N1, half-duplex RS485
- **Cycle time:** 100ms — inverter transmits 21 bytes, remote responds 10ms later with 21 bytes
- **Packet types:**
  - Inverter: 21 bytes, identified by the mode byte, the voltage at offset 2 and the model byte at offset 14. Some inverters send a 22nd byte with no data; the parser learns which kind it is talking to.
  - Remote: 21 bytes, identified by footer/subtype byte at offset 20
  - BMK: 18 bytes, starts with 0x81
  - RTR: 2 bytes, starts with 0x91

The ESP32 UART delivers bytes in chunks, so a frame can be split across reads. The parser keeps a partial frame until the rest arrives. Remote settings are only published once two frames in a row agree.

### Tests

The frame parser has no ESPHome dependencies and can be tested on a desktop machine:

```bash
c++ -std=c++17 -Wall -Wextra -I esphome/components/magnum_inverter tests/test_frame_scanner.cpp -o /tmp/test_frame_scanner && /tmp/test_frame_scanner
```

The test replays sample packets through a simulated 19200-baud bus and ESP32 UART, for inverters that send 21 and 22 bytes, with and without junk bytes on the line.

## Supported Inverter Models

All Magnum Energy inverter/charger models are supported:

MM612, MM612-AE, MM1212, MMS1012, MM1012E, MM1512, MMS912E, ME1512, ME2012, RD2212, ME2512, ME3112, MS2012, MS1512E, MS2012E, MSH3012M, MS2812, MS2712E, MM1324E, MM1524, RD1824, RD2624E, RD2824, RD4024E, RD3924, MS4124E, MS2024, MSH4024M, MSH4024RE, MS4024, MS4024AE, MS4024PAE, MS4448AE, MS3748AEJ, MS4048, MS4448PAE, MS3748PAEJ, MS4348PE

## Troubleshooting

**No data / all zeros:**
- Verify wiring: A→A, B→B, GND→GND
- Check baud rate is 19200
- Try swapping A and B lines
- Ensure the RJ-11 cable is plugged into the inverter's "Network" port or port 5 or 6 on the ME-ARTR and using a phone splitter

**Rejected bytes climbing:**
- A few during startup are normal while the parser synchronizes with the packet stream
- A steady climb means noise on the line or packets the parser does not know (AGS, PT-100). Press `record_packets` and check the log.

**Garbled data (lots of 0xFF):**
- RS485 A and B are backwards
- Wrong baud rate — verify 19200
- Electrical issue — check connections and cable length

## License

MIT License — see [LICENSE](LICENSE) for details.

## Acknowledgments

- Protocol based on the Magnum Energy Networking Communications Protocol specification
- Built for the ESPHome ecosystem and Home Assistant integration
