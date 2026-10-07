// Desktop tests for the Magnum frame scanner.
//
//   c++ -std=c++17 -Wall -Wextra -I esphome/components/magnum_inverter \
//       tests/test_frame_scanner.cpp -o /tmp/test_frame_scanner && /tmp/test_frame_scanner
//
// Sample packets come from pymagnum's testdata/allpackets.txt (an MS4024PAE
// with an ME-ARTR). The bus is simulated byte by byte at 19200 baud, with the
// ESP32 UART handing bytes over every 18 bytes or when the line goes idle, and
// the ESPHome loop reading whatever has arrived at a fixed interval.

#include "magnum_frame.h"

#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

using namespace esphome::magnum_inverter;

using Bytes = std::vector<uint8_t>;

static Bytes hex(const char *s) {
  Bytes out;
  for (size_t i = 0; s[i] && s[i + 1]; i += 2)
    out.push_back((uint8_t) std::stoul(std::string(s + i, 2), nullptr, 16));
  return out;
}

static const Bytes INV = hex("400000F60016770001003D1133246B010005025800");
static const Bytes REMOTE_80 = hex("00002808640A280000C89B840C1412200000280080");
static const Bytes REMOTE_00 = hex("00002808640A280000C89B840C1412200000000000");
static const Bytes REMOTE_A0 = hex("00002808640A280000C89B840C14122014007300A0");
// Same as REMOTE_80 but with the settings seen on this repo owner's ME-ARTR:
// custom battery type (120), 90% charger, 5 A shore, LBCO 21.0 V.
static const Bytes REMOTE_CUSTOM = hex("00001E785A052800" "00D2AF78009512200000280080");
static const Bytes BMK = hex("814C09F1007407E00C08FF984FF000140A01");
static const Bytes RTR = hex("9128");

struct Packet {
  Bytes bytes;
  double gap_before_ms;
};

struct Event {
  double t_ms;
  uint8_t b;
};

// Turns packets into a timed byte stream.
static std::vector<Event> to_wire(const std::vector<Packet> &packets) {
  const double byte_ms = 10.0 / 19.2;  // 10 bits per byte at 19200 baud
  std::vector<Event> ev;
  double t = 0;
  for (auto &p : packets) {
    t += p.gap_before_ms;
    for (uint8_t b : p.bytes) {
      t += byte_ms;
      ev.push_back({t, b});
    }
  }
  return ev;
}

// Delivers bytes the way the ESP32 UART driver does (rx_full_threshold=18,
// rx_timeout=2 symbols), and reads them every loop_ms like the ESPHome loop.
template<typename Reader> static void replay(const std::vector<Event> &ev, double loop_ms, Reader &&read) {
  const double timeout_ms = 2 * 10.0 / 19.2;
  std::vector<double> ready(ev.size());
  size_t undelivered = 0;
  for (size_t i = 0; i < ev.size(); i++) {
    bool burst_ends = i + 1 == ev.size() || ev[i + 1].t_ms - ev[i].t_ms > timeout_ms;
    bool fifo_full = i + 1 - undelivered >= 18;
    if (fifo_full || burst_ends) {
      double when = fifo_full ? ev[i].t_ms : ev[i].t_ms + timeout_ms;
      for (size_t j = undelivered; j <= i; j++)
        ready[j] = when;
      undelivered = i + 1;
    }
  }
  size_t next = 0;
  double last_rx = 0;
  for (double now = 0; next < ev.size() || now < ev.back().t_ms + 100; now += loop_ms) {
    Bytes chunk;
    while (next < ev.size() && ready[next] <= now)
      chunk.push_back(ev[next++].b);
    if (!chunk.empty())
      last_rx = now;
    read(chunk, now - last_rx);
  }
}

struct Counts {
  int inverter = 0, remote = 0, bmk = 0, rtr = 0, bad = 0;
  uint32_t rejected = 0;
};

// Counts frames, and counts as bad any frame whose bytes do not match a packet
// that was actually sent.
struct Checker {
  std::vector<Bytes> sent;
  Counts c;
  void operator()(FrameType t, const uint8_t *d, size_t len) {
    bool real = false;
    for (auto &s : sent)
      real |= s.size() >= len && memcmp(s.data(), d, len) == 0;
    if (!real)
      c.bad++;
    switch (t) {
      case FrameType::INVERTER: c.inverter++; break;
      case FrameType::REMOTE: c.remote++; break;
      case FrameType::BMK: c.bmk++; break;
      case FrameType::RTR: c.rtr++; break;
    }
  }
};

// Port of the scan loop in the previous magnum_inverter.cpp, for comparison.
// It needs 22 bytes for an inverter frame and drops whatever is left at the
// end of the buffer.
static void old_scan(Bytes &buf, Counts &c) {
  auto old_remote_subtype = [](uint8_t b) {
    return b == 0xA0 || b == 0xA1 || b == 0xA2 || b == 0xA3 || b == 0xA4 || b == 0x80 || b == 0x11 ||
           b == 0xC0 || b == 0xC1 || b == 0xC2 || b == 0xC3 || b == 0xD0;
  };
  size_t pos = 0;
  while (pos < buf.size()) {
    size_t rem = buf.size() - pos;
    const uint8_t *p = &buf[pos];
    if (rem >= 2 && p[0] == 0x91) { c.rtr++; pos += 2; continue; }
    if (rem >= 18 && p[0] == 0x81) {
      uint16_t v = be_u16(p, 2);
      if (v >= 500 && v <= 7000) { c.bmk++; pos += 18; continue; }
    }
    if (rem >= 22 && is_valid_mode(p[0]) && is_known_model(p[14])) {
      uint16_t v10 = be_u16(p, 2);
      if (v10 >= 80 && v10 <= 800) { c.inverter++; pos += 22; continue; }
    }
    if (rem >= 21) {
      if (old_remote_subtype(p[20])) { c.remote++; pos += 21; continue; }
      if (p[20] == 0x00) {
        bool zeros = true;
        for (int k = 14; k <= 20; k++) zeros &= p[k] == 0;
        if (zeros) { c.remote++; pos += 21; continue; }
      }
    }
    pos++;
    c.rejected++;
  }
  buf.clear();
}

