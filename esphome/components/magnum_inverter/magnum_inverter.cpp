#include "magnum_inverter.h"
#include "esphome/core/log.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace esphome {
namespace magnum_inverter {

static const char *const TAG = "magnum_inv";

// A frame split across UART reads is held until the rest arrives. If the bus
// stays quiet this long, the partial frame is dropped.
static const uint32_t PARTIAL_FRAME_TIMEOUT_MS = 30;
static const uint32_t PUBLISH_INTERVAL_MS = 1000;
static const uint32_t DIAG_PUBLISH_INTERVAL_MS = 10000;

// Round to 1 decimal place using integer math to avoid float precision artifacts
static inline float round1(float v) { return (int32_t) (v * 10.0f + (v >= 0 ? 0.5f : -0.5f)) / 10.0f; }

// ==========================================================================
// Lookup tables
// ==========================================================================

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
  if (t > 100) return "Custom";
  switch (t) {
    case 2:  return "Gel";
    case 4:  return "Flooded";
    case 8:  return "AGM";
    case 10: return "AGM2";
    default: return "Unknown";
  }
}

// ==========================================================================
// setup / loop
// ==========================================================================

void MagnumInverter::setup() { startup_ms_ = millis(); }

void MagnumInverter::dump_config() {
  ESP_LOGCONFIG(TAG, "Magnum Inverter:");
  ESP_LOGCONFIG(TAG, "  Stale timeout: %u ms", (unsigned) stale_timeout_ms_);
  this->check_uart_settings(19200, 1, uart::UART_CONFIG_PARITY_NONE, 8);
}

void MagnumInverter::read_uart_() {
  uint8_t chunk[64];
  size_t n;
  while ((n = std::min<size_t>(available(), sizeof(chunk))) > 0) {
    if (!read_array(chunk, n))
      break;
    last_rx_ms_ = millis();
    last_byte_rx_us_ = micros();

    if (recording_ && (last_rx_ms_ - record_start_ms_) < RECORD_DURATION_MS) {
      // Log bytes as they arrive, 32 per line, so lines fit the logger buffer
      for (size_t off = 0; off < n; off += 32) {
        size_t cnt = std::min<size_t>(32, n - off);
        char hex[32 * 3 + 1];
        for (size_t i = 0; i < cnt; i++)
          snprintf(hex + i * 3, 4, "%02X ", chunk[off + i]);
        hex[cnt * 3] = '\0';
        ESP_LOGI(TAG, "REC +%ums: %s", (unsigned) (last_rx_ms_ - record_start_ms_), hex);
      }
      record_bytes_ += n;
    }

    scanner_.feed(chunk, n, [this](FrameType t, const uint8_t *d, size_t) { this->on_frame_(t, d); });
  }
}

