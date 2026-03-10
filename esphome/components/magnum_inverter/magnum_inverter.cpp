#include "magnum_inverter.h"
#include "esphome/core/log.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace magnum_inverter {

static const char *const TAG = "magnum_inv";

// Round to 1 decimal place using integer math to avoid float precision artifacts
static inline float round1(float v) {
  return (int32_t)(v * 10.0f + (v >= 0 ? 0.5f : -0.5f)) / 10.0f;
}

// ==========================================================================
// Lookup tables
// ==========================================================================

static const uint8_t KNOWN_INVERTER_MODELS[] = {
  0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0F,
  0x14, 0x15, 0x19, 0x1E, 0x23, 0x24, 0x28, 0x2C,
  0x2D, 0x2F, 0x35, 0x36, 0x37, 0x3B, 0x3F, 0x45,
  0x4A, 0x5A, 0x5B, 0x67, 0x68, 0x69, 0x6A, 0x6B,
  0x6F, 0x70, 0x72, 0x73, 0x74, 0x75
};

static bool is_known_model(uint8_t m) {
  for (auto km : KNOWN_INVERTER_MODELS)
    if (km == m) return true;
  return false;
}

const char *MagnumInverter::model_to_str_(uint8_t m) {
  switch (m) {
    case 0x06: return "MM612";      
    case 0x07: return "MM612-AE";
    case 0x08: return "MM1212";     
    case 0x09: return "MMS1012";
    case 0x0A: return "MM1012E";    
    case 0x0B: return "MM1512";
    case 0x0C: return "MMS912E";    
    case 0x0F: return "ME1512";
    case 0x14: return "ME2012";     
    case 0x15: return "RD2212";
    case 0x19: return "ME2512";     
    case 0x1E: return "ME3112";
    case 0x23: return "MS2012";     
    case 0x24: return "MS1512E";
    case 0x28: return "MS2012E";    
    case 0x2C: return "MSH3012M";
    case 0x2D: return "MS2812";     
    case 0x2F: return "MS2712E";
    case 0x35: return "MM1324E";    
    case 0x36: return "MM1524";
    case 0x37: return "RD1824";     
    case 0x3B: return "RD2624E";
    case 0x3F: return "RD2824";     
    case 0x45: return "RD4024E";
    case 0x4A: return "RD3924";     
    case 0x5A: return "MS4124E";
    case 0x5B: return "MS2024";     
    case 0x67: return "MSH4024M";
    case 0x68: return "MSH4024RE";  
    case 0x69: return "MS4024";
    case 0x6A: return "MS4024AE";   
    case 0x6B: return "MS4024PAE";
    case 0x6F: return "MS4448AE";   
    case 0x70: return "MS3748AEJ";
    case 0x72: return "MS4048";     
    case 0x73: return "MS4448PAE";
    case 0x74: return "MS3748PAEJ"; 
    case 0x75: return "MS4348PE";
    default: return "Unknown";
  }
}

const char *MagnumInverter::fault_to_str_(uint8_t f) {
  switch (f) {
    case 0x00: return "None";       
    case 0x01: return "Stuck relay";
    case 0x02: return "DC overload";  
    case 0x03: return "AC overload";
    case 0x04: return "Dead battery"; 
    case 0x05: return "Backfeed";
    case 0x08: return "Low battery";  
    case 0x09: return "High battery";
    case 0x0A: return "High AC volts";
    case 0x10: return "Bad bridge";
    case 0x12: return "NTC fault";    
    case 0x13: return "FET overload";
    case 0x14: return "Internal fault 4";
    case 0x16: return "Stacker mode fault";
    case 0x17: return "Stacker no clk fault";
    case 0x18: return "Stacker clk ph fault";
    case 0x19: return "Stacker ph loss fault";
    case 0x20: return "Over temp";    
    case 0x21: return "Relay fault";
    case 0x80: return "Charger fault";
    case 0x81: return "High battery temp";
    case 0x90: return "Open SELCO TCO"; 
    case 0x91: return "CB3 open fault";
    default:   return "Unknown fault";
  }
}

