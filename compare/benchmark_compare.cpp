#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <string>
#include <functional>
#include <fstream>
#include <sstream>
#include "rdemo.cpp"
#include "hat_array.hpp"
#include "doubling_array.hpp"

using Clock = std::chrono::high_resolution_clock;

struct Result {
    std::string name;
    int param_r = -1;
    int N = 0;
    long long final_words = 0;
    long long final_extra = 0;
    long long peak_words = 0;
    long long peak_extra = 0;
    double final_util = 0.0;
    double grow_ms = 0.0;
    double access_ms = 0.0;
    double shrink_ms = 0.0;
    long long checksum = 0;
};

template <class DS>
Result run_one(const std::string& name, int N, std::function<DS()> maker, int param_r = -1) {
    DS ds = maker();
    Result res;
    res.name = name;
    res.param_r = param_r;
    res.N = N;

    auto t1 = Clock::now();
    for (int i = 0; i < N; ++i) ds.grow(i);
    auto t2 = Clock::now();

    long long checksum = 0;
    int accessCount = std::min(N, 200000);
    int stride = std::max(1, N / std::max(1, accessCount));
    auto t3 = Clock::now();
    for (int i = 0, cnt = 0; i < N && cnt < accessCount; i += stride, ++cnt) checksum += ds.access(i);
    auto t4 = Clock::now();

    ds.resetPeak();
    auto t5 = Clock::now();
    for (int i = 0; i < N / 2; ++i) ds.shrink();
    auto t6 = Clock::now();

    res.final_words = ds.allocatedWords();
    res.final_extra = ds.extraWords();
    res.peak_words = ds.peakAllocatedWords();
    res.peak_extra = res.peak_words - ds.size();
    res.final_util = ds.utilization();
    res.grow_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
    res.access_ms = std::chrono::duration<double, std::milli>(t4 - t3).count();
    res.shrink_ms = std::chrono::duration<double, std::milli>(t6 - t5).count();
    res.checksum = checksum;
    return res;
}

void print_result(const Result& r) {
    std::cout << std::left << std::setw(12) << r.name;
    if (r.param_r >= 0) std::cout << " r=" << std::setw(2) << r.param_r;
    else std::cout << " r=- ";
    std::cout << " N=" << std::setw(8) << r.N
              << " final=" << std::setw(8) << r.final_words
              << " extra=" << std::setw(8) << r.final_extra
              << " peak=" << std::setw(8) << r.peak_words
              << " util=" << std::fixed << std::setprecision(3) << std::setw(7) << r.final_util
              << " grow(ms)=" << std::setw(9) << std::setprecision(3) << r.grow_ms
              << " access(ms)=" << std::setw(9) << r.access_ms
              << " shrink(ms)=" << std::setw(9) << r.shrink_ms
              << " checksum=" << r.checksum
              << "\n";
}

void write_csv(const std::vector<Result>& rows, const std::string& path) {
    std::ofstream out(path);
    out << "name,r,N,final_words,final_extra,peak_words,peak_extra,final_util,grow_ms,access_ms,shrink_ms,checksum\n";
    for (const auto& r : rows) {
        out << r.name << ',' << r.param_r << ',' << r.N << ','
            << r.final_words << ',' << r.final_extra << ','
            << r.peak_words << ',' << r.peak_extra << ','
            << r.final_util << ',' << r.grow_ms << ','
            << r.access_ms << ',' << r.shrink_ms << ','
            << r.checksum << '\n';
    }
}

int main() {
    std::vector<int> Ns = {10000, 50000, 100000, 200000};
    std::vector<int> rs = {2, 3, 4, 5};
    std::vector<Result> all;

    std::cout << "Benchmark: grow to N, sample-access, then shrink to N/2\n";
    std::cout << "Space unit: machine words allocated by the data structure\n\n";

    for (int N : Ns) {
        all.push_back(run_one<DoublingArray>("Double", N, [] { return DoublingArray(); }));
        all.push_back(run_one<HATArray>("HAT", N, [] { return HATArray(); }));
        for (int r : rs) {
            all.push_back(run_one<ResizableArrayR>("OptimalR", N, [r] { return ResizableArrayR(r, false); }, r));
        }
    }

    std::cout << std::left
              << std::setw(12) << "Method"
              << " cfg   N        final    extra    peak     util    grow(ms) access(ms)shrink(ms) checksum\n";
    std::cout << std::string(115, '-') << "\n";
    for (const auto& r : all) print_result(r);

    write_csv(all, "/mnt/data/benchmark_results.csv");
    std::cout << "\nCSV written to /mnt/data/benchmark_results.csv\n";
    return 0;
}
