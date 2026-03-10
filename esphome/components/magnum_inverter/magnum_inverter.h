#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/time/real_time_clock.h"
#include <vector>
#include <cmath>

namespace magnum_inverter {

class MagnumInverter : public esphome::Component, public esphome::uart::UARTDevice {
 public:
  explicit MagnumInverter(esphome::uart::UARTComponent *parent) : UARTDevice(parent) {}

  // ---- Inverter sensors ----
  void set_battery_voltage_sensor(esphome::sensor::Sensor *s) { batt_v_ = s; }
  void set_dc_amps_sensor(esphome::sensor::Sensor *s) { dc_a_ = s; }
  void set_ac_out_voltage_sensor(esphome::sensor::Sensor *s) { ac_out_v_ = s; }
  void set_ac_in_voltage_sensor(esphome::sensor::Sensor *s) { ac_in_v_ = s; }
  void set_frequency_sensor(esphome::sensor::Sensor *s) { hz_ = s; }
  void set_battery_temp_sensor(esphome::sensor::Sensor *s) { batt_temp_ = s; }
  void set_fet_temp_sensor(esphome::sensor::Sensor *s) { fet_temp_ = s; }
  void set_xfmr_temp_sensor(esphome::sensor::Sensor *s) { xfmr_temp_ = s; }
  void set_mode_text_sensor(esphome::text_sensor::TextSensor *s) { mode_ = s; }
  void set_inverter_fault_code_sensor(esphome::sensor::Sensor *s) { inv_fault_code_ = s; }
  void set_inverter_fault_text_sensor(esphome::text_sensor::TextSensor *s) { inv_fault_text_ = s; }
  void set_inverter_fault_active_sensor(esphome::sensor::Sensor *s) { inv_fault_active_ = s; }
  void set_inverter_model_text_sensor(esphome::text_sensor::TextSensor *s) { inv_model_text_ = s; }
  void set_inverter_stackmode_text_sensor(esphome::text_sensor::TextSensor *s) { inv_stackmode_text_ = s; }
  void set_inverter_revision_sensor(esphome::sensor::Sensor *s) { inv_revision_ = s; }
  void set_inverter_led_text_sensor(esphome::text_sensor::TextSensor *s) { inv_led_ = s; }
  void set_charger_led_text_sensor(esphome::text_sensor::TextSensor *s) { chg_led_ = s; }

  // ---- Power (derived) ----
  void set_ac_out_amps_sensor(esphome::sensor::Sensor *s) { ac_out_a_ = s; }
  void set_ac_out_watts_sensor(esphome::sensor::Sensor *s) { ac_out_w_ = s; }
  void set_ac_in_amps_sensor(esphome::sensor::Sensor *s) { ac_in_a_ = s; }
  void set_battery_watts_sensor(esphome::sensor::Sensor *s) { batt_w_ = s; }

  // ---- BMK sensors ----
  void set_bmk_soc_sensor(esphome::sensor::Sensor *s) { bmk_soc_ = s; }
  void set_bmk_voltage_sensor(esphome::sensor::Sensor *s) { bmk_v_ = s; }
  void set_bmk_amps_sensor(esphome::sensor::Sensor *s) { bmk_a_ = s; }
  void set_bmk_min_voltage_sensor(esphome::sensor::Sensor *s) { bmk_vmin_ = s; }
  void set_bmk_max_voltage_sensor(esphome::sensor::Sensor *s) { bmk_vmax_ = s; }
  void set_bmk_ah_inout_sensor(esphome::sensor::Sensor *s) { bmk_ah_inout_ = s; }
  void set_bmk_ah_trip_sensor(esphome::sensor::Sensor *s) { bmk_ah_trip_ = s; }
  void set_bmk_cumulative_ah_sensor(esphome::sensor::Sensor *s) { bmk_cumah_ = s; }
  void set_bmk_revision_sensor(esphome::sensor::Sensor *s) { bmk_revision_ = s; }
  void set_bmk_fault_text_sensor(esphome::text_sensor::TextSensor *s) { bmk_fault_text_ = s; }
  void set_bmk_watts_sensor(esphome::sensor::Sensor *s) { bmk_w_ = s; }

  // ---- Remote/ARTR sensors (per Magnum protocol spec) ----
  void set_remote_searchwatts_sensor(esphome::sensor::Sensor *s) { remote_searchwatts_ = s; }
  void set_remote_chargeramps_sensor(esphome::sensor::Sensor *s) { remote_chargeramps_ = s; }
  void set_remote_absorb_sensor(esphome::sensor::Sensor *s) { remote_absorb_ = s; }
  void set_remote_float_sensor(esphome::sensor::Sensor *s) { remote_float_ = s; }
  void set_remote_eq_sensor(esphome::sensor::Sensor *s) { remote_eq_ = s; }
  void set_remote_lbco_sensor(esphome::sensor::Sensor *s) { remote_lbco_ = s; }
  void set_remote_battery_size_sensor(esphome::sensor::Sensor *s) { remote_battery_size_ = s; }
  void set_remote_shore_amps_sensor(esphome::sensor::Sensor *s) { remote_shore_amps_ = s; }
  void set_remote_vac_cutout_sensor(esphome::sensor::Sensor *s) { remote_vac_cutout_ = s; }
  void set_remote_absorb_time_sensor(esphome::sensor::Sensor *s) { remote_absorb_time_ = s; }
  void set_remote_battery_type_text_sensor(esphome::text_sensor::TextSensor *s) { remote_battery_type_ = s; }