const char *MagnumInverter::bmk_fault_to_str_(uint8_t f) {
  switch (f) {
    case 0: return "Reserved"; 
    case 1: return "Normal"; 
    case 2: return "Fault start";
    default: return "Unknown";
  }
}

const char *MagnumInverter::mode_to_str_(uint8_t m) {
  switch (m) {
    case 0x00: return "Standby";   
    case 0x01: return "EQ";
    case 0x02: return "Float";     
    case 0x04: return "Absorb";
    case 0x08: return "Bulk";      
    case 0x09: return "Battery Saver";
    case 0x10: return "Charge";    
    case 0x20: return "Off";
    case 0x40: return "Invert";    
    case 0x50: return "Inverter Standby";
    case 0x80: return "Search";    
    default:   return "Unknown";
  }
}

const char *MagnumInverter::stackmode_to_str_(uint8_t s) {
  switch (s) {
    case 0x00: return "Stand Alone"; 
    case 0x01: return "Parallel master";
    case 0x02: return "Parallel slave"; 
    case 0x04: return "Series master";
    case 0x08: return "Series slave"; 
    default: return "Unknown";
  }
}

const char *MagnumInverter::battery_type_to_str_(uint8_t t) {
  if (t >= 100) return "Custom";
  switch (t) {
    case 2:  return "Gel";
    case 4:  return "Flooded";
    case 8:  return "AGM";
    case 10: return "AGM2";
    default: return "Unknown";
  }
}

static bool is_valid_mode(uint8_t m) {
  return m==0x00||m==0x01||m==0x02||m==0x04||m==0x08||m==0x09||
         m==0x10||m==0x20||m==0x40||m==0x50||m==0x80;
}

static bool is_remote_subtype(uint8_t b) {
  return b==0xA0||b==0xA1||b==0xA2||b==0xA3||b==0xA4||
         b==0x80||b==0x11||b==0xC0||b==0xC1||b==0xC2||b==0xC3||b==0xD0;
}

// ==========================================================================
// setup / loop
// ==========================================================================

void MagnumInverter::setup() {
  ESP_LOGI(TAG, "Magnum Inverter component starting (scan-based, v3)");
  buf_.reserve(256);
  startup_ms_ = millis();
}

