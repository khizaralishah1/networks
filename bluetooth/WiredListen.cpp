// wiredlisten.cpp
// Capture audio from the microphone of wired earphones on macOS (via CoreAudio,
// through the miniaudio single-header library). Shows a live level meter and can
// optionally record to a WAV file so you can listen back to it.
//
// Build (macOS):
//   curl -O https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h
//   clang++ -std=c++17 -O2 wiredlisten.cpp -o wiredlisten -framework CoreAudio -framework
//   AudioToolbox -framework CoreFoundation
//
// Usage:
//   ./wiredlisten                        # use the headphone-jack mic ("External Microphone")
//   ./wiredlisten --list                 # list input devices
//   ./wiredlisten "EarPods"              # first input whose name contains "EarPods"
//   (USB-C/Lightning earphones)
//   ./wiredlisten default                # the system default input
//   ./wiredlisten "External" out.wav     # also record to out.wav (stop with Ctrl+C)

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

static std::atomic<float> g_peak{0.0f};
static std::atomic<bool> g_stop{false};
static std::atomic<bool> g_recording{false};
static std::atomic<unsigned> g_dropped{0};
static ma_pcm_rb g_ring;
static ma_uint32 g_bytesPerFrame = 0;

static void on_sigint(int) { g_stop = true; }

// Runs on CoreAudio's audio thread: no printing, no locks, no file I/O.
static void data_callback(ma_device* device, void* /*output*/, const void* input,
                          ma_uint32 frameCount) {
  const float* samples = static_cast<const float*>(input);
  const ma_uint32 count = frameCount * device->capture.channels;

  // ---- This is where you process the audio ----
  // `samples` holds `count` interleaved float samples in [-1.0, 1.0].
  float peak = 0.0f;
  for (ma_uint32 i = 0; i < count; ++i) {
    peak = std::max(peak, std::fabs(samples[i]));
  }
  float prev = g_peak.load(std::memory_order_relaxed);
  while (peak > prev && !g_peak.compare_exchange_weak(prev, peak)) {
  }

  // Hand samples to the main thread through a lock-free ring buffer for recording.
  if (g_recording.load(std::memory_order_relaxed)) {
    const ma_uint8* src = static_cast<const ma_uint8*>(input);
    ma_uint32 remaining = frameCount;
    while (remaining > 0) {
      ma_uint32 n = remaining;
      void* dst = nullptr;
      if (ma_pcm_rb_acquire_write(&g_ring, &n, &dst) != MA_SUCCESS || n == 0) {
        g_dropped.fetch_add(remaining);  // ring is full; main thread is behind
        break;
      }
      std::memcpy(dst, src, static_cast<size_t>(n) * g_bytesPerFrame);
      ma_pcm_rb_commit_write(&g_ring, n);
      src += static_cast<size_t>(n) * g_bytesPerFrame;
      remaining -= n;
    }
  }
}

static bool contains_ci(const std::string& haystack, const std::string& needle) {
  auto lower = [](std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
  };
  return lower(haystack).find(lower(needle)) != std::string::npos;
}

