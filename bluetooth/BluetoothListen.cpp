// btlisten.cpp
// Capture audio from a Bluetooth earbuds microphone on macOS (via CoreAudio,
// through the miniaudio single-header library) and show a live level meter.
//
// Build (macOS):
//   curl -O https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h
//   clang++ -std=c++17 -O2 btlisten.cpp -o btlisten -framework CoreAudio -framework AudioToolbox -framework CoreFoundation
//
// Usage:
//   ./btlisten                 # list input devices
//   ./btlisten "AirPods"       # capture from the first input whose name contains "AirPods"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

static std::atomic<float> g_peak{0.0f};

// Called by CoreAudio on its own audio thread. Keep it fast: no printing, no locks.
static void data_callback(ma_device* device, void* /*output*/, const void* input, ma_uint32 frameCount)
{
    const float* samples = static_cast<const float*>(input);
    const ma_uint32 count = frameCount * device->capture.channels;

    // ---- This is where you process the audio ----
    // `samples` holds `count` interleaved float samples in [-1.0, 1.0].
    // Copy them into a ring buffer, run an FFT, write a WAV, stream to a
    // speech recognizer, etc.
    float peak = 0.0f;
    for (ma_uint32 i = 0; i < count; ++i) {
        peak = std::max(peak, std::fabs(samples[i]));
    }

    // Keep the max since the main thread last read it.
    float prev = g_peak.load(std::memory_order_relaxed);
    while (peak > prev && !g_peak.compare_exchange_weak(prev, peak)) {}
}

static bool contains_ci(const std::string& haystack, const std::string& needle)
{
    auto lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    };
    return lower(haystack).find(lower(needle)) != std::string::npos;
}

int main(int argc, char** argv)
{
    ma_context context;
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
        std::fprintf(stderr, "Failed to initialize audio context.\n");
        return 1;
    }

    ma_device_info* playbackInfos;
    ma_uint32 playbackCount;
    ma_device_info* captureInfos;
    ma_uint32 captureCount;
    if (ma_context_get_devices(&context, &playbackInfos, &playbackCount, &captureInfos, &captureCount) != MA_SUCCESS) {
        std::fprintf(stderr, "Failed to enumerate devices.\n");
        ma_context_uninit(&context);
        return 1;
    }

    if (argc < 2) {
        std::printf("Input devices:\n");
        for (ma_uint32 i = 0; i < captureCount; ++i) {
            std::printf("  [%u] %s%s\n", i, captureInfos[i].name, captureInfos[i].isDefault ? "  (default)" : "");
        }
        std::printf("\nRun again with part of a device name, e.g.  ./btlisten \"AirPods\"\n");
        ma_context_uninit(&context);
        return 0;
    }

    const std::string wanted = argv[1];
    const ma_device_info* chosen = nullptr;
    for (ma_uint32 i = 0; i < captureCount; ++i) {
        if (contains_ci(captureInfos[i].name, wanted)) {
            chosen = &captureInfos[i];
            break;
        }
    }
    if (!chosen) {
        std::fprintf(stderr, "No input device matching \"%s\". Run with no arguments to list them.\n", wanted.c_str());
        ma_context_uninit(&context);
        return 1;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.pDeviceID = &chosen->id;
    config.capture.format    = ma_format_f32;
    config.capture.channels  = 1;   // Bluetooth headset mics are mono
    config.sampleRate        = 0;   // 0 = use the device's native rate (typically 16 kHz or 8 kHz over HFP)
    config.dataCallback      = data_callback;

    ma_device device;
    if (ma_device_init(&context, &config, &device) != MA_SUCCESS) {
        std::fprintf(stderr, "Failed to open device. Check microphone permission for your terminal app.\n");
        ma_context_uninit(&context);
        return 1;
    }

    std::printf("Capturing from: %s\n", device.capture.name);
    std::printf("Sample rate: %u Hz, channels: %u\n", device.sampleRate, device.capture.channels);
    std::printf("Press Ctrl+C to stop.\n\n");

    if (ma_device_start(&device) != MA_SUCCESS) {
        std::fprintf(stderr, "Failed to start device.\n");
        ma_device_uninit(&device);
        ma_context_uninit(&context);
        return 1;
    }

    // Simple level meter so you can tell it's working.
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const float peak = g_peak.exchange(0.0f);
        const int bars = static_cast<int>(std::min(1.0f, peak) * 50.0f);
        std::printf("\r[%-50s] %.3f", std::string(bars, '#').c_str(), peak);
        std::fflush(stdout);
    }

    ma_device_uninit(&device);
    ma_context_uninit(&context);
    return 0;
}