  // ---- Inverter on/off text sensor ----
  void set_inverter_on_text_sensor(esphome::text_sensor::TextSensor *s) { inv_on_ = s; }

  // ---- RTR sensors ----
  void set_rtr_revision_sensor(esphome::sensor::Sensor *s) { rtr_revision_ = s; }
  void set_rtr_frames_sensor(esphome::sensor::Sensor *s) { rtr_frames_ = s; }
  void set_rtr_model_text_sensor(esphome::text_sensor::TextSensor *s) { rtr_model_text_ = s; }

  // ---- Time ----
  void set_time(esphome::time::RealTimeClock *t) { time_ = t; }

  // ---- Diagnostics ----
  void set_inverter_frames_sensor(esphome::sensor::Sensor *s) { inv_frames_ = s; }
  void set_bmk_frames_sensor(esphome::sensor::Sensor *s) { bmk_frames_ = s; }
  void set_remote_frames_sensor(esphome::sensor::Sensor *s) { remote_frames_ = s; }
  void set_rejected_frames_sensor(esphome::sensor::Sensor *s) { rejected_ = s; }
  void set_last_frame_age_sensor(esphome::sensor::Sensor *s) { last_frame_age_ = s; }

  // ---- Fault history ----
  void set_last_fault_code_sensor(esphome::sensor::Sensor *s) { last_fault_code_ = s; }
  void set_last_fault_text_sensor(esphome::text_sensor::TextSensor *s) { last_fault_text_ = s; }
  void set_last_fault_time_text_sensor(esphome::text_sensor::TextSensor *s) { last_fault_time_ = s; }

  // ---- Packet recording (on-demand raw dump) ----
  void start_packet_recording() { recording_ = true; record_start_ms_ = millis(); record_pkt_count_ = 0; }

  // ---- Inverter toggle (on/off) ----
  void send_inverter_toggle();

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return esphome::setup_priority::DATA; }

 protected:
  std::vector<uint8_t> buf_;

  void scan_buffer_();
  void process_inverter_(const uint8_t *data, size_t len, bool classic);
  void process_bmk_(const uint8_t *data, size_t len);
  void process_remote_(const uint8_t *data, size_t len, uint8_t subtype);
  void maybe_publish_();

  static const char *fault_to_str_(uint8_t f);
  static const char *bmk_fault_to_str_(uint8_t f);
  static const char *mode_to_str_(uint8_t m);
  static const char *model_to_str_(uint8_t m);
  static const char *stackmode_to_str_(uint8_t s);
  static const char *battery_type_to_str_(uint8_t t);

  static inline int16_t be_i16(const uint8_t *b, size_t o) {
    return (int16_t)((uint16_t(b[o]) << 8) | uint16_t(b[o + 1]));
  }
  static inline uint16_t be_u16(const uint8_t *b, size_t o) {
    return (uint16_t(b[o]) << 8) | uint16_t(b[o + 1]);
  }

  // Sensor pointers
  esphome::sensor::Sensor *batt_v_{nullptr}, *dc_a_{nullptr}, *ac_out_v_{nullptr}, *ac_in_v_{nullptr};
  esphome::sensor::Sensor *hz_{nullptr}, *batt_temp_{nullptr}, *fet_temp_{nullptr}, *xfmr_temp_{nullptr};
  esphome::text_sensor::TextSensor *mode_{nullptr};
  esphome::sensor::Sensor *inv_fault_code_{nullptr}, *inv_fault_active_{nullptr};
  esphome::text_sensor::TextSensor *inv_fault_text_{nullptr}, *inv_model_text_{nullptr}, *inv_stackmode_text_{nullptr};
  esphome::sensor::Sensor *ac_out_a_{nullptr}, *ac_out_w_{nullptr}, *ac_in_a_{nullptr}, *batt_w_{nullptr};
  esphome::sensor::Sensor *inv_revision_{nullptr};
  esphome::text_sensor::TextSensor *inv_led_{nullptr}, *chg_led_{nullptr}, *inv_on_{nullptr};
  esphome::sensor::Sensor *bmk_soc_{nullptr}, *bmk_v_{nullptr}, *bmk_a_{nullptr};
  esphome::sensor::Sensor *bmk_vmin_{nullptr}, *bmk_vmax_{nullptr};
  esphome::sensor::Sensor *bmk_ah_inout_{nullptr}, *bmk_ah_trip_{nullptr}, *bmk_cumah_{nullptr};
  esphome::sensor::Sensor *bmk_revision_{nullptr}, *bmk_w_{nullptr};
  esphome::text_sensor::TextSensor *bmk_fault_text_{nullptr};
  esphome::sensor::Sensor *remote_searchwatts_{nullptr}, *remote_chargeramps_{nullptr};
  esphome::sensor::Sensor *remote_absorb_{nullptr}, *remote_float_{nullptr}, *remote_eq_{nullptr}, *remote_lbco_{nullptr};
  esphome::sensor::Sensor *remote_battery_size_{nullptr};
  esphome::sensor::Sensor *remote_shore_amps_{nullptr}, *remote_vac_cutout_{nullptr}, *remote_absorb_time_{nullptr};
  esphome::text_sensor::TextSensor *remote_battery_type_{nullptr};
  esphome::sensor::Sensor *rtr_revision_{nullptr}, *rtr_frames_{nullptr};
  esphome::text_sensor::TextSensor *rtr_model_text_{nullptr};
  esphome::time::RealTimeClock *time_{nullptr};
  esphome::sensor::Sensor *inv_frames_{nullptr}, *bmk_frames_{nullptr}, *remote_frames_{nullptr};
  esphome::sensor::Sensor *rejected_{nullptr}, *last_frame_age_{nullptr};
  esphome::sensor::Sensor *last_fault_code_{nullptr};
  esphome::text_sensor::TextSensor *last_fault_text_{nullptr}, *last_fault_time_{nullptr};