int main(int argc, char** argv) {
  ma_context context;
  if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
    std::fprintf(stderr, "Failed to initialize audio context.\n");
    return 1;
  }

  ma_device_info* playbackInfos;
  ma_uint32 playbackCount;
  ma_device_info* captureInfos;
  ma_uint32 captureCount;
  if (ma_context_get_devices(&context, &playbackInfos, &playbackCount, &captureInfos,
                             &captureCount) != MA_SUCCESS) {
    std::fprintf(stderr, "Failed to enumerate devices.\n");
    ma_context_uninit(&context);
    return 1;
  }

  auto list_devices = [&]() {
    std::printf("Input devices:\n");
    for (ma_uint32 i = 0; i < captureCount; ++i) {
      std::printf("  [%u] %s%s\n", i, captureInfos[i].name,
                  captureInfos[i].isDefault ? "  (default)" : "");
    }
  };

  const std::string wanted = (argc >= 2) ? argv[1] : "External Microphone";
  const std::string wavPath = (argc >= 3) ? argv[2] : "";

  if (wanted == "--list") {
    list_devices();
    ma_context_uninit(&context);
    return 0;
  }

  const ma_device_info* chosen = nullptr;
  for (ma_uint32 i = 0; i < captureCount && !chosen; ++i) {
    if (wanted == "default" ? captureInfos[i].isDefault
                            : contains_ci(captureInfos[i].name, wanted)) {
      chosen = &captureInfos[i];
    }
  }
  if (!chosen) {
    std::fprintf(stderr, "No input device matching \"%s\".\n\n", wanted.c_str());
    list_devices();
    std::fprintf(stderr,
                 "\nIs the headset plugged in? Try: ./wiredlisten \"<part of a name above>\"\n");
    ma_context_uninit(&context);
    return 1;
  }

  ma_device_config config = ma_device_config_init(ma_device_type_capture);
  config.capture.pDeviceID = &chosen->id;
  config.capture.format = ma_format_f32;
  config.capture.channels = 0;  // 0 = device's native channel count (usually 1 for a headset mic)
  config.sampleRate = 0;        // 0 = device's native rate (typically 44.1 or 48 kHz when wired)
  config.dataCallback = data_callback;

  ma_device device;
  if (ma_device_init(&context, &config, &device) != MA_SUCCESS) {
    std::fprintf(stderr,
                 "Failed to open device. Check microphone permission for your terminal app.\n");
    ma_context_uninit(&context);
    return 1;
  }

  std::printf("Capturing from: %s\n", device.capture.name);
  std::printf("Sample rate: %u Hz, channels: %u\n", device.sampleRate, device.capture.channels);

  // Optional WAV recording.
  ma_encoder encoder;
  bool encoderOpen = false;
  if (!wavPath.empty()) {
    g_bytesPerFrame = ma_get_bytes_per_frame(device.capture.format, device.capture.channels);
    // ~2 seconds of headroom between the audio thread and the file writer.
    if (ma_pcm_rb_init(device.capture.format, device.capture.channels, device.sampleRate * 2,
                       nullptr, nullptr, &g_ring) != MA_SUCCESS) {
      std::fprintf(stderr, "Failed to create ring buffer.\n");
      ma_device_uninit(&device);
      ma_context_uninit(&context);
      return 1;
    }
    ma_encoder_config encConfig = ma_encoder_config_init(
        ma_encoding_format_wav, device.capture.format, device.capture.channels, device.sampleRate);
    if (ma_encoder_init_file(wavPath.c_str(), &encConfig, &encoder) != MA_SUCCESS) {
      std::fprintf(stderr, "Failed to open %s for writing.\n", wavPath.c_str());
      ma_pcm_rb_uninit(&g_ring);
      ma_device_uninit(&device);
      ma_context_uninit(&context);
      return 1;
    }
    encoderOpen = true;
    g_recording = true;
    std::printf("Recording to: %s\n", wavPath.c_str());
  }

  std::signal(SIGINT, on_sigint);
  std::printf("Press Ctrl+C to stop.\n\n");

  if (ma_device_start(&device) != MA_SUCCESS) {
    std::fprintf(stderr, "Failed to start device.\n");
    if (encoderOpen) {
      ma_encoder_uninit(&encoder);
      ma_pcm_rb_uninit(&g_ring);
    }
    ma_device_uninit(&device);
    ma_context_uninit(&context);
    return 1;
  }

  auto drain_ring = [&]() {
    if (!encoderOpen) return;
    for (;;) {
      ma_uint32 n = 0xFFFFFFFF;
      void* src = nullptr;
      if (ma_pcm_rb_acquire_read(&g_ring, &n, &src) != MA_SUCCESS || n == 0) break;
      ma_uint64 written = 0;
      ma_encoder_write_pcm_frames(&encoder, src, n, &written);
      ma_pcm_rb_commit_read(&g_ring, n);
    }
  };

  while (!g_stop) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    drain_ring();
    const float peak = g_peak.exchange(0.0f);
    const int bars = static_cast<int>(std::min(1.0f, peak) * 50.0f);
    std::printf("\r[%-50s] %.3f", std::string(bars, '#').c_str(), peak);
    std::fflush(stdout);
  }

  std::printf("\n\nStopping...\n");
  ma_device_stop(&device);
  g_recording = false;
  if (encoderOpen) {
    drain_ring();
    ma_encoder_uninit(&encoder);  // finalizes the WAV header
    ma_pcm_rb_uninit(&g_ring);
    std::printf("Saved %s", wavPath.c_str());
    if (g_dropped > 0) std::printf(" (warning: dropped %u frames)", g_dropped.load());
    std::printf("\n");
  }

  ma_device_uninit(&device);
  ma_context_uninit(&context);
  return 0;
}