static std::vector<Packet> make_bus(int cycles, bool extra_inverter_byte, const Bytes &remote, bool with_rtr,
                                    std::mt19937 &rng, int garbage_every = 0) {
  std::vector<Packet> out;
  std::uniform_real_distribution<double> jitter(-1.5, 1.5);
  std::uniform_int_distribution<int> byte(0, 255);
  for (int i = 0; i < cycles; i++) {
    Bytes inv = INV;
    if (extra_inverter_byte)
      inv.push_back(0x00);
    out.push_back({inv, i == 0 ? 0 : 40 + jitter(rng)});
    out.push_back({remote, 10 + jitter(rng)});
    if (with_rtr && i % 4 == 0)
      out.push_back({RTR, 5 + jitter(rng)});
    else
      out.push_back({BMK, 5 + jitter(rng)});
    if (garbage_every && i % garbage_every == 0) {
      Bytes junk(7);
      for (auto &b : junk) b = (uint8_t) byte(rng);
      out.push_back({junk, 3});
    }
  }
  return out;
}

static int failures = 0;

static void expect(bool ok, const char *what) {
  printf("  %s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok)
    failures++;
}

struct Result {
  Counts now, old;
};

static Result run(const std::vector<Packet> &bus, double loop_ms, const std::vector<Bytes> &sent) {
  auto ev = to_wire(bus);
  Result r;

  FrameScanner scanner;
  Checker check{sent, {}};
  replay(ev, loop_ms, [&](const Bytes &chunk, double idle_ms) {
    if (!chunk.empty())
      scanner.feed(chunk.data(), chunk.size(), check);
    else if (idle_ms > 30 && scanner.pending() > 0)
      scanner.flush(check);
  });
  r.now = check.c;
  r.now.rejected = scanner.rejected_bytes();

  Bytes buf;
  replay(ev, loop_ms, [&](const Bytes &chunk, double) {
    buf.insert(buf.end(), chunk.begin(), chunk.end());
    if (buf.size() >= 2)
      old_scan(buf, r.old);
  });
  return r;
}

static void report(const char *name, const Result &r, int cycles) {
  printf("%s\n", name);
  printf("  new: inverter %d/%d  remote %d  bmk %d  rtr %d  rejected bytes %u  wrong frames %d\n",
         r.now.inverter, cycles, r.now.remote, r.now.bmk, r.now.rtr, r.now.rejected, r.now.bad);
  printf("  old: inverter %d/%d  remote %d  bmk %d  rtr %d  rejected bytes %u\n", r.old.inverter, cycles,
         r.old.remote, r.old.bmk, r.old.rtr, r.old.rejected);
}

int main() {
  const int N = 1000;
  std::vector<Bytes> sent = {INV, REMOTE_80, REMOTE_00, REMOTE_A0, REMOTE_CUSTOM, BMK, RTR};

  for (double loop_ms : {16.0, 1.0}) {
    printf("\n=== ESPHome loop every %.0f ms ===\n", loop_ms);
    for (bool extra : {true, false}) {
      std::mt19937 rng(42);
      char name[96];
      snprintf(name, sizeof(name), "%s-byte inverter frames, REMOTE_80, BMK + RTR", extra ? "22" : "21");
      auto r = run(make_bus(N, extra, REMOTE_80, true, rng), loop_ms, sent);
      report(name, r, N);
      expect(r.now.inverter == N, "every inverter frame parsed");
      expect(r.now.remote == N, "every remote frame parsed");
      expect(r.now.bmk + r.now.rtr == N, "every BMK/RTR frame parsed");
      expect(r.now.bad == 0 && r.now.rejected == 0, "no wrong frames, no rejected bytes");
    }
    for (bool extra : {true, false}) {
      std::mt19937 rng(7);
      char name[96];
      snprintf(name, sizeof(name), "%s-byte inverter frames, owner's ARTR settings", extra ? "22" : "21");
      auto r = run(make_bus(N, extra, REMOTE_CUSTOM, false, rng), loop_ms, sent);
      report(name, r, N);
      expect(r.now.inverter == N && r.now.remote == N && r.now.bmk == N, "every frame parsed");
      expect(r.now.bad == 0, "no wrong frames");
    }
    for (bool extra : {true, false}) {
      std::mt19937 rng(99);
      char name[96];
      snprintf(name, sizeof(name), "%s-byte inverter frames, REMOTE_00, 7 junk bytes every 5th cycle",
               extra ? "22" : "21");
      auto r = run(make_bus(N, extra, REMOTE_00, true, rng, 5), loop_ms, sent);
      report(name, r, N);
      expect(r.now.inverter >= N * 98 / 100, "at least 98% of inverter frames parsed");
      expect(r.now.bad <= N / 200, "wrong frames under 0.5%");
    }
  }

  printf("\n%s\n", failures ? "SOME TESTS FAILED" : "all tests passed");
  return failures ? 1 : 0;
}
