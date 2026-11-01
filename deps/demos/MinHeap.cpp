#include <iostream>
#include <vector>
#include <utility>

class MinHeap {
 public:
  bool Empty() const { return heap.empty(); }
  size_t Size() const { return heap.size(); }

  int Peek() const {
    if (heap.empty()) throw std::out_of_range("Heap is empty");
    return heap[0];
  }

  void Insert(int val) {
    heap.push_back(val);
    ShiftUp(heap.size() - 1);
  }

  int ExtractMin() {
    if (heap.empty()) throw std::out_of_range("Heap is empty");

    int min_val = heap[0];
    heap[0] = heap.back();
    heap.pop_back();
    if (!heap.empty()) ShiftDown(0);

    return min_val;
  }

 private:
  std::vector<int> heap;

  int Parent(int i) { return (i - 1) / 2; }
  int LeftChild(int i) { return 2 * i + 1; }
  int RightChild(int i) { return 2 * i + 2; }

  void ShiftUp(int i) {
    // Keep moving up if parent > you
    while (i > 0 && heap[i] < heap[Parent(i)]) {
      std::swap(heap[i], heap[Parent(i)]);
      i = Parent(i);
    }
  }

  void ShiftDown(int i) {
    int value = heap[i];
    int left = heap[LeftChild(i)];
    int right = heap[RightChild(i)];
    
  }
};