void MagnumInverter::loop() {
  read_uart_();

  if (scanner_.pending() > 0 && (millis() - last_rx_ms_) > PARTIAL_FRAME_TIMEOUT_MS)
    scanner_.flush([this](FrameType t, const uint8_t *d, size_t) { this->on_frame_(t, d); });

  if (recording_ && (millis() - record_start_ms_) >= RECORD_DURATION_MS) {
    ESP_LOGI(TAG, "Recording complete: %u bytes captured", (unsigned) record_bytes_);
    recording_ = false;
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
        read_uart_();

        uint32_t idle_us = micros() - last_byte_rx_us_;

        // Sweet spot: 2-7ms of silence = just after inverter finished, before remote starts
        if (idle_us >= 2000 && idle_us <= 7000) {
          uint8_t cmd[REMOTE_LEN];
          memcpy(cmd, last_remote_pkt_, REMOTE_LEN);
          cmd[0] = 0x01;  // toggle command

          write_array(cmd, REMOTE_LEN);
          flush();
          toggle_frames_sent_++;
          sent = true;

          // Wait for our TX to complete before resuming (~11ms at 19200 baud)
          delayMicroseconds(12000);
          last_byte_rx_us_ = micros();

          ESP_LOGI(TAG, "Toggle TX[%u/%u]: sent at %uus after last byte (busywait %ums)", toggle_frames_sent_,
                   TOGGLE_FRAME_COUNT, (unsigned) idle_us, (unsigned) (millis() - spin_start));

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

  maybe_publish_();
}

void MagnumInverter::on_frame_(FrameType type, const uint8_t *data) {
  last_any_frame_ms_ = millis();
  switch (type) {
    case FrameType::INVERTER:
      process_inverter_(data);
      break;
    case FrameType::REMOTE:
      process_remote_(data);
      break;
    case FrameType::BMK:
      process_bmk_(data);
      break;
    case FrameType::RTR:
      rtr_frame_count_++;
      rtr_revision_val_ = data[1] / 10.0f;
      ESP_LOGV(TAG, "RTR: rev=%.1f", rtr_revision_val_);
      break;
  }
}

// ==========================================================================
// Inverter processing (21 bytes)
// ==========================================================================

void MagnumInverter::process_inverter_(const uint8_t *data) {
  inv_frame_count_++;
  last_inv_ms_ = millis();
  inv_stale_published_ = false;

  uint8_t mode  = data[0];
  uint8_t fault = data[1];
  float vdc     = be_i16(data, 2) / 10.0f;
  float adc     = (float) be_i16(data, 4);
  float vac_out = (float) data[6];
  float vac_in  = (float) data[7];   // peak, not RMS
  uint8_t inv_led = data[8];         // 0 = off, nonzero = on
  uint8_t chg_led = data[9];         // 0 = off, nonzero = on
  uint8_t revision = data[10];       // e.g. 40 = v4.0
  float bat_t   = (float) data[11];
  float xfmr_t  = (float) data[12];
  float fet_t   = (float) data[13];
  uint8_t model = data[14];
  uint8_t stackmode = data[15];
  float aac_in  = (float) data[16];
  float aac_out = (float) data[17];
  float hz      = be_i16(data, 18) / 10.0f;

  if (model <= 50)       voltage_multiplier_ = 1;
  else if (model <= 107) voltage_multiplier_ = 2;
  else if (model <= 150) voltage_multiplier_ = 4;

  inv_pending_ = true;
  pend_batt_v_ = vdc; pend_dc_a_ = adc;
  pend_ac_out_v_ = vac_out; pend_ac_in_v_ = vac_in;
  pend_hz_ = hz;
  pend_batt_temp_ = bat_t; pend_xfmr_temp_ = xfmr_t; pend_fet_temp_ = fet_t;
  pend_ac_out_a_ = aac_out; pend_ac_in_a_ = aac_in;
  pend_mode_ = mode; pend_fault_ = fault;
  pend_model_ = model; pend_stackmode_ = stackmode;
  pend_inv_led_ = inv_led; pend_chg_led_ = chg_led; pend_inv_revision_ = revision;

  record_fault_(fault);

  ESP_LOGV(TAG, "INV: mode=0x%02X(%s) fault=0x%02X V=%.1f A=%.0f ACout=%u ACin=%u Hz=%.1f model=0x%02X(%s)", mode,
           mode_to_str_(mode), fault, vdc, adc, data[6], data[7], hz, model, model_to_str_(model));
}

// Publishes the last-fault sensors each time a fault starts, including a
// repeat of the same fault after it has cleared.
void MagnumInverter::record_fault_(uint8_t fault) {
  if (fault == active_fault_)
    return;
  active_fault_ = fault;
  if (fault == 0x00)
    return;

  ESP_LOGW(TAG, "Inverter fault: %s (0x%02X)", fault_to_str_(fault), fault);
  if (last_fault_code_) last_fault_code_->publish_state(fault);
  if (last_fault_text_) {
    char tmp[48];
    snprintf(tmp, sizeof(tmp), "%s (0x%02X)", fault_to_str_(fault), fault);
    last_fault_text_->publish_state(tmp);
  }
#ifdef USE_TIME
  if (last_fault_time_ && time_) {
    auto t = time_->utcnow();
    if (t.is_valid()) {
      char ts[24];
      t.strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ");
      last_fault_time_->publish_state(ts);
    }
  }
#endif
}

// ==========================================================================
// BMK processing (18 bytes)
// ==========================================================================

void MagnumInverter::process_bmk_(const uint8_t *data) {
  bmk_frame_count_++;
  last_bmk_ms_ = millis();
  bmk_stale_published_ = false;

  int8_t soc = (int8_t) data[1];
  float vdc  = be_u16(data, 2) / 100.0f;
  float adc  = be_i16(data, 4) / 10.0f;
  float vmin = be_u16(data, 6) / 100.0f;
  float vmax = be_u16(data, 8) / 100.0f;
  int16_t amph = be_i16(data, 10);
  float amphtrip = be_u16(data, 12) / 10.0f;
  float cumah    = be_u16(data, 14) * 100.0f;   // cumulative AmpH out of battery
  uint8_t bmk_rev = data[16];                     // revision (e.g. 10 = v1.0)
  uint8_t fault = data[17];                       // 0=reserved, 1=normal, 2=fault

  bmk_pending_ = true;
  pend_bmk_soc_ = (float) soc; pend_bmk_v_ = vdc; pend_bmk_a_ = adc;
  pend_bmk_vmin_ = vmin; pend_bmk_vmax_ = vmax;
  pend_bmk_ah_ = (float) amph; pend_bmk_ah_trip_ = amphtrip;
  pend_bmk_cumah_ = cumah; pend_bmk_revision_ = bmk_rev;
  pend_bmk_fault_ = fault;

  ESP_LOGV(TAG, "BMK: SOC=%d%% V=%.2f A=%.1f Vmin=%.2f Vmax=%.2f Ah=%d fault=%u", soc, vdc, adc, vmin, vmax, amph,
           fault);
}

// ==========================================================================
// Remote processing
// ==========================================================================

void MagnumInverter::process_remote_(const uint8_t *data) {
  remote_frame_count_++;

  // Per Magnum protocol spec, Remote sends 21 bytes:
  //   Byte 0:  Inverter ON/OFF (bit 0 = toggle inv, bit 1 = toggle charger)
  //   Byte 1:  Search watts (0=defeated, 5=default, 5-50W)
  //   Byte 2:  Battery size (1 count = 10Ah)
  //   Byte 3:  Battery type (2=Gel, 4=Flooded, 8=AGM, 10=AGM2, >100 = custom absorb V)
  //   Byte 4:  Charger amps (0-100, steps of 10, percent of max)
  //   Byte 5:  AC shore amps (5-60)
  //   Byte 6:  Remote revision (e.g. 40 = 4.0)
  //   Byte 7:  Parallel threshold / force charge
  //   Byte 8:  Auto genstart (0=off, 1=enable, 2=test, 4=quiet, 5=on)
  //   Byte 9:  LBCO voltage (0.1V resolution)
  //   Byte 10: VAC cutout voltage (non-linear, 155=80V default, 255=EMS override)
  //   Byte 11: Float volts (0.1V resolution, scaled to 12V numbers)
  //   Byte 12: EQ volts (0-20, added to absorb voltage, 0.1V resolution)
  //   Byte 13: Absorb time (0.1hr increments, e.g. 20 = 2.0 hrs)
  //   Bytes 14-19: Time or AGS/BMK data depending on the footer
  //   Byte 20: Footer/subtype (0x80=BMK, 0xA0/A1/A2/A3/A4=AGS, 0x00=base, etc.)

  // Bytes 0-13 are the same in every subtype. Only accept settings that match
  // the previous remote frame, so a single misread frame is ignored.
  bool confirmed = have_last_remote_settings_ && memcmp(last_remote_settings_, data, 14) == 0;
  memcpy(last_remote_settings_, data, 14);
  have_last_remote_settings_ = true;
  if (!confirmed)
    return;

  // Capture the last-seen remote packet as a template for toggle commands
  // (only capture normal packets where byte 0 is 0x00, not command packets)
  if (data[0] == 0x00) {
    memcpy(last_remote_pkt_, data, REMOTE_LEN);
    have_remote_template_ = true;
  }

  memcpy(pend_remote_, data, REMOTE_LEN);
  remote_pending_ = true;
  ESP_LOGV(TAG, "REMOTE: subtype=0x%02X", data[20]);
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

void MagnumInverter::publish_inverter_unknown_() {
  for (auto *s : {batt_v_, dc_a_, ac_out_v_, ac_in_v_, hz_, batt_temp_, xfmr_temp_, fet_temp_, ac_out_a_, ac_out_w_,
                  ac_in_a_, batt_w_, inv_fault_code_, inv_fault_active_}) {
    if (s) s->publish_state(NAN);
  }
  for (auto *b : {inv_led_bs_, chg_led_bs_, inv_fault_bs_}) {
    if (b) b->invalidate_state();
  }
}

void MagnumInverter::publish_bmk_unknown_() {
  for (auto *s : {bmk_soc_, bmk_v_, bmk_a_, bmk_vmin_, bmk_vmax_, bmk_ah_inout_, bmk_ah_trip_, bmk_cumah_, bmk_w_}) {
    if (s) s->publish_state(NAN);
  }
}

void MagnumInverter::maybe_publish_() {
  const uint32_t now = millis();
  if ((now - last_publish_ms_) < PUBLISH_INTERVAL_MS) return;
  last_publish_ms_ = now;

  bool inv_fresh = last_inv_ms_ != 0 && (now - last_inv_ms_) < stale_timeout_ms_;
  bool bmk_fresh = last_bmk_ms_ != 0 && (now - last_bmk_ms_) < stale_timeout_ms_;

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
      char tmp[48];
      snprintf(tmp, sizeof(tmp), "%s (0x%02X)", fault_to_str_(pend_fault_), pend_fault_);
      inv_fault_text_->publish_state(tmp);
    }
    if (inv_fault_active_) inv_fault_active_->publish_state(pend_fault_ != 0 ? 1.0f : 0.0f);
    if (inv_fault_bs_) inv_fault_bs_->publish_state(pend_fault_ != 0);
    if (inv_model_text_) inv_model_text_->publish_state(model_to_str_(pend_model_));
    if (inv_stackmode_text_) inv_stackmode_text_->publish_state(stackmode_to_str_(pend_stackmode_));
    if (inv_revision_) inv_revision_->publish_state(pend_inv_revision_ / 10.0f);
    if (inv_led_) inv_led_->publish_state(pend_inv_led_ != 0 ? "On" : "Off");
    if (chg_led_) chg_led_->publish_state(pend_chg_led_ != 0 ? "On" : "Off");
    if (inv_led_bs_) inv_led_bs_->publish_state(pend_inv_led_ != 0);
    if (chg_led_bs_) chg_led_bs_->publish_state(pend_chg_led_ != 0);
    if (inv_on_) inv_on_->publish_state(pend_mode_ != 0x20 ? "On" : "Off");
    if (ac_out_a_) ac_out_a_->publish_state(pend_ac_out_a_);
    if (ac_out_w_) ac_out_w_->publish_state(pend_ac_out_v_ * pend_ac_out_a_);
    if (ac_in_a_)  ac_in_a_->publish_state(pend_ac_in_a_);
    if (batt_w_) {
      float raw_w = bmk_fresh ? pend_bmk_v_ * pend_bmk_a_ : pend_batt_v_ * pend_dc_a_;
      batt_w_->publish_state(round1(raw_w));
    }
  } else if (!inv_fresh && last_inv_ms_ != 0 && !inv_stale_published_) {
    ESP_LOGW(TAG, "No inverter frames for %us, marking inverter sensors unknown",
             (unsigned) (stale_timeout_ms_ / 1000));
    inv_stale_published_ = true;
    publish_inverter_unknown_();
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
  } else if (!bmk_fresh && last_bmk_ms_ != 0 && !bmk_stale_published_) {
    ESP_LOGW(TAG, "No BMK frames for %us, marking BMK sensors unknown", (unsigned) (stale_timeout_ms_ / 1000));
    bmk_stale_published_ = true;
    publish_bmk_unknown_();
  }

  // ---- Remote/ARTR settings ----
  // Voltages are scaled by the battery bank voltage, which comes from the
  // inverter model, so wait for an inverter frame before publishing them.
  if (remote_pending_ && voltage_multiplier_ != 0) {
    remote_pending_ = false;
    const uint8_t *d = pend_remote_;
    float vm = (float) voltage_multiplier_;
    uint8_t batt_type = d[3];

    // Battery type above 100 is a custom absorb voltage on a 12 V scale. For
    // the preset types the absorb voltage is not on the bus, so leave it unknown.
    float absorb_v = batt_type > 100 ? batt_type / 10.0f * vm : NAN;
    // LBCO is sent as the actual voltage (pymagnum, and 200 = 20.0 V on a 24 V
    // MS4024PAE). A byte cannot hold 48 V LBCO values, so assume a 24 V scale
    // there. That case is unverified.
    float lbco_v = d[9] / 10.0f * (voltage_multiplier_ == 4 ? 2.0f : 1.0f);

    if (remote_searchwatts_) remote_searchwatts_->publish_state(d[1]);
    if (remote_battery_size_) remote_battery_size_->publish_state(d[2] * 10.0f);
    if (remote_chargeramps_) remote_chargeramps_->publish_state(d[4]);
    if (remote_shore_amps_) remote_shore_amps_->publish_state(d[5]);
    if (remote_lbco_) remote_lbco_->publish_state(round1(lbco_v));
    if (remote_vac_cutout_) remote_vac_cutout_->publish_state(d[10]);
    if (remote_float_) remote_float_->publish_state(round1(d[11] / 10.0f * vm));
    if (remote_absorb_) remote_absorb_->publish_state(std::isnan(absorb_v) ? NAN : round1(absorb_v));
    if (remote_eq_) remote_eq_->publish_state(std::isnan(absorb_v) ? NAN : round1(absorb_v + d[12] / 10.0f * vm));
    if (remote_absorb_time_) remote_absorb_time_->publish_state(round1(d[13] / 10.0f));
    if (remote_battery_type_) remote_battery_type_->publish_state(battery_type_to_str_(batt_type));

    if (startup_logging_()) {
      ESP_LOGI(TAG, "REMOTE[0x%02X]: search=%uW batt=%uAh type=%u chg=%u%% shore=%uA lbco=%.1fV float=%.1fV "
                    "absorb=%.1fV abstime=%.1fh vac=%u vm=%u",
               d[20], d[1], d[2] * 10u, batt_type, d[4], d[5], lbco_v, d[11] / 10.0f * vm, absorb_v,
               d[13] / 10.0f, d[10], voltage_multiplier_);
    }
  }

  // ---- RTR ----
  if (rtr_revision_ && std::isfinite(rtr_revision_val_)) rtr_revision_->publish_state(rtr_revision_val_);
  if (rtr_model_text_ && std::isfinite(rtr_revision_val_)) {
    char rtr_str[32];
    snprintf(rtr_str, sizeof(rtr_str), "ME-ARTR v%.1f", rtr_revision_val_);
    rtr_model_text_->publish_state(rtr_str);
  }

  bool connected = last_any_frame_ms_ != 0 && (now - last_any_frame_ms_) < stale_timeout_ms_;
  if (connected_bs_) connected_bs_->publish_state(connected);

  // Counters change on every publish, so send them less often to keep the
  // Home Assistant database small.
  if ((now - last_diag_publish_ms_) >= DIAG_PUBLISH_INTERVAL_MS) {
    last_diag_publish_ms_ = now;
    if (inv_frames_)    inv_frames_->publish_state((float) inv_frame_count_);
    if (bmk_frames_)    bmk_frames_->publish_state((float) bmk_frame_count_);
    if (remote_frames_) remote_frames_->publish_state((float) remote_frame_count_);
    if (rejected_)      rejected_->publish_state((float) scanner_.rejected_bytes());
    if (rtr_frames_)    rtr_frames_->publish_state((float) rtr_frame_count_);
    if (last_frame_age_) {
      if (last_any_frame_ms_ == 0) last_frame_age_->publish_state(NAN);
      else last_frame_age_->publish_state((now - last_any_frame_ms_) / 1000.0f);
    }
  }
}

}  // namespace magnum_inverter
}  // namespace esphome
