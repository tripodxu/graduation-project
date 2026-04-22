#include <iostream>

#include <vector>

#include <cassert>

#include <iomanip>

#include <string>

#include <sstream>

#include <algorithm>

#include <limits>

using namespace std;



class ResizableArray {

private:

    int r_;

    int B_;

    int N_;

    vector<int**> A_;

    vector<int> n_;



    int ipow(int base, int exp) const {

        int res = 1;

        for (int i = 0; i < exp; ++i) res *= base;

        return res;

    }

    int blockSize(int level) const { return ipow(B_, level); }

    int* allocateBlock(int size) { return new int[size]; }

    void deallocateBlock(int* p) { delete[] p; }

    int* getBlock(int level, int idx) { return A_[level][idx]; }

    const int* getBlock(int level, int idx) const { return A_[level][idx]; }

    void setBlock(int level, int idx, int* blk) { A_[level][idx] = blk; }



    void copy(const int* src, int s, int* dst, int d, int len) {

        for (int i = 0; i < len; ++i) dst[d + i] = src[s + i];

    }



    int levelItemCount(int level) const {

        if (level == 1) {

            if (n_[1] == 0) return 0;

            return (n_[1] - 1) * blockSize(1) + n_[0];

        }

        return n_[level] * blockSize(level);

    }



    int computedSize() const {

        int total = 0;

        for (int level = 1; level < r_; ++level) total += levelItemCount(level);

        return total;

    }



    void clearStorage() {

        for (int i = 1; i < r_; ++i) {

            if (A_[i] != nullptr) {

                for (int j = 0; j < n_[i]; ++j) {

                    if (A_[i][j] != nullptr) deallocateBlock(A_[i][j]);

                }

                delete[] A_[i];

                A_[i] = nullptr;

            }

        }

    }



    void allocatePointerArrays() {

        A_.assign(r_, nullptr);

        for (int i = 1; i < r_; ++i) A_[i] = new int*[2 * B_]();

    }



    void check() const {

        assert(n_[0] >= 0 && n_[0] <= B_);

        for (int i = 1; i < r_; ++i) assert(n_[i] >= 0 && n_[i] <= 2 * B_);

        assert(computedSize() == N_);

    }



    void rebuildNoMetrics(int newB) {

        vector<int> old;

        old.reserve(N_);

        for (int i = 0; i < N_; ++i) old.push_back(access(i));

        clearStorage();

        B_ = newB;

        N_ = 0;

        n_.assign(r_, 0);

        allocatePointerArrays();

        for (int x : old) growNoMetrics(x);

    }



    void combineBlocksNoMetrics() {

        int k = -1;

        for (int i = 1; i < r_; ++i) {

            if (n_[i] < 2 * B_) { k = i; break; }

        }

        assert(k != -1);

        for (int i = k - 1; i >= 1; --i) {

            const int largeSize = blockSize(i + 1);

            const int smallSize = blockSize(i);

            int* big = allocateBlock(largeSize);

            for (int j = 0; j < B_; ++j) {

                int* blk = getBlock(i, j);

                copy(blk, 0, big, j * smallSize, smallSize);

                deallocateBlock(blk);

            }

            setBlock(i + 1, n_[i + 1], big);

            for (int j = B_; j < n_[i]; ++j) setBlock(i, j - B_, getBlock(i, j));

            for (int j = max(0, n_[i] - B_); j < n_[i]; ++j) setBlock(i, j, nullptr);

            n_[i] -= B_;

            n_[i + 1]++;

        }

        if (n_[1] > 0) n_[0] = B_;

    }



    void splitBlocksNoMetrics() {

        int k = -1;

        for (int i = 2; i < r_; ++i) {

            if (n_[i] > 0) { k = i; break; }

        }

        assert(k != -1);

        for (int i = k - 1; i >= 1; --i) {

            n_[i + 1]--;

            int* big = getBlock(i + 1, n_[i + 1]);

            const int smallSize = blockSize(i);

            for (int j = 0; j < B_; ++j) {

                int* blk = allocateBlock(smallSize);

                copy(big, j * smallSize, blk, 0, smallSize);

                setBlock(i, j, blk);

            }

            setBlock(i + 1, n_[i + 1], nullptr);

            n_[i] = B_;

            deallocateBlock(big);

        }

        if (n_[1] > 0) n_[0] = B_;

    }



