#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

static constexpr std::size_t NUM_SAMPLES = 1'000'000;
static constexpr std::size_t PAYLOAD_SIZE = 64;

static uint64_t now_ns()
{
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL
         + static_cast<uint64_t>(ts.tv_nsec);
}

static double percentile(std::vector<uint64_t>& samples, double p)
{
    const double index = p * (samples.size() - 1);
    const auto lower = static_cast<std::size_t>(index);
    const auto upper = lower + 1;

    if (upper >= samples.size()) {
        return static_cast<double>(samples.back());
    }

    const double fraction = index - lower;

    return samples[lower] * (1.0 - fraction)
         + samples[upper] * fraction;
}

int main()
{
    // ------------------------------------------------------------
    // 1. Create UDP socket
    // ------------------------------------------------------------

    const int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        std::cerr << "socket() failed: "
                  << std::strerror(errno) << '\n';
        return 1;
    }

    // ------------------------------------------------------------
    // 2. Destination
    //
    // We use localhost initially.
    //
    // This benchmark measures sendto() itself, not the physical
    // network.
    // ------------------------------------------------------------

    sockaddr_in destination{};

    destination.sin_family = AF_INET;
    destination.sin_port = htons(9999);

    if (inet_pton(AF_INET, "127.0.0.1", &destination.sin_addr) != 1) {
        std::cerr << "inet_pton() failed\n";
        close(sock);
        return 1;
    }

    // ------------------------------------------------------------
    // 3. Packet
    // ------------------------------------------------------------

    alignas(64) uint8_t packet[PAYLOAD_SIZE]{};

    // Put something recognizable into the packet.
    for (std::size_t i = 0; i < PAYLOAD_SIZE; ++i) {
        packet[i] = static_cast<uint8_t>(i);
    }

    // ------------------------------------------------------------
    // 4. Warm-up
    //
    // Don't measure the first calls. They may include things such
    // as lazy initialization, cache misses, etc.
    // ------------------------------------------------------------

    constexpr std::size_t WARMUP = 10'000;

    for (std::size_t i = 0; i < WARMUP; ++i) {

        const ssize_t result = sendto(
            sock,
            packet,
            PAYLOAD_SIZE,
            0,
            reinterpret_cast<sockaddr*>(&destination),
            sizeof(destination)
        );

        if (result < 0) {
            std::cerr << "warmup sendto() failed: "
                      << std::strerror(errno) << '\n';

            close(sock);
            return 1;
        }
    }

    // ------------------------------------------------------------
    // 5. Allocate samples
    // ------------------------------------------------------------

    std::vector<uint64_t> samples;
    samples.reserve(NUM_SAMPLES);

    // ------------------------------------------------------------
    // 6. Benchmark
    // ------------------------------------------------------------

    for (std::size_t i = 0; i < NUM_SAMPLES; ++i) {

        const uint64_t start = now_ns();

        const ssize_t result = sendto(
            sock,
            packet,
            PAYLOAD_SIZE,
            0,
            reinterpret_cast<sockaddr*>(&destination),
            sizeof(destination)
        );

        const uint64_t end = now_ns();

        if (result < 0) {
            std::cerr << "sendto() failed at iteration "
                      << i << ": "
                      << std::strerror(errno) << '\n';

            close(sock);
            return 1;
        }

        samples.push_back(end - start);
    }

    close(sock);

    // ------------------------------------------------------------
    // 7. Sort
    // ------------------------------------------------------------

    std::sort(samples.begin(), samples.end());

    // ------------------------------------------------------------
    // 8. Statistics
    // ------------------------------------------------------------

    const uint64_t min = samples.front();
    const uint64_t max = samples.back();

    const double p50   = percentile(samples, 0.50);
    const double p90   = percentile(samples, 0.90);
    const double p99   = percentile(samples, 0.99);
    const double p999  = percentile(samples, 0.999);
    const double p9999 = percentile(samples, 0.9999);

    const long double sum = std::accumulate(
        samples.begin(),
        samples.end(),
        static_cast<long double>(0)
    );

    const double mean =
        static_cast<double>(sum / samples.size());

    // ------------------------------------------------------------
    // 9. Print
    // ------------------------------------------------------------

    std::cout << '\n';
    std::cout << "UDP sendto() benchmark\n";
    std::cout << "======================\n";

    std::cout << "Samples       : " << NUM_SAMPLES << '\n';
    std::cout << "Payload       : " << PAYLOAD_SIZE << " bytes\n";
    std::cout << "Destination   : 127.0.0.1:9999\n";
    std::cout << '\n';

    std::cout << std::fixed << std::setprecision(2);

    std::cout << "min           : " << min << " ns\n";
    std::cout << "mean          : " << mean << " ns\n";
    std::cout << "p50           : " << p50 << " ns\n";
    std::cout << "p90           : " << p90 << " ns\n";
    std::cout << "p99           : " << p99 << " ns\n";
    std::cout << "p99.9         : " << p999 << " ns\n";
    std::cout << "p99.99        : " << p9999 << " ns\n";
    std::cout << "max           : " << max << " ns\n";

    std::cout << '\n';

    std::cout << "Approximate:\n";
    std::cout << "p50           : " << p50 / 1'000.0 << " us\n";
    std::cout << "p99           : " << p99 / 1'000.0 << " us\n";
    std::cout << "p99.9         : " << p999 / 1'000.0 << " us\n";
    std::cout << "max           : " << max / 1'000.0 << " us\n";

    return 0;
}