void MagnumInverter::loop() {
  // Drain UART into our buffer
  while (available()) {
    uint8_t b;
    read_byte(&b);
    buf_.push_back(b);
    last_byte_rx_us_ = micros();
  }

  // Toggle TX: use busy-wait to transmit in the 10ms gap after inverter TX
  // The inverter TXs every 100ms, then the remote responds 10ms later.
  // We need to beat the real remote by transmitting within ~5ms of the last byte.
  // Since ESPHome's loop isn't fast enough, we busy-wait here when a toggle is active.
  if (toggle_active_ && have_remote_template_ && toggle_frames_sent_ < TOGGLE_FRAME_COUNT) {
    uint32_t now = millis();

    // Safety timeout
    if ((now - toggle_start_ms_) > TOGGLE_TIMEOUT_MS) {
      ESP_LOGW(TAG, "Toggle timeout, aborting (sent %u frames)", toggle_frames_sent_);
      toggle_active_ = false;
    } else {
      // Busy-wait: spin for up to 120ms watching for a bus idle gap of 2-5ms
      // This blocks the main loop but ensures we catch the precise timing window
      uint32_t spin_start = millis();
      bool sent = false;

      while ((millis() - spin_start) < 120 && !sent) {
        // Keep draining UART
        while (available()) {
          uint8_t b;
          read_byte(&b);
          buf_.push_back(b);
          last_byte_rx_us_ = micros();
        }

        uint32_t idle_us = micros() - last_byte_rx_us_;

        // Sweet spot: 2-7ms of silence = just after inverter finished, before remote starts
        if (idle_us >= 2000 && idle_us <= 7000) {
          uint8_t cmd[21];
          memcpy(cmd, last_remote_pkt_, 21);
          cmd[0] = 0x01;  // toggle command

          write_array(cmd, 21);
          flush();
          toggle_frames_sent_++;
          sent = true;

          // Wait for our TX to complete before resuming (~11ms at 19200 baud)
          delayMicroseconds(12000);
          last_byte_rx_us_ = micros();

          ESP_LOGI(TAG, "Toggle TX[%u/%u]: sent at %uus after last byte (busywait %ums)",
                   toggle_frames_sent_, TOGGLE_FRAME_COUNT,
                   (unsigned)idle_us, (unsigned)(millis() - spin_start));

          if (toggle_frames_sent_ >= TOGGLE_FRAME_COUNT) {
            ESP_LOGI(TAG, "Toggle complete: %u command frames sent", toggle_frames_sent_);
            toggle_active_ = false;
          }
        }
      }

      if (!sent) {
        ESP_LOGD(TAG, "Toggle: no suitable gap found this loop iteration");
      }
    }
  }

  // Prevent unbounded growth
  if (buf_.size() > 2048) {
    ESP_LOGW(TAG, "Buffer overflow (%u), trimming", (unsigned)buf_.size());
    buf_.erase(buf_.begin(), buf_.end() - 256);
    rejected_count_++;
  }

  // Debug: dump the first few raw buffers so we can see what's actually in there
  if (dump_count_ < 5 && buf_.size() >= 40 && startup_logging_()) {
    dump_count_++;
    size_t dump_len = std::min(buf_.size(), (size_t)80);
    char hex[dump_len * 3 + 1];
    for (size_t i = 0; i < dump_len; i++) {
      snprintf(hex + i * 3, 4, "%02X ", buf_[i]);
    }
    hex[dump_len * 3] = '\0';
    ESP_LOGI(TAG, "RAW buf[%u]: %s", (unsigned)buf_.size(), hex);
  }

  // On-demand packet recording: log all raw buffer contents for RECORD_DURATION_MS
  if (recording_) {
    uint32_t elapsed = millis() - record_start_ms_;
    if (elapsed < RECORD_DURATION_MS) {
      if (buf_.size() > 0) {
        record_pkt_count_++;
        // Dump up to 256 bytes per loop iteration
        size_t dump_len = std::min(buf_.size(), (size_t)256);
        char hex[dump_len * 3 + 1];
        for (size_t i = 0; i < dump_len; i++) {
          snprintf(hex + i * 3, 4, "%02X ", buf_[i]);
        }
        hex[dump_len * 3] = '\0';
        ESP_LOGI(TAG, "REC[%u] @%ums buf[%u]: %s",
                 (unsigned)record_pkt_count_, (unsigned)elapsed,
                 (unsigned)buf_.size(), hex);
      }
    } else {
      ESP_LOGI(TAG, "Recording complete: %u snapshots captured in %ums",
               (unsigned)record_pkt_count_, (unsigned)elapsed);
      recording_ = false;
    }
  }

  // Scan for packets
  scan_buffer_();

  // Throttled publish
  maybe_publish_();
}

// ==========================================================================
// Buffer scanner
// ==========================================================================
//
// From the UART debug logs we know the traffic pattern.  Packets appear
// back-to-back in the buffer with NO framing bytes.  We scan forward
// trying to match known packet signatures at each position.
//
// Key anchors from your traffic:
//   INVERTER 22 bytes: mode@0 is valid, model@14=0x6B, Hz@18-19 is ~0x025A
//   REMOTE   21 bytes: last byte is a known subtype (0x80,0xA0,0xA1,0x11,0xC0,0xD0,etc)
//                      or last 7 bytes are 0x00 (REMOTE_00)
//   BMK      18 bytes: first byte = 0x81
//   RTR       2 bytes: first byte = 0x91