    long long simulateResidentBytesAfterRebuild(int newB) const {

        ResizableArray tmp(r_);

        tmp.clearStorage();

        tmp.B_ = newB;

        tmp.N_ = 0;

        tmp.n_.assign(r_, 0);

        tmp.allocatePointerArrays();

        for (int i = 0; i < N_; ++i) tmp.growNoMetrics(0);

        return tmp.residentBytes();

    }



public:

    explicit ResizableArray(int r = 3) : r_(r), B_(2), N_(0), A_(r, nullptr), n_(r, 0) {

        assert(r_ >= 2);

        allocatePointerArrays();

    }

    ~ResizableArray() { clearStorage(); }



    void growNoMetrics(int x) {

        long long limit = 1;

        for (int i = 0; i < r_; ++i) limit *= B_;

        if (N_ == limit) rebuildNoMetrics(2 * B_);

        if (n_[1] == 2 * B_ && n_[0] == B_) combineBlocksNoMetrics();

        if (n_[1] == 0 || n_[0] == B_) {

            int* blk = allocateBlock(blockSize(1));

            setBlock(1, n_[1], blk);

            n_[1]++;

            n_[0] = 0;

        }

        int* last = getBlock(1, n_[1] - 1);

        last[n_[0]] = x;

        n_[0]++;

        N_++;

        check();

    }



    void shrinkNoMetrics() {

        if (N_ == 0) return;

        if (B_ > 2) {

            long long threshold = 1;

            for (int i = 0; i < r_; ++i) threshold *= (B_ / 4);

            if (N_ == threshold) rebuildNoMetrics(B_ / 2);

        }

        if (n_[1] == 0) splitBlocksNoMetrics();

        n_[0]--;

        N_--;

        if (n_[0] == 0 && n_[1] > 0) {

            deallocateBlock(getBlock(1, n_[1] - 1));

            setBlock(1, n_[1] - 1, nullptr);

            n_[1]--;

            n_[0] = (n_[1] > 0 ? blockSize(1) : 0);

        }

        check();

    }



    long long nextGrowTransientExtraBytes() const {

        long long extra = 0;

        long long limit = 1;

        for (int i = 0; i < r_; ++i) limit *= B_;

        if (N_ == limit) extra = max(extra, simulateResidentBytesAfterRebuild(2 * B_));

        if (n_[1] == 2 * B_ && n_[0] == B_) {

            int k = -1;

            for (int i = 1; i < r_; ++i) if (n_[i] < 2 * B_) { k = i; break; }

            if (k != -1) {

                for (int i = k - 1; i >= 1; --i) {

                    extra = max(extra, 1LL * blockSize(i + 1) * (long long)sizeof(int));

                }

            }

        }

        if (n_[1] == 0 || n_[0] == B_) extra = max(extra, 1LL * blockSize(1) * (long long)sizeof(int));

        return extra;

    }



    long long nextShrinkTransientExtraBytes() const {

        if (N_ == 0) return 0;

        long long extra = 0;

        if (B_ > 2) {

            long long threshold = 1;

            for (int i = 0; i < r_; ++i) threshold *= (B_ / 4);

            if (N_ == threshold) extra = max(extra, simulateResidentBytesAfterRebuild(B_ / 2));

        }

        if (n_[1] == 0) {

            int k = -1;

            for (int i = 2; i < r_; ++i) if (n_[i] > 0) { k = i; break; }

            if (k != -1) {

                for (int i = k - 1; i >= 1; --i) {

                    extra = max(extra, 1LL * B_ * blockSize(i) * (long long)sizeof(int));

                }

            }

        }

        return extra;

    }



    int access(int idx) const {

        assert(idx >= 0 && idx < N_);

        int remaining = idx;

        for (int level = r_ - 1; level >= 1; --level) {

            const int sz = blockSize(level);

            const int total = levelItemCount(level);

            if (remaining < total) {

                const int b = remaining / sz;

                const int off = remaining % sz;

                if (level == 1 && b == n_[1] - 1) assert(off < n_[0]);

                return getBlock(level, b)[off];

            }

            remaining -= total;

        }

        assert(false);

        return -1;

    }



    int size() const { return N_; }

    int base() const { return B_; }

    long long payloadBytes() const { return 1LL * N_ * (long long)sizeof(int); }

    long long pointerBytes() const { return 1LL * (r_ - 1) * 2 * B_ * (long long)sizeof(int*); }

