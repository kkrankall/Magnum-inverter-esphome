#pragma once

// Frame recognition for the Magnum RS485 network.
//
// This header has no ESPHome dependencies so the scanner can be unit tested on
// a desktop machine (see tests/test_frame_scanner.cpp).
//
// Packet layouts, per the Magnum networking protocol notes and pymagnum:
//   INVERTER 21 bytes. Some inverters send a 22nd byte that carries no data.
//   REMOTE   21 bytes. The last byte is a footer naming the accessory polled.
//   BMK      18 bytes, starts with 0x81.
//   RTR       2 bytes, starts with 0x91.
//
// The ESP32 UART hands bytes over in chunks (every 18 bytes at 19200 baud, or
// when the line goes idle), so a frame can be split across reads. The scanner
// keeps an incomplete frame until the rest arrives instead of discarding it.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace esphome {
namespace magnum_inverter {

enum class FrameType : uint8_t { INVERTER, REMOTE, BMK, RTR };

static constexpr size_t INVERTER_LEN = 21;
static constexpr size_t REMOTE_LEN = 21;
static constexpr size_t BMK_LEN = 18;
static constexpr size_t RTR_LEN = 2;

inline uint16_t be_u16(const uint8_t *b, size_t o) { return (uint16_t(b[o]) << 8) | uint16_t(b[o + 1]); }
inline int16_t be_i16(const uint8_t *b, size_t o) { return (int16_t) be_u16(b, o); }

inline bool is_valid_mode(uint8_t m) {
  switch (m) {
    case 0x00: case 0x01: case 0x02: case 0x04: case 0x08: case 0x09:
    case 0x10: case 0x20: case 0x40: case 0x50: case 0x80:
      return true;
    default:
      return false;
  }
}

inline bool is_known_model(uint8_t m) {
  switch (m) {
    case 0x06: case 0x07: case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0F:
    case 0x14: case 0x15: case 0x19: case 0x1E: case 0x23: case 0x24: case 0x28: case 0x2C:
    case 0x2D: case 0x2F: case 0x35: case 0x36: case 0x37: case 0x3B: case 0x3F: case 0x45:
    case 0x4A: case 0x5A: case 0x5B: case 0x67: case 0x68: case 0x69: case 0x6A: case 0x6B:
    case 0x6F: case 0x70: case 0x72: case 0x73: case 0x74: case 0x75:
      return true;
    default:
      return false;
  }
}

inline bool is_remote_footer(uint8_t b) {
  return b == 0x00 || b == 0x11 || b == 0x80 || (b >= 0xA0 && b <= 0xA4) || (b >= 0xC0 && b <= 0xC3) ||
         b == 0xD0;
}

// The *_prefix_ok checks read only the first n bytes, so they also tell
// whether a partly received frame could still turn out valid.

inline bool inverter_prefix_ok(const uint8_t *d, size_t n) {
  if (n >= 1 && !is_valid_mode(d[0]))
    return false;
  if (n >= 4) {
    uint16_t v10 = be_u16(d, 2);  // 8.0 to 80.0 V DC
    if (v10 < 80 || v10 > 800)
      return false;
  }
  if (n >= 15 && !is_known_model(d[14]))
    return false;
  return true;
}

inline bool remote_prefix_ok(const uint8_t *d, size_t n) {
  if (n >= 1 && d[0] > 0x07)  // command bits (toggle inverter / charger)
    return false;
  if (n >= 5 && d[4] > 100)  // charger rate, percent
    return false;
  if (n >= 6 && d[5] > 100)  // shore amps
    return false;
  if (n >= REMOTE_LEN && !is_remote_footer(d[20]))
    return false;
  return true;
}

// Tighter remote check, used only to learn whether the inverter sends a 22nd
// byte. A remote read one byte off puts the battery size where the battery
// type belongs, which this rejects.
inline bool remote_strict(const uint8_t *d) {
  uint8_t type = d[3];
  bool type_ok = type == 2 || type == 4 || type == 8 || type == 10 || type > 100;
  return type_ok && d[6] >= 10 && remote_prefix_ok(d, REMOTE_LEN);  // d[6]: remote revision, 1.0 or later
}

inline bool bmk_prefix_ok(const uint8_t *d, size_t n) {
  if (n >= 1 && d[0] != 0x81)
    return false;
  if (n >= 2 && d[1] > 100)  // state of charge
    return false;
  if (n >= 4) {
    uint16_t v100 = be_u16(d, 2);  // 5.00 to 70.00 V DC
    if (v100 < 500 || v100 > 7000)
      return false;
  }
  return true;
}

inline bool rtr_prefix_ok(const uint8_t *d, size_t n) {
  if (n >= 1 && d[0] != 0x91)
    return false;
  if (n >= 2 && (d[1] < 10 || d[1] > 99))  // router revision 1.0 to 9.9
    return false;
  return true;
}

class FrameScanner {
 public:
  // Appends received bytes and passes each complete frame to
  // on_frame(FrameType, const uint8_t *data, size_t len).
  template<typename F> void feed(const uint8_t *data, size_t len, F &&on_frame) {
    this->buf_.insert(this->buf_.end(), data, data + len);
    if (this->buf_.size() > MAX_BUFFER) {
      size_t drop = this->buf_.size() - MAX_BUFFER;
      this->buf_.erase(this->buf_.begin(), this->buf_.begin() + drop);
      this->rejected_ += drop;
      this->after_inverter_ = false;
    }
    this->scan_(false, on_frame);
  }