void MagnumInverter::scan_buffer_() {
  if (buf_.size() < 2) return;

  size_t pos = 0;

  while (pos < buf_.size()) {
    size_t remaining = buf_.size() - pos;

    // ---- RTR: 2 bytes, starts with 0x91 ----
    if (remaining >= 2 && buf_[pos] == 0x91) {
      float rev = buf_[pos+1] / 10.0f;
      rtr_frame_count_++;
      rtr_revision_val_ = rev;
      ESP_LOGD(TAG, "RTR @%u: rev=%.1f", (unsigned)pos, rev);
      last_any_frame_ms_ = millis();
      pos += 2;
      continue;
    }

    // ---- BMK: 18 bytes, starts with 0x81 ----
    if (remaining >= 18 && buf_[pos] == 0x81) {
      uint16_t bmk_v_raw = be_u16(&buf_[pos], 2);
      if (bmk_v_raw >= 500 && bmk_v_raw <= 7000) {  // 5.00V to 70.00V
        if (startup_logging_()) {
          ESP_LOGI(TAG, "BMK candidate @%u: v_raw=%u (%.2fV)", (unsigned)pos, bmk_v_raw, bmk_v_raw/100.0f);
        }
        process_bmk_(&buf_[pos], 18);
        last_any_frame_ms_ = millis();
        pos += 18;
        continue;
      } else {
        if (startup_logging_()) {
          ESP_LOGW(TAG, "BMK rejected: v_raw=%u out of range", bmk_v_raw);
        }
      }
    }

    // ---- INVERTER: 22 bytes raw (we consume 22, parse 21) ----
    // Anchor: model byte at offset 14 must be a known inverter model
    // Secondary: mode byte at offset 0 must be a valid mode
    // Tertiary: voltage at bytes 2-3 should be reasonable
    if (remaining >= 22) {
      uint8_t mode_byte = buf_[pos];
      uint8_t model_byte = buf_[pos + 14];

      if (is_valid_mode(mode_byte) && is_known_model(model_byte)) {
        uint16_t v10 = be_u16(&buf_[pos], 2);
        if (v10 >= 80 && v10 <= 800) {  // 8.0V - 80.0V
          process_inverter_(&buf_[pos], 21, false);
          last_any_frame_ms_ = millis();
          pos += 22;  // 22 raw bytes
          continue;
        }
      }
    }

    // ---- REMOTE: 21 bytes, identified by last byte subtype ----
    if (remaining >= 21) {
      uint8_t last_byte = buf_[pos + 20];

      // Known non-zero subtypes
      if (is_remote_subtype(last_byte)) {
        process_remote_(&buf_[pos], 21, last_byte);
        last_any_frame_ms_ = millis();
        pos += 21;
        continue;
      }

      // REMOTE_00: last byte is 0x00 with last 7 bytes all zero
      if (last_byte == 0x00) {
        bool seven_zeros = true;
        for (int k = 14; k <= 20; k++) {
          if (buf_[pos + k] != 0x00) { seven_zeros = false; break; }
        }
        if (seven_zeros) {
          process_remote_(&buf_[pos], 21, 0x00);
          last_any_frame_ms_ = millis();
          pos += 21;
          continue;
        }
      }
    }

    // ---- Nothing matched at pos, advance one byte ----
    pos++;
    rejected_count_++;
  }

  // Remove consumed/scanned bytes
  if (pos > 0 && pos <= buf_.size()) {
    buf_.erase(buf_.begin(), buf_.begin() + pos);
  }
}

// ==========================================================================
// Inverter processing (21 bytes of a 22-byte raw frame)
// ==========================================================================

