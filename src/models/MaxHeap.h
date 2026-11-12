#ifndef MAX_HEAP_H
#define MAX_HEAP_H

#include <vector>
#include <algorithm>
#include <stdexcept>

using namespace std;

class MaxHeap {
public:
    vector<int> heap;
    long long comparisonCount = 0;
    long long swapCount = 0;

    MaxHeap() {}
    MaxHeap(const vector<int>& A) {
        buildMaxHeap(A);
    }

    void buildMaxHeap(const vector<int>& A) {
        heap = A;
        int n = (int)heap.size();
        for (int i = (n / 2) - 1; i >= 0; i--) {
            siftDown(i, n);
        }
    }

    void siftDown(int i, int heapSize) {
        int largest = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < heapSize) {
            ++comparisonCount;
            if (heap[left] > heap[largest]) largest = left;
        }
        if (right < heapSize) {
            ++comparisonCount;
            if (heap[right] > heap[largest]) largest = right;
        }
        if (largest != i) {
            swap(heap[i], heap[largest]);
            ++swapCount;
            siftDown(largest, heapSize);
        }
    }

    void siftUp(int i) {
        int parent = (i - 1) / 2;
        if (i > 0) {
            ++comparisonCount;
            if (heap[i] > heap[parent]) {
                swap(heap[i], heap[parent]);
                ++swapCount;
                siftUp(parent);
            }
        }
    }

    void insert(int val) {
        heap.push_back(val);
        siftUp((int)heap.size() - 1);
    }

    int extractMax() {
        if (heap.empty()) {
            throw runtime_error("Heap underflow");
        }
        int maxVal = heap[0];
        heap[0] = heap.back();
        heap.pop_back();
        if (!heap.empty()) {
            siftDown(0, (int)heap.size());
        }
        return maxVal;
    }

    void heapSort() {
        int n = (int)heap.size();
        buildMaxHeap(heap);
        for (int i = n - 1; i > 0; i--) {
            swap(heap[0], heap[i]);
            ++swapCount;
            siftDown(0, i);
        }
    }

    static void heapSortArray(vector<int>& A) {
        MaxHeap mh;
        mh.buildMaxHeap(A);
        int n = (int)mh.heap.size();
        for (int i = n - 1; i > 0; i--) {
            swap(mh.heap[0], mh.heap[i]);
            mh.siftDown(0, i);
        }
        A = mh.heap;
    }

    bool isMaxHeap() {
        int n = (int)heap.size();
        for (int i = 0; i < n; i++) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            if (left < n && heap[left] > heap[i]) return false;
            if (right < n && heap[right] > heap[i]) return false;
        }
        return true;
    }

    void resetMetrics() { comparisonCount = 0; swapCount = 0; }

    const vector<int>& getHeap() const { return heap; }
};

#endif
