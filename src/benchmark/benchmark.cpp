#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <stdexcept>
#include "models/BST.h"
#include "models/AVLTree.h"
#include "models/MaxHeap.h"

using namespace std;
using namespace std::chrono;

static const unsigned int SEED = 20261111;

static vector<int> makeData(int n, const string& pattern, mt19937& rng) {
    vector<int> data(n);
    iota(data.begin(), data.end(), 0);
    if (pattern == "random") {
        shuffle(data.begin(), data.end(), rng);
    } else if (pattern == "reverse") {
        reverse(data.begin(), data.end());
    } else if (pattern == "duplicates") {
        for (int i = 0; i < n; ++i) data[i] = i % 10;
    }
    return data;
}

static long long elapsedNs(steady_clock::time_point start) {
    return duration_cast<nanoseconds>(steady_clock::now() - start).count();
}

static long long median(vector<long long> samples) {
    sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

struct SortMetrics {
    long long comparisons = 0;
    long long swaps = 0;
    long long moves = 0;
    int maxStackDepth = 0;
};

static int medianOfThree(const vector<int>& values, int lo, int hi, SortMetrics& metrics) {
    int mid = lo + (hi - lo) / 2;
    int a = values[lo], b = values[mid], c = values[hi];
    ++metrics.comparisons;
    if (a < b) {
        ++metrics.comparisons;
        if (b < c) return b;
        ++metrics.comparisons;
        return a < c ? c : a;
    }
    ++metrics.comparisons;
    if (a < c) return a;
    ++metrics.comparisons;
    return b < c ? c : b;
}

static void quickSortRange(vector<int>& values, int lo, int hi,
                           SortMetrics& metrics, int depth) {
    while (lo < hi) {
        metrics.maxStackDepth = max(metrics.maxStackDepth, depth);
        int pivot = medianOfThree(values, lo, hi, metrics);
        int less = lo, current = lo, greater = hi;
        while (current <= greater) {
            ++metrics.comparisons;
            if (values[current] < pivot) {
                if (less != current) {
                    swap(values[less], values[current]);
                    ++metrics.swaps;
                }
                ++less;
                ++current;
            } else {
                ++metrics.comparisons;
                if (values[current] > pivot) {
                    if (current != greater) {
                        swap(values[current], values[greater]);
                        ++metrics.swaps;
                    }
                    --greater;
                } else {
                    ++current;
                }
            }
        }
        if (less - lo < hi - greater) {
            quickSortRange(values, lo, less - 1, metrics, depth + 1);
            lo = greater + 1;
        } else {
            quickSortRange(values, greater + 1, hi, metrics, depth + 1);
            hi = less - 1;
        }
    }
}

static SortMetrics quickSort(vector<int>& values) {
    SortMetrics metrics;
    if (values.size() > 1)
        quickSortRange(values, 0, (int)values.size() - 1, metrics, 1);
    return metrics;
}

static void mergeSortRange(vector<int>& values, vector<int>& scratch,
                           int lo, int hi, SortMetrics& metrics, int depth) {
    metrics.maxStackDepth = max(metrics.maxStackDepth, depth);
    if (lo >= hi) return;
    int mid = lo + (hi - lo) / 2;
    mergeSortRange(values, scratch, lo, mid, metrics, depth + 1);
    mergeSortRange(values, scratch, mid + 1, hi, metrics, depth + 1);
    int left = lo, right = mid + 1, out = lo;
    while (left <= mid && right <= hi) {
        ++metrics.comparisons;
        scratch[out++] = values[left] <= values[right] ? values[left++] : values[right++];
        ++metrics.moves;
    }
    while (left <= mid) { scratch[out++] = values[left++]; ++metrics.moves; }
    while (right <= hi) { scratch[out++] = values[right++]; ++metrics.moves; }
    for (int i = lo; i <= hi; ++i) { values[i] = scratch[i]; ++metrics.moves; }
}

static SortMetrics mergeSort(vector<int>& values) {
    SortMetrics metrics;
    if (values.size() > 1) {
        vector<int> scratch(values.size());
        mergeSortRange(values, scratch, 0, (int)values.size() - 1, metrics, 1);
    }
    return metrics;
}

static void benchmarkSortComparison() {
    for (int n : {100, 1000, 10000}) {
        for (const string pattern : {"random", "sorted", "reverse", "duplicates"}) {
            mt19937 rng(SEED + (unsigned int)n + (unsigned int)pattern.size());
            const vector<int> data = makeData(n, pattern, rng);
            vector<int> expected = data;
            sort(expected.begin(), expected.end());

            vector<long long> heapTimes, mergeTimes, quickTimes;
            SortMetrics heapMetrics, mergeMetrics, quickMetrics;
            for (int run = 0; run < 5; ++run) {
                MaxHeap heap;
                heap.heap = data;
                heap.resetMetrics();
                auto start = steady_clock::now();
                heap.heapSort();
                heapTimes.push_back(elapsedNs(start));
                if (heap.heap != expected) throw runtime_error("Heap Sort comparison failed");
                heapMetrics.comparisons = heap.comparisonCount;
                heapMetrics.swaps = heap.swapCount;
            }
            for (int run = 0; run < 5; ++run) {
                vector<int> values = data;
                auto start = steady_clock::now();
                mergeMetrics = mergeSort(values);
                mergeTimes.push_back(elapsedNs(start));
                if (values != expected) throw runtime_error("Merge Sort comparison failed");
            }
            for (int run = 0; run < 5; ++run) {
                vector<int> values = data;
                auto start = steady_clock::now();
                quickMetrics = quickSort(values);
                quickTimes.push_back(elapsedNs(start));
                if (values != expected) throw runtime_error("Quick Sort comparison failed");
            }
            cout << "SORT_COMPARE," << n << ',' << pattern << ",HEAP_SORT," << median(heapTimes) << ','
                 << heapMetrics.comparisons << ',' << heapMetrics.swaps << ",0,0\n";
            cout << "SORT_COMPARE," << n << ',' << pattern << ",MERGE_SORT," << median(mergeTimes) << ','
                 << mergeMetrics.comparisons << ',' << mergeMetrics.moves << ',' << n * (int)sizeof(int)
                 << ',' << mergeMetrics.maxStackDepth << "\n";
            cout << "SORT_COMPARE," << n << ',' << pattern << ",QUICK_SORT_3WAY," << median(quickTimes) << ','
                 << quickMetrics.comparisons << ',' << quickMetrics.swaps << ",0,"
                 << quickMetrics.maxStackDepth << "\n";
        }
    }
}

static long long countSearchComparisons(TreeNode* root, const vector<int>& queries) {
    long long comparisons = 0;
    for (int key : queries) {
        TreeNode* node = root;
        while (node) {
            ++comparisons;
            int nodeKey = ((BSTNode*)node)->key;
            if (key == nodeKey) break;
            ++comparisons;
            node = key < nodeKey ? node->left : node->right;
        }
    }
    return comparisons;
}

static void benchmarkHeights(mt19937& rng) {
    for (int n : {100, 1000, 10000}) {
        for (const string pattern : {"sorted", "random"}) {
            vector<int> data = makeData(n, pattern, rng);
            BST bst;
            AVLTree avl;
            for (int key : data) { bst.insert(key); avl.insert(key); }
            cout << "HEIGHT," << n << ',' << pattern << ",BST," << bst.height() << "\n";
            cout << "HEIGHT," << n << ',' << pattern << ",AVL," << avl.height() << "\n";
        }
    }
}

static void benchmarkSearch(mt19937& rng) {
    for (int n : {100, 1000, 10000}) {
        uniform_int_distribution<int> queryKey(0, n - 1);
        vector<int> queries(10000);
        for (int& key : queries) key = queryKey(rng);
        for (const string pattern : {"sorted", "random"}) {
            vector<int> data = makeData(n, pattern, rng);
            BST bst;
            AVLTree avl;
            for (int key : data) { bst.insert(key); avl.insert(key); }

            long long bstChecksum = 0, avlChecksum = 0;
            vector<long long> bstTimes, avlTimes;
            for (int run = 0; run < 5; ++run) {
                bstChecksum = 0;
                auto start = steady_clock::now();
                for (int key : queries) {
                    BSTNode* found = bst.search(key);
                    if (found) bstChecksum += found->key;
                }
                bstTimes.push_back(elapsedNs(start));
            }
            long long bstComparisons = countSearchComparisons(bst.root, queries);
            for (int run = 0; run < 5; ++run) {
                avlChecksum = 0;
                auto start = steady_clock::now();
                for (int key : queries) {
                    BSTNode* found = avl.search(key);
                    if (found) avlChecksum += found->key;
                }
                avlTimes.push_back(elapsedNs(start));
            }
            long long avlComparisons = countSearchComparisons(avl.root, queries);

            cout << "SEARCH," << n << ',' << pattern << ",BST,10000,"
                 << fixed << setprecision(3) << (double)bstComparisons / queries.size()
                 << ',' << median(bstTimes) << ',' << bstChecksum << "\n";
            cout << "SEARCH," << n << ',' << pattern << ",AVL,10000,"
                 << fixed << setprecision(3) << (double)avlComparisons / queries.size()
                 << ',' << median(avlTimes) << ',' << avlChecksum << "\n";
        }
    }
}

static void benchmarkAvlRotations(mt19937& rng) {
    for (int n : {100, 1000, 10000}) {
        for (const string pattern : {"sorted", "random"}) {
            vector<int> data = makeData(n, pattern, rng);
            vector<int> deleteOrder = data;
            if (pattern == "random") shuffle(deleteOrder.begin(), deleteOrder.end(), rng);
            vector<long long> insertTimes, deleteTimes;
            long long insertRotations = 0, deleteRotations = 0;
            long long maxInsertRotations = 0, maxDeleteRotations = 0;
            for (int run = 0; run < 5; ++run) {
                AVLTree sample;
                auto start = steady_clock::now();
                for (int key : data) {
                    long long before = sample.getRotationCount();
                    sample.insert(key);
                    maxInsertRotations = max(maxInsertRotations, sample.getRotationCount() - before);
                }
                insertTimes.push_back(elapsedNs(start));
                insertRotations = sample.getRotationCount();
            }
            for (int run = 0; run < 5; ++run) {
                AVLTree sample;
                for (int key : data) sample.insert(key);
                sample.resetRotationCount();
                auto start = steady_clock::now();
                for (int key : deleteOrder) {
                    long long before = sample.getRotationCount();
                    sample.remove(key);
                    maxDeleteRotations = max(maxDeleteRotations, sample.getRotationCount() - before);
                }
                deleteTimes.push_back(elapsedNs(start));
                deleteRotations = sample.getRotationCount();
            }
            cout << "AVL_ROTATIONS," << n << ',' << pattern << ",insert," << insertRotations << ','
                 << fixed << setprecision(3) << (double)insertRotations / n << ','
                 << maxInsertRotations << ',' << median(insertTimes) << "\n";
            cout << "AVL_ROTATIONS," << n << ',' << pattern << ",delete," << deleteRotations << ','
                 << fixed << setprecision(3) << (double)deleteRotations / n << ','
                 << maxDeleteRotations << ',' << median(deleteTimes) << "\n";
        }
    }
}

static void benchmarkOrderStatistics(mt19937& rng) {
    const int n = 10000;
    const int queryCount = 1000;
    vector<int> data = makeData(n, "random", rng);
    vector<int> queries(queryCount);
    uniform_int_distribution<int> kth(1, n);
    for (int& query : queries) query = kth(rng);
    BST bst;
    AVLTree avl;
    for (int key : data) { bst.insert(key); avl.insert(key); }
    for (int query : queries) {
        if (bst.kthSmallest(query) != avl.kth(query))
            throw runtime_error("BST and AVL kth results differ");
    }

    long long bstChecksum = 0, avlChecksum = 0;
    vector<long long> bstTimes, avlTimes;
    for (int run = 0; run < 5; ++run) {
        bstChecksum = 0;
        auto start = steady_clock::now();
        for (int query : queries) bstChecksum += bst.kthSmallest(query);
        bstTimes.push_back(elapsedNs(start));
    }
    for (int run = 0; run < 5; ++run) {
        avlChecksum = 0;
        auto start = steady_clock::now();
        for (int query : queries) avlChecksum += avl.kth(query);
        avlTimes.push_back(elapsedNs(start));
    }
    cout << "ORDER_STATISTIC,10000,random,BST_INORDER_SCAN,1000," << median(bstTimes) << ',' << bstChecksum << "\n";
    cout << "ORDER_STATISTIC,10000,random,AVL_SUBTREE_SIZE,1000," << median(avlTimes) << ',' << avlChecksum << "\n";
}

static void benchmarkHeap(mt19937& rng) {
    for (int n : {100, 1000, 10000}) {
        for (const string pattern : {"random", "sorted", "reverse"}) {
            vector<int> data = makeData(n, pattern, rng);
            vector<long long> buildTimes;
            MaxHeap heap;
            for (int run = 0; run < 5; ++run) {
                MaxHeap sample;
                auto start = steady_clock::now();
                sample.buildMaxHeap(data);
                buildTimes.push_back(elapsedNs(start));
                heap = sample;
            }
            if (!heap.isMaxHeap()) throw runtime_error("Heap build failed validation");
            cout << "HEAP_BUILD," << n << ',' << pattern << ",BOTTOM_UP," << median(buildTimes) << ','
                 << heap.comparisonCount << ',' << heap.swapCount << "\n";
        }

        for (const string pattern : {"random", "sorted", "reverse", "duplicates"}) {
            vector<int> data = makeData(n, pattern, rng);
            vector<long long> sortTimes;
            MaxHeap heap;
            for (int run = 0; run < 5; ++run) {
                MaxHeap sample;
                sample.heap = data;
                sample.resetMetrics();
                auto start = steady_clock::now();
                sample.heapSort();
                sortTimes.push_back(elapsedNs(start));
                heap = sample;
            }
            if (!is_sorted(heap.heap.begin(), heap.heap.end())) throw runtime_error("Heap sort failed validation");
            cout << "HEAP_SORT," << n << ',' << pattern << ",HEAP_SORT," << median(sortTimes) << ','
                 << heap.comparisonCount << ',' << heap.swapCount << "\n";
        }
    }
}

int main(int argc, char** argv) {
    bool eventsEnabled = true;
    for (int i = 1; i < argc; ++i) {
        string option = argv[i];
        if (option == "--disable-events") eventsEnabled = false;
        else if (option == "--help") {
            cerr << "Usage: benchmark.exe [--disable-events]\n";
            return 0;
        } else {
            cerr << "Unknown option: " << option << "\nUsage: benchmark.exe [--disable-events]\n";
            return 2;
        }
    }
    BinaryTree::setEventRecordingEnabled(eventsEnabled);
    mt19937 rng(SEED);
    cout << "# Vault Heist benchmark results; seed=" << SEED << "\n";
    cout << "# Event recording: " << (eventsEnabled ? "enabled" : "disabled") << "\n";
    cout << "# HEIGHT: type,n,input,structure,height\n";
    benchmarkHeights(rng);
    cout << "# SEARCH: type,n,input,structure,queries,avg_comparisons,median_time_ns,checksum\n";
    benchmarkSearch(rng);
    cout << "# AVL_ROTATIONS: type,n,input,operation,total_rotations,avg_per_op,max_in_one_op,median_time_ns\n";
    benchmarkAvlRotations(rng);
    cout << "# ORDER_STATISTIC: type,n,input,method,queries,median_time_ns,checksum\n";
    benchmarkOrderStatistics(rng);
    cout << "# HEAP_BUILD / HEAP_SORT: type,n,input,method,median_time_ns,comparisons,swaps\n";
    benchmarkHeap(rng);
    cout << "# SORT_COMPARE: type,n,input,algorithm,median_time_ns,comparisons,swaps_or_moves,aux_array_bytes,max_stack_depth\n";
    benchmarkSortComparison();
    return 0;
}