void MagnumInverter::process_inverter_(const uint8_t *data, size_t len, bool classic) {
  inv_frame_count_++;

  uint8_t mode  = data[0];
  uint8_t fault = data[1];
  float vdc     = be_i16(data, 2) / 10.0f;
  float adc     = (float)be_i16(data, 4);
  float vac_out = (float)data[6];
  float vac_in  = (float)data[7];
  uint8_t inv_led = data[8];     // 0 = off, nonzero = on
  uint8_t chg_led = data[9];     // 0 = off, nonzero = on
  uint8_t revision = data[10];   // e.g. 40 = v4.0
  float bat_t   = (float)data[11];
  float xfmr_t  = (float)data[12];
  float fet_t   = (float)data[13];
  uint8_t model = data[14];

  uint8_t stackmode = 0;
  float aac_in = 0, aac_out = 0, hz = 0;

  if (!classic && len >= 21) {
    stackmode = data[15];
    aac_in  = (float)data[16];
    aac_out = (float)data[17];
    hz = be_i16(data, 18) / 10.0f;
  }

  if (model <= 50)       voltage_multiplier_ = 1;
  else if (model <= 107) voltage_multiplier_ = 2;
  else if (model <= 150) voltage_multiplier_ = 4;

  cached_batt_v_ = vdc;
  cached_dc_a_ = adc;
  cached_ac_out_v_ = vac_out;

  inv_pending_ = true;
  pend_batt_v_ = vdc; pend_dc_a_ = adc;
  pend_ac_out_v_ = vac_out; pend_ac_in_v_ = vac_in;
  pend_hz_ = hz;
  pend_batt_temp_ = bat_t; pend_xfmr_temp_ = xfmr_t; pend_fet_temp_ = fet_t;
  pend_ac_out_a_ = aac_out; pend_ac_in_a_ = aac_in;
  pend_mode_ = mode; pend_fault_ = fault;
  pend_model_ = model; pend_stackmode_ = stackmode;
  pend_inv_led_ = inv_led; pend_chg_led_ = chg_led; pend_inv_revision_ = revision;

  if (fault != 0x00 && fault != latched_fault_) {
    latched_fault_ = fault;
    if (last_fault_code_) last_fault_code_->publish_state(fault);
    if (last_fault_text_) {
      char tmp[80];
      snprintf(tmp, sizeof(tmp), "%s (0x%02X)", fault_to_str_(fault), fault);
      last_fault_text_->publish_state(tmp);
    }
    if (last_fault_time_ && time_ && time_->now().is_valid()) {
      auto t = time_->now();
      char ts[32];
      snprintf(ts, sizeof(ts), "%04d-%02d-%02d %02d:%02d:%02d",
               t.year, t.month, t.day_of_month, t.hour, t.minute, t.second);
      last_fault_time_->publish_state(ts);
    }
  }

  ESP_LOGD(TAG, "INV: mode=0x%02X(%s) fault=0x%02X V=%.1f A=%.0f ACout=%u ACin=%u Hz=%.1f model=0x%02X(%s)",
           mode, mode_to_str_(mode), fault, vdc, adc, data[6], data[7], hz, model, model_to_str_(model));
}

// ==========================================================================
// BMK processing (18 bytes)
// ==========================================================================

void MagnumInverter::process_bmk_(const uint8_t *data, size_t len) {
  if (len < 18) return;

  bmk_frame_count_++;
  last_bmk_ms_ = millis();

  int8_t soc = (int8_t)data[1];
  float vdc  = be_u16(data, 2) / 100.0f;
  float adc  = be_i16(data, 4) / 10.0f;
  float vmin = be_u16(data, 6) / 100.0f;
  float vmax = be_u16(data, 8) / 100.0f;
  int16_t amph = be_i16(data, 10);
  float amphtrip = be_u16(data, 12) / 10.0f;
  float cumah    = be_u16(data, 14) * 100.0f;   // cumulative AmpH out of battery
  uint8_t bmk_rev = data[16];                     // revision (e.g. 10 = v1.0)
  uint8_t fault = data[17];                       // 0=reserved, 1=normal, 2=fault

  cached_bmk_v_ = vdc; cached_bmk_a_ = adc;

  bmk_pending_ = true;
  pend_bmk_soc_ = (float)soc; pend_bmk_v_ = vdc; pend_bmk_a_ = adc;
  pend_bmk_vmin_ = vmin; pend_bmk_vmax_ = vmax;
  pend_bmk_ah_ = (float)amph; pend_bmk_ah_trip_ = amphtrip;
  pend_bmk_cumah_ = cumah; pend_bmk_revision_ = bmk_rev;
  pend_bmk_fault_ = fault;

  ESP_LOGD(TAG, "BMK: SOC=%d%% V=%.2f A=%.1f Vmin=%.2f Vmax=%.2f Ah=%d fault=%u",
           soc, vdc, adc, vmin, vmax, amph, fault);
}

// ==========================================================================
// Remote processing
// ==========================================================================