  // Call when the bus has gone quiet: a partial frame left in the buffer will
  // not be completed, so drop it.
  template<typename F> void flush(F &&on_frame) { this->scan_(true, on_frame); }

  size_t pending() const { return this->buf_.size(); }
  uint32_t rejected_bytes() const { return this->rejected_; }
  bool inverter_sends_extra_byte() const { return this->pad_score_ > 0; }

 protected:
  static constexpr size_t MAX_BUFFER = 512;
  static constexpr int8_t PAD_SCORE_LIMIT = 8;

  enum class Match : uint8_t { NONE, NEED_MORE, FRAME };

  template<typename F> void scan_(bool flush, F &&on_frame) {
    size_t pos = 0;
    while (pos < this->buf_.size()) {
      const uint8_t *p = this->buf_.data() + pos;
      size_t rem = this->buf_.size() - pos;

      if (this->after_inverter_) {
        // Compare the two possible alignments of the remote frame before deciding.
        if (rem < REMOTE_LEN + 1 && !flush)
          break;
        this->after_inverter_ = false;
        if (this->skip_extra_byte_(p, rem)) {
          pos++;
          continue;
        }
      }

      FrameType type;
      size_t len = 0;
      Match m = match_(p, rem, type, len);
      if (m == Match::NEED_MORE && !flush)
        break;
      if (m == Match::FRAME) {
        on_frame(type, p, len);
        pos += len;
        this->after_inverter_ = type == FrameType::INVERTER;
        continue;
      }
      pos++;
      this->rejected_++;
    }
    if (pos > 0)
      this->buf_.erase(this->buf_.begin(), this->buf_.begin() + pos);
  }

  // Called on the byte after an inverter frame. Returns true when that byte is
  // the inverter's 22nd byte rather than the start of the next frame.
  bool skip_extra_byte_(const uint8_t *p, size_t rem) {
    if (rem >= REMOTE_LEN + 1) {
      bool at0 = remote_strict(p);
      bool at1 = remote_strict(p + 1);
      if (at1 && !at0) {
        if (this->pad_score_ < PAD_SCORE_LIMIT)
          this->pad_score_++;
        return true;
      }
      if (at0 && !at1) {
        if (this->pad_score_ > -PAD_SCORE_LIMIT)
          this->pad_score_--;
        return false;
      }
    }
    // Ambiguous: go with what this inverter has done before, unless another
    // frame plainly starts here.
    if (this->pad_score_ <= 0)
      return false;
    FrameType type;
    size_t len = 0;
    return !(match_(p, rem, type, len) == Match::FRAME && type != FrameType::REMOTE);
  }

  static Match match_(const uint8_t *p, size_t rem, FrameType &type, size_t &len) {
    bool need_more = false;

    if (rem >= INVERTER_LEN) {
      if (inverter_prefix_ok(p, INVERTER_LEN)) {
        type = FrameType::INVERTER;
        len = INVERTER_LEN;
        return Match::FRAME;
      }
    } else if (inverter_prefix_ok(p, rem)) {
      need_more = true;
    }

    if (rem >= BMK_LEN) {
      if (bmk_prefix_ok(p, BMK_LEN)) {
        type = FrameType::BMK;
        len = BMK_LEN;
        return Match::FRAME;
      }
    } else if (bmk_prefix_ok(p, rem)) {
      need_more = true;
    }

    if (rem >= RTR_LEN) {
      if (rtr_prefix_ok(p, RTR_LEN)) {
        type = FrameType::RTR;
        len = RTR_LEN;
        return Match::FRAME;
      }
    } else if (rtr_prefix_ok(p, rem)) {
      need_more = true;
    }

    if (rem >= REMOTE_LEN) {
      if (remote_prefix_ok(p, REMOTE_LEN)) {
        type = FrameType::REMOTE;
        len = REMOTE_LEN;
        return Match::FRAME;
      }
    } else if (remote_prefix_ok(p, rem)) {
      need_more = true;
    }

    return need_more ? Match::NEED_MORE : Match::NONE;
  }

  std::vector<uint8_t> buf_;
  uint32_t rejected_{0};
  bool after_inverter_{false};
  int8_t pad_score_{0};
};

}  // namespace magnum_inverter
}  // namespace esphome