    long long capacitySlots() const {

        long long cap = 0;

        for (int i = 1; i < r_; ++i) cap += 1LL * n_[i] * blockSize(i);

        return cap;

    }

    long long capacityBytes() const { return capacitySlots() * (long long)sizeof(int); }

    long long residentBytes() const { return capacityBytes() + pointerBytes(); }

    long long slackSlots() const { return capacitySlots() - N_; }

    long long slackBytes() const { return slackSlots() * (long long)sizeof(int); }

    double capacityUtilization() const {

        long long cap = capacitySlots();

        return cap ? 100.0 * N_ / (double)cap : 100.0;

    }

    double byteUtilization() const {

        long long bytes = residentBytes();

        return bytes ? 100.0 * payloadBytes() / (double)bytes : 100.0;

    }

};



struct InstantMetrics {

    int N = 0;

    int B = 0;

    long long payload_bytes = 0;

    long long capacity_slots = 0;

    long long capacity_bytes = 0;

    long long pointer_bytes = 0;

    long long resident_bytes = 0;

    long long slack_slots = 0;

    long long slack_bytes = 0;

    long long next_grow_transient_extra_bytes = 0;

    long long next_shrink_transient_extra_bytes = 0;

    long long adjust_space_worst_bytes = 0;

    long long total_space_worst_bytes = 0;

    double capacity_util = 100.0;

    double byte_util = 100.0;

    double total_byte_util_worst = 100.0;

};



struct ProcessSummary {

    string phase;

    int r = 0;

    int targetN = 0;

    int sampled_states = 0;

    double avg_capacity_util = 0.0;

    double avg_byte_util = 0.0;

    double avg_total_byte_util_worst = 0.0;

    double max_capacity_util = 0.0;

    double min_capacity_util = 0.0;

    double max_byte_util = 0.0;

    double min_byte_util = 0.0;

    double avg_transient_extra_bytes = 0.0;

    long long max_transient_extra_bytes = 0;

    InstantMetrics at_targetN;

};



static InstantMetrics captureInstant(const ResizableArray& arr) {

    InstantMetrics m;

    m.N = arr.size();

    m.B = arr.base();

    m.payload_bytes = arr.payloadBytes();

    m.capacity_slots = arr.capacitySlots();

    m.capacity_bytes = arr.capacityBytes();

    m.pointer_bytes = arr.pointerBytes();

    m.resident_bytes = arr.residentBytes();

    m.slack_slots = arr.slackSlots();

    m.slack_bytes = arr.slackBytes();

    m.next_grow_transient_extra_bytes = arr.nextGrowTransientExtraBytes();

    m.next_shrink_transient_extra_bytes = arr.nextShrinkTransientExtraBytes();

    m.adjust_space_worst_bytes = max(m.next_grow_transient_extra_bytes, m.next_shrink_transient_extra_bytes);

    m.total_space_worst_bytes = m.resident_bytes + m.adjust_space_worst_bytes;

    m.capacity_util = arr.capacityUtilization();

    m.byte_util = arr.byteUtilization();

    m.total_byte_util_worst = m.total_space_worst_bytes ? 100.0 * m.payload_bytes / (double)m.total_space_worst_bytes : 100.0;

    return m;

}



static ProcessSummary analyzeGrowProcess(int r, int targetN) {

    ResizableArray arr(r);

    ProcessSummary s;

    s.phase = "grow"; s.r = r; s.targetN = targetN;

    double sumCap = 0.0, sumByte = 0.0, sumTotalWorst = 0.0, sumTransient = 0.0;

    s.max_capacity_util = -numeric_limits<double>::infinity();

    s.min_capacity_util =  numeric_limits<double>::infinity();

    s.max_byte_util = -numeric_limits<double>::infinity();

    s.min_byte_util =  numeric_limits<double>::infinity();

    for (int i = 1; i <= targetN; ++i) {

        long long transient = arr.nextGrowTransientExtraBytes();

        arr.growNoMetrics(i);

        InstantMetrics m = captureInstant(arr);

        sumCap += m.capacity_util;

        sumByte += m.byte_util;

        sumTotalWorst += m.total_byte_util_worst;

        sumTransient += transient;

        s.max_transient_extra_bytes = max(s.max_transient_extra_bytes, transient);

        s.max_capacity_util = max(s.max_capacity_util, m.capacity_util);

        s.min_capacity_util = min(s.min_capacity_util, m.capacity_util);

        s.max_byte_util = max(s.max_byte_util, m.byte_util);

        s.min_byte_util = min(s.min_byte_util, m.byte_util);

        s.sampled_states++;

        if (i == targetN) s.at_targetN = m;

    }

    if (s.sampled_states > 0) {

        s.avg_capacity_util = sumCap / s.sampled_states;

        s.avg_byte_util = sumByte / s.sampled_states;

        s.avg_total_byte_util_worst = sumTotalWorst / s.sampled_states;

        s.avg_transient_extra_bytes = sumTransient / s.sampled_states;

    }

    return s;

}