void MagnumInverter::process_remote_(const uint8_t *data, size_t len, uint8_t subtype) {
  remote_frame_count_++;

  // Per Magnum protocol spec, Remote sends 21 bytes:
  //   Byte 0:  Inverter ON/OFF (bit 0 = toggle inv, bit 1 = toggle charger)
  //   Byte 1:  Search watts (0=defeated, 5=default, 5-50W)
  //   Byte 2:  Battery size (1 count = 10Ah)
  //   Byte 3:  Battery type (2=Gel, 4=Flooded, 8=AGM, 10=AGM2, 100+=Custom absorb V)
  //   Byte 4:  Charger amps (0-100, steps of 10, percent of max)
  //   Byte 5:  AC shore amps (5-60)
  //   Byte 6:  Remote revision (e.g. 40 = 4.0)
  //   Byte 7:  Ambient temp (0-150 °C)
  //   Byte 8:  Auto genstart (0=off, 1=enable, 2=test, 4=quiet, 5=on)
  //   Byte 9:  LBCO voltage (0.1V resolution, scaled to 12V numbers)
  //   Byte 10: VAC cutout voltage (non-linear, 155=80V default, 255=EMS override)
  //   Byte 11: Float volts (0.1V resolution, scaled to 12V numbers)
  //   Byte 12: EQ volts (0-20, added to absorb voltage, 0.1V resolution)
  //   Byte 13: Absorb time (0.1hr increments, e.g. 20 = 2.0 hrs)
  //   Byte 14: Hours (0-23) -- or AGS/BMK data depending on subtype
  //   Byte 15: Minutes (0-59) -- or AGS/BMK data depending on subtype
  //   Bytes 16-19: AGS/BMK data depending on footer
  //   Byte 20: Footer/subtype (0x80=BMK, 0xA0/A1/A2/A3/A4=AGS, 0x00=base, etc.)

  if (len < 21) return;

  // Capture the last-seen remote packet as a template for toggle commands
  // (only capture normal packets where byte 0 is 0x00, not command packets)
  if (data[0] == 0x00) {
    memcpy(last_remote_pkt_, data, 21);
    have_remote_template_ = true;
  }

  // Parse base remote settings from every packet (bytes 1-13 are consistent)
  // voltage_multiplier_: 1=12V, 2=24V, 4=48V (set from inverter model)
  float vm = (float)voltage_multiplier_;

  float searchwatts  = (float)data[1];                 // 0-50 watts
  float battery_size = (float)data[2] * 10.0f;         // 1 count = 10Ah
  uint8_t batt_type  = data[3];                        // 2/4/8/10/100+
  float chargeramps  = (float)data[4];                 // 0-100 percent
  float shore_amps   = (float)data[5];                 // 5-60 amps
  // data[6] = remote revision, data[7] = ambient temp, data[8] = genstart
  float lbco_v       = data[9] / 10.0f * vm;           // scaled to 12V, multiply by vm
  float vac_cutout   = (float)data[10];                // raw value (non-linear)
  float float_v      = data[11] / 10.0f * vm;          // scaled to 12V
  float eq_offset    = data[12] / 10.0f * vm;          // 0-2.0V added to absorb for EQ voltage
  float absorb_time  = data[13] / 10.0f;               // 0.1hr resolution

  // Absorb voltage: if battery_type >= 100, it IS the absorb voltage (scaled to 12V)
  // Otherwise it's derived from the battery type preset
  float absorb_v;
  if (batt_type >= 100) {
    absorb_v = batt_type / 10.0f * vm;                 // custom: e.g. 144 = 14.4V * vm
  } else {
    // Standard presets (12V base values):
    // Gel=14.1, Flooded=14.6, AGM=14.4, AGM2=14.4
    float base;
    switch (batt_type) {
      case 2:  base = 14.1f; break;  // Gel
      case 4:  base = 14.6f; break;  // Flooded
      case 8:  base = 14.4f; break;  // AGM
      case 10: base = 14.4f; break;  // AGM2
      default: base = 14.4f; break;
    }
    absorb_v = base * vm;
  }

  // Per Magnum spec: EQ voltage = absorb voltage + EQ offset
  float eq_v = absorb_v + eq_offset;

  remote_pending_ = true;
  pend_remote_searchwatts_ = searchwatts;
  pend_remote_battery_size_ = battery_size;
  pend_remote_battery_type_ = batt_type;
  pend_remote_chargeramps_ = chargeramps;
  pend_remote_shore_amps_ = shore_amps;
  pend_remote_lbco_ = lbco_v;
  pend_remote_vac_cutout_ = vac_cutout;
  pend_remote_float_ = float_v;
  pend_remote_eq_ = eq_v;
  pend_remote_absorb_time_ = absorb_time;
  pend_remote_absorb_ = absorb_v;

  if (startup_logging_()) {
    ESP_LOGI(TAG, "REMOTE[0x%02X]: search=%.0fW batt=%.0fAh type=%u chg=%.0f%% shore=%.0fA lbco=%.1fV float=%.1fV eq=%.1fV absorb=%.1fV abstime=%.1fh vac=%u vm=%.0f",
             subtype, searchwatts, battery_size, batt_type, chargeramps, shore_amps,
             lbco_v, float_v, eq_v, absorb_v, absorb_time, data[10], vm);
  }

  ESP_LOGD(TAG, "REMOTE: subtype=0x%02X", subtype);
}