  // State
  uint32_t last_publish_ms_{0};
  uint32_t last_any_frame_ms_{0};
  uint32_t inv_frame_count_{0}, bmk_frame_count_{0}, remote_frame_count_{0}, rejected_count_{0};
  uint8_t voltage_multiplier_{1};
  float cached_batt_v_{NAN}, cached_dc_a_{NAN}, cached_ac_out_v_{NAN};
  float cached_bmk_v_{NAN}, cached_bmk_a_{NAN};
  uint32_t last_bmk_ms_{0};
  uint32_t startup_ms_{0};

  // Returns true if we're in the first 10s after boot AND not recording
  bool startup_logging_() const {
    return !recording_ && (millis() - startup_ms_) < 10000;
  }

  bool inv_pending_{false};
  float pend_batt_v_, pend_dc_a_, pend_ac_out_v_, pend_ac_in_v_;
  float pend_hz_, pend_batt_temp_, pend_fet_temp_, pend_xfmr_temp_;
  float pend_ac_out_a_, pend_ac_in_a_;
  uint8_t pend_mode_, pend_fault_, pend_model_, pend_stackmode_;
  uint8_t pend_inv_led_, pend_chg_led_, pend_inv_revision_;

  bool bmk_pending_{false};
  float pend_bmk_soc_, pend_bmk_v_, pend_bmk_a_;
  float pend_bmk_vmin_, pend_bmk_vmax_;
  float pend_bmk_ah_, pend_bmk_ah_trip_, pend_bmk_cumah_;
  uint8_t pend_bmk_fault_, pend_bmk_revision_;

  uint8_t latched_fault_{0};

  // RTR state
  uint32_t rtr_frame_count_{0};
  float rtr_revision_val_{NAN};

  // Remote/ARTR pending values (per Magnum protocol spec byte order)
  bool remote_pending_{false};
  float pend_remote_searchwatts_, pend_remote_chargeramps_;
  float pend_remote_absorb_, pend_remote_float_, pend_remote_eq_, pend_remote_lbco_;
  float pend_remote_battery_size_;
  float pend_remote_shore_amps_, pend_remote_vac_cutout_, pend_remote_absorb_time_;
  uint8_t pend_remote_battery_type_{0};

  // Debug: log first N raw buffer dumps
  uint8_t dump_count_{0};

  // On-demand packet recording
  bool recording_{false};
  uint32_t record_start_ms_{0};
  uint32_t record_pkt_count_{0};
  static const uint32_t RECORD_DURATION_MS = 15000;

  // Inverter toggle control
  bool toggle_active_{false};
  uint32_t toggle_start_ms_{0};
  uint8_t toggle_frames_sent_{0};
  static const uint8_t TOGGLE_FRAME_COUNT = 5;      // send 5 command frames
  static const uint32_t TOGGLE_TIMEOUT_MS = 5000;    // safety timeout

  // Bus idle tracking (microseconds for precise gap detection)
  uint32_t last_byte_rx_us_{0};

  // Last-seen remote packet template (bytes 0-20)
  uint8_t last_remote_pkt_[21]{};
  bool have_remote_template_{false};
};

// Button that triggers 15-second raw packet recording
class RecordPacketsButton : public esphome::button::Button {
 public:
  void set_parent(MagnumInverter *parent) { parent_ = parent; }

 protected:
  void press_action() override { parent_->start_packet_recording(); }
  MagnumInverter *parent_{nullptr};
};

// Button that toggles inverter on/off by sending command packet
class ToggleInverterButton : public esphome::button::Button {
 public:
  void set_parent(MagnumInverter *parent) { parent_ = parent; }

 protected:
  void press_action() override { parent_->send_inverter_toggle(); }
  MagnumInverter *parent_{nullptr};
};

}  // namespace magnum_inverter