static ProcessSummary analyzeShrinkProcess(int r, int targetN) {

    ResizableArray arr(r);

    for (int i = 1; i <= targetN; ++i) arr.growNoMetrics(i);

    ProcessSummary s;

    s.phase = "shrink"; s.r = r; s.targetN = targetN; s.at_targetN = captureInstant(arr);

    double sumCap = 0.0, sumByte = 0.0, sumTotalWorst = 0.0, sumTransient = 0.0;

    s.max_capacity_util = -numeric_limits<double>::infinity();

    s.min_capacity_util =  numeric_limits<double>::infinity();

    s.max_byte_util = -numeric_limits<double>::infinity();

    s.min_byte_util =  numeric_limits<double>::infinity();

    for (int cur = targetN; cur >= 1; --cur) {

        long long transient = arr.nextShrinkTransientExtraBytes();

        InstantMetrics m = captureInstant(arr);

        sumCap += m.capacity_util;

        sumByte += m.byte_util;

        sumTotalWorst += m.total_byte_util_worst;

        sumTransient += transient;

        s.max_transient_extra_bytes = max(s.max_transient_extra_bytes, transient);

        s.max_capacity_util = max(s.max_capacity_util, m.capacity_util);

        s.min_capacity_util = min(s.min_capacity_util, m.capacity_util);

        s.max_byte_util = max(s.max_byte_util, m.byte_util);

        s.min_byte_util = min(s.min_byte_util, m.byte_util);

        s.sampled_states++;

        arr.shrinkNoMetrics();

    }

    if (s.sampled_states > 0) {

        s.avg_capacity_util = sumCap / s.sampled_states;

        s.avg_byte_util = sumByte / s.sampled_states;

        s.avg_total_byte_util_worst = sumTotalWorst / s.sampled_states;

        s.avg_transient_extra_bytes = sumTransient / s.sampled_states;

    }

    return s;

}



static string fmtInt(long long x) {

    ostringstream os; os << x; return os.str();

}

static string fmtD(double x) {

    ostringstream os; os << fixed << setprecision(2) << x; return os.str();

}



static void printDivider(ostream& os, const vector<int>& widths) {

    os << '+';

    for (int w : widths) os << string(w + 2, '-') << '+';

    os << '\n';

}



static void printRow(ostream& os, const vector<string>& cells, const vector<int>& widths) {

    os << '|';

    for (size_t i = 0; i < cells.size(); ++i) {

        os << ' ' << left << setw(widths[i]) << cells[i] << '|' ;

    }

    os << '\n';

}



static void printProcessTable(ostream& os, const ProcessSummary& s) {

    vector<string> headers = {

        "phase","r","targetN","avg cap util %","avg byte util %","avg total util %",

        "min cap %","max cap %","min byte %","max byte %","avg transient B/op","max transient B/op"

    };

    vector<string> row = {

        s.phase, fmtInt(s.r), fmtInt(s.targetN), fmtD(s.avg_capacity_util), fmtD(s.avg_byte_util),

        fmtD(s.avg_total_byte_util_worst), fmtD(s.min_capacity_util), fmtD(s.max_capacity_util),

        fmtD(s.min_byte_util), fmtD(s.max_byte_util), fmtD(s.avg_transient_extra_bytes), fmtInt(s.max_transient_extra_bytes)

    };

    vector<int> widths(headers.size());

    for (size_t i = 0; i < headers.size(); ++i) widths[i] = max<int>(headers[i].size(), row[i].size());

    printDivider(os, widths); printRow(os, headers, widths); printDivider(os, widths); printRow(os, row, widths); printDivider(os, widths);

}