// ==========================================================================
// Inverter toggle (on/off command)
// ==========================================================================

void MagnumInverter::send_inverter_toggle() {
  if (!have_remote_template_) {
    ESP_LOGW(TAG, "Cannot toggle: no remote packet template captured yet (wait for remote traffic)");
    return;
  }
  if (toggle_active_) {
    ESP_LOGW(TAG, "Toggle already in progress, ignoring");
    return;
  }

  ESP_LOGI(TAG, "Starting inverter toggle sequence (current mode=0x%02X)", pend_mode_);

  toggle_active_ = true;
  toggle_start_ms_ = millis();
  toggle_frames_sent_ = 0;
}

// ==========================================================================
// Throttled publish
// ==========================================================================

void MagnumInverter::maybe_publish_() {
  const uint32_t now = millis();
  if ((now - last_publish_ms_) < 1000) return;
  last_publish_ms_ = now;

  if (inv_pending_) {
    inv_pending_ = false;
    if (batt_v_)    batt_v_->publish_state(pend_batt_v_);
    if (dc_a_)      dc_a_->publish_state(pend_dc_a_);
    if (ac_out_v_)  ac_out_v_->publish_state(pend_ac_out_v_);
    if (ac_in_v_)   ac_in_v_->publish_state(pend_ac_in_v_);
    if (hz_)        hz_->publish_state(pend_hz_);
    if (batt_temp_) batt_temp_->publish_state(pend_batt_temp_);
    if (xfmr_temp_) xfmr_temp_->publish_state(pend_xfmr_temp_);
    if (fet_temp_)  fet_temp_->publish_state(pend_fet_temp_);
    if (mode_) mode_->publish_state(mode_to_str_(pend_mode_));
    if (inv_fault_code_) inv_fault_code_->publish_state(pend_fault_);
    if (inv_fault_text_) {
      char tmp[80];
      snprintf(tmp, sizeof(tmp), "%s (0x%02X)", fault_to_str_(pend_fault_), pend_fault_);
      inv_fault_text_->publish_state(tmp);
    }
    if (inv_fault_active_) inv_fault_active_->publish_state(pend_fault_ != 0 ? 1.0f : 0.0f);
    if (inv_model_text_) inv_model_text_->publish_state(model_to_str_(pend_model_));
    if (inv_stackmode_text_) inv_stackmode_text_->publish_state(stackmode_to_str_(pend_stackmode_));
    if (inv_revision_) inv_revision_->publish_state(pend_inv_revision_ / 10.0f);
    if (inv_led_) inv_led_->publish_state(pend_inv_led_ != 0 ? "On" : "Off");
    if (chg_led_) chg_led_->publish_state(pend_chg_led_ != 0 ? "On" : "Off");
    if (ac_out_a_) ac_out_a_->publish_state(pend_ac_out_a_);
    if (ac_out_w_) ac_out_w_->publish_state(pend_ac_out_v_ * pend_ac_out_a_);
    if (ac_in_a_)  ac_in_a_->publish_state(pend_ac_in_a_);
    if (batt_w_) {
      bool bmk_recent = (last_bmk_ms_ != 0) && ((now - last_bmk_ms_) < 5000) &&
                        std::isfinite(cached_bmk_v_) && std::isfinite(cached_bmk_a_);
      float raw_w = bmk_recent ? cached_bmk_v_ * cached_bmk_a_ : pend_batt_v_ * pend_dc_a_;
      batt_w_->publish_state(round1(raw_w));
    }
  }

  if (bmk_pending_) {
    bmk_pending_ = false;
    if (bmk_soc_)      bmk_soc_->publish_state(pend_bmk_soc_);
    if (bmk_v_)        bmk_v_->publish_state(pend_bmk_v_);
    if (bmk_a_)        bmk_a_->publish_state(pend_bmk_a_);
    if (bmk_vmin_)     bmk_vmin_->publish_state(pend_bmk_vmin_);
    if (bmk_vmax_)     bmk_vmax_->publish_state(pend_bmk_vmax_);
    if (bmk_ah_inout_) bmk_ah_inout_->publish_state(pend_bmk_ah_);
    if (bmk_ah_trip_)  bmk_ah_trip_->publish_state(pend_bmk_ah_trip_);
    if (bmk_cumah_)    bmk_cumah_->publish_state(pend_bmk_cumah_);
    if (bmk_revision_) bmk_revision_->publish_state(pend_bmk_revision_ / 10.0f);
    if (bmk_w_)        bmk_w_->publish_state(round1(pend_bmk_v_ * pend_bmk_a_));
    if (bmk_fault_text_) bmk_fault_text_->publish_state(bmk_fault_to_str_(pend_bmk_fault_));
  }

  if (inv_frames_)    inv_frames_->publish_state((float)inv_frame_count_);
  if (bmk_frames_)    bmk_frames_->publish_state((float)bmk_frame_count_);
  if (remote_frames_) remote_frames_->publish_state((float)remote_frame_count_);
  if (rejected_)      rejected_->publish_state((float)rejected_count_);

  // ---- RTR ----
  if (rtr_frames_) rtr_frames_->publish_state((float)rtr_frame_count_);
  if (rtr_revision_ && std::isfinite(rtr_revision_val_)) rtr_revision_->publish_state(rtr_revision_val_);
  if (rtr_model_text_ && std::isfinite(rtr_revision_val_)) {
    char rtr_str[32];
    snprintf(rtr_str, sizeof(rtr_str), "ME-ARTR v%.1f", rtr_revision_val_);
    rtr_model_text_->publish_state(rtr_str);
  }

  // ---- Remote/ARTR settings ----
  if (remote_pending_) {
    remote_pending_ = false;
    if (remote_searchwatts_) remote_searchwatts_->publish_state(pend_remote_searchwatts_);
    if (remote_chargeramps_) remote_chargeramps_->publish_state(pend_remote_chargeramps_);
    if (remote_absorb_) remote_absorb_->publish_state(round1(pend_remote_absorb_));
    if (remote_float_) remote_float_->publish_state(round1(pend_remote_float_));
    if (remote_eq_) remote_eq_->publish_state(round1(pend_remote_eq_));
    if (remote_lbco_) remote_lbco_->publish_state(round1(pend_remote_lbco_));
    if (remote_battery_size_) remote_battery_size_->publish_state(pend_remote_battery_size_);
    if (remote_shore_amps_) remote_shore_amps_->publish_state(pend_remote_shore_amps_);
    if (remote_vac_cutout_) remote_vac_cutout_->publish_state(pend_remote_vac_cutout_);
    if (remote_absorb_time_) remote_absorb_time_->publish_state(round1(pend_remote_absorb_time_));
    if (remote_battery_type_) remote_battery_type_->publish_state(battery_type_to_str_(pend_remote_battery_type_));
  }

  // ---- Inverter on/off (derived from mode) ----
  if (inv_on_) {
    inv_on_->publish_state(pend_mode_ != 0x20 ? "On" : "Off");
  }

  if (last_frame_age_) {
    if (last_any_frame_ms_ == 0) last_frame_age_->publish_state(NAN);
    else last_frame_age_->publish_state((now - last_any_frame_ms_) / 1000.0f);
  }
}

}  // namespace magnum_inverter