static void printStateTable(ostream& os, const InstantMetrics& m) {

    vector<string> headers = {

        "N","B","payload B","capacity slots","capacity B","pointer B","resident B",

        "slack slots","slack B","next grow extra B","next shrink extra B","adjust worst B",

        "total worst B","cap util %","byte util %","total util %"

    };

    vector<string> row = {

        fmtInt(m.N), fmtInt(m.B), fmtInt(m.payload_bytes), fmtInt(m.capacity_slots), fmtInt(m.capacity_bytes),

        fmtInt(m.pointer_bytes), fmtInt(m.resident_bytes), fmtInt(m.slack_slots), fmtInt(m.slack_bytes),

        fmtInt(m.next_grow_transient_extra_bytes), fmtInt(m.next_shrink_transient_extra_bytes),

        fmtInt(m.adjust_space_worst_bytes), fmtInt(m.total_space_worst_bytes), fmtD(m.capacity_util),

        fmtD(m.byte_util), fmtD(m.total_byte_util_worst)

    };

    vector<int> widths(headers.size());

    for (size_t i = 0; i < headers.size(); ++i) widths[i] = max<int>(headers[i].size(), row[i].size());

    printDivider(os, widths); printRow(os, headers, widths); printDivider(os, widths); printRow(os, row, widths); printDivider(os, widths);

}



static string csvHeader() {

    return "phase,r,targetN,sampled_states,avg_capacity_util,avg_byte_util,avg_total_byte_util_worst,max_capacity_util,min_capacity_util,max_byte_util,min_byte_util,avg_transient_extra_bytes,max_transient_extra_bytes,B_at_N,payload_bytes_at_N,capacity_slots_at_N,capacity_bytes_at_N,pointer_bytes_at_N,resident_bytes_at_N,slack_slots_at_N,slack_bytes_at_N,next_grow_extra_bytes_at_N,next_shrink_extra_bytes_at_N,adjust_worst_bytes_at_N,total_worst_bytes_at_N,capacity_util_at_N,byte_util_at_N,total_byte_util_worst_at_N";

}



static string toCsvRow(const ProcessSummary& s) {

    ostringstream out;

    out << fixed << setprecision(2)

        << s.phase << ',' << s.r << ',' << s.targetN << ',' << s.sampled_states << ','

        << s.avg_capacity_util << ',' << s.avg_byte_util << ',' << s.avg_total_byte_util_worst << ','

        << s.max_capacity_util << ',' << s.min_capacity_util << ','

        << s.max_byte_util << ',' << s.min_byte_util << ','

        << s.avg_transient_extra_bytes << ',' << s.max_transient_extra_bytes << ','

        << s.at_targetN.B << ',' << s.at_targetN.payload_bytes << ',' << s.at_targetN.capacity_slots << ','

        << s.at_targetN.capacity_bytes << ',' << s.at_targetN.pointer_bytes << ',' << s.at_targetN.resident_bytes << ','

        << s.at_targetN.slack_slots << ',' << s.at_targetN.slack_bytes << ','

        << s.at_targetN.next_grow_transient_extra_bytes << ',' << s.at_targetN.next_shrink_transient_extra_bytes << ','

        << s.at_targetN.adjust_space_worst_bytes << ',' << s.at_targetN.total_space_worst_bytes << ','

        << s.at_targetN.capacity_util << ',' << s.at_targetN.byte_util << ',' << s.at_targetN.total_byte_util_worst;

    return out.str();

}



int main(int argc, char* argv[]) {

    vector<int> queryNs;

    if (argc <= 1) queryNs = {1000032};

    else {

        for (int i = 1; i < argc; ++i) {

            int x = stoi(argv[i]);

            if (x <= 0) { cerr << "N must be positive.\n"; return 1; }

            queryNs.push_back(x);

        }

    }



    vector<ProcessSummary> all;

    for (int N : queryNs) {

        for (int r : {4}) {

            all.push_back(analyzeGrowProcess(r, N));

            all.push_back(analyzeShrinkProcess(r, N));

        }

    }



    cout << csvHeader() << "\n";

    for (const auto& s : all) cout << toCsvRow(s) << "\n";



    cerr << fixed << setprecision(2);

    cerr << "\n===== Readable tables =====\n\n";

    for (int N : queryNs) {

        cerr << "Target N = " << N << "\n\n";

        for (const auto& s : all) {

            if (s.targetN != N) continue;

            cerr << "Process summary\n";

            printProcessTable(cerr, s);

            cerr << "State at N\n";

            printStateTable(cerr, s.at_targetN);

            cerr << "\n";

        }

    }

    return 0;

}