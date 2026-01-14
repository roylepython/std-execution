#pragma once

#include <execution>
#include <simd>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

// C++26 performance features
#if __cpp_lib_simd >= 202300L
#include <simd>
#endif

#if __cpp_lib_hazard_pointers >= 202300L
#include <hazard_pointers>
#endif

namespace dualstack::performance {

// SIMD packet processing utilities
#if __cpp_lib_simd >= 202300L

template<typename T>
using simd_vector = std::simd<T>;

// SIMD-accelerated checksum calculation
template<typename T>
auto simd_checksum(std::span<const T> data) -> std::uint32_t {
    simd_vector<std::uint32_t> sum{0};
    
    constexpr auto simd_width = simd_vector<T>::size();
    const auto simd_iterations = data.size() / simd_width;
    
    for (std::size_t i = 0; i < simd_iterations; ++i) {
        simd_vector<T> chunk{&data[i * simd_width]};
        sum += std::simd_cast<std::uint32_t>(chunk);
    }
    
    // Horizontal reduction
    std::uint32_t result = 0;
    for (std::size_t i = 0; i < simd_vector<std::uint32_t>::size(); ++i) {
        result += sum[i];
    }
    
    // Handle remaining elements
    for (std::size_t i = simd_iterations * simd_width; i < data.size(); ++i) {
        result += static_cast<std::uint32_t>(data[i]);
    }
    
    return result;
}

// SIMD packet validation
template<typename Packet>
auto simd_validate_packets(std::span<const Packet> packets) -> bool {
    simd_vector<bool> valid{true};
    
    constexpr auto simd_width = simd_vector<bool>::size();
    const auto simd_iterations = packets.size() / simd_width;
    
    for (std::size_t i = 0; i < simd_iterations; ++i) {
        // Validate packet headers in parallel
        simd_vector<bool> chunk_valid;
        for (std::size_t j = 0; j < simd_width; ++j) {
            chunk_valid[j] = packets[i * simd_width + j].is_valid();
        }
        valid &= chunk_valid;
    }
    
    // Check results
    for (std::size_t i = 0; i < simd_vector<bool>::size(); ++i) {
        if (!valid[i]) return false;
    }
    
    // Validate remaining packets
    for (std::size_t i = simd_iterations * simd_width; i < packets.size(); ++i) {
        if (!packets[i].is_valid()) return false;
    }
    
    return true;
}

#endif // __cpp_lib_simd

// Hazard pointer-based lock-free structures
#if __cpp_lib_hazard_pointers >= 202300L

template<typename T>
class lockfree_queue {
private:
    struct node {
        std::shared_ptr<T> data;
        std::atomic<node*> next;
        
        node() : next(nullptr) {}
        explicit node(T&& value) : data(std::make_shared<T>(std::move(value))), next(nullptr) {}
    };
    
    std::atomic<node*> head_;
    std::atomic<node*> tail_;
    std::hazard_pointer_domain domain_;
    
public:
    lockfree_queue() {
        auto dummy = std::make_unique<node>();
        head_ = dummy.get();
        tail_ = dummy.release();
    }
    
    ~lockfree_queue() {
        while (auto* node = head_.load()) {
            head_ = node->next.load();
            delete node;
        }
    }
    
    void push(T&& item) {
        auto new_node = std::make_unique<node>(std::move(item));
        auto* new_node_ptr = new_node.release();
        
        auto* prev_tail = tail_.exchange(new_node_ptr);
        prev_tail->next.store(new_node_ptr);
    }
    
    auto pop() -> std::shared_ptr<T> {
        std::hazard_pointer hazard{domain_};
        
        node* head;
        do {
            head = hazard.protect(head_);
        } while (head != head_.load()); // Reload if hazard pointer protection failed
        
        node* next = head->next.load();
        if (next == nullptr) return nullptr; // Queue is empty
        
        // Advance head
        head_ = next;
        
        auto result = std::move(next->data);
        delete head;
        return result;
    }
};

#endif // __cpp_lib_hazard_pointers

// Thread pool for parallel execution
class thread_pool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    
public:
    explicit thread_pool(std::size_t num_threads = std::thread::hardware_concurrency()) {
        stop_ = false;
        for (std::size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                        
                        if (stop_ && tasks_.empty()) return;
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    
                    task();
                }
            });
        }
    }
    
    ~thread_pool() {
        stop_ = true;
        condition_.notify_all();
        for (auto& worker : workers_) {
            worker.join();
        }
    }
    
    template<typename F>
    auto enqueue(F&& f) -> std::future<std::invoke_result_t<F>> {
        using return_type = std::invoke_result_t<F>;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
        std::future<return_type> result = task->get_future();
        
        {
            std::scoped_lock lock(queue_mutex_);
            if (stop_) throw std::runtime_error("enqueue on stopped threadpool");
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
};

// Memory pool for network buffers
class memory_pool {
private:
    std::queue<std::byte*> free_blocks_;
    std::mutex mutex_;
    std::size_t block_size_;
    std::size_t capacity_;
    
public:
    explicit memory_pool(std::size_t block_size, std::size_t capacity)
        : block_size_(block_size), capacity_(capacity) {
        // Pre-allocate memory blocks
        for (std::size_t i = 0; i < capacity_; ++i) {
            free_blocks_.push(static_cast<std::byte*>(std::malloc(block_size_)));
        }
    }
    
    ~memory_pool() {
        std::scoped_lock lock(mutex_);
        while (!free_blocks_.empty()) {
            std::free(free_blocks_.front());
            free_blocks_.pop();
        }
    }
    
    auto allocate() -> std::byte* {
        std::scoped_lock lock(mutex_);
        if (free_blocks_.empty()) {
            return static_cast<std::byte*>(std::malloc(block_size_));
        }
        auto* ptr = free_blocks_.front();
        free_blocks_.pop();
        return ptr;
    }
    
    auto deallocate(std::byte* ptr) -> void {
        std::scoped_lock lock(mutex_);
        if (free_blocks_.size() < capacity_) {
            free_blocks_.push(ptr);
        } else {
            std::free(ptr);
        }
    }
};

// Performance monitoring utilities
class perf_monitor {
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::atomic<std::uint64_t> operation_count_;
    
public:
    perf_monitor() : operation_count_(0) {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    auto start_operation() -> void {
        ++operation_count_;
    }
    
    auto get_elapsed_time() const -> std::chrono::milliseconds {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    }
    
    auto get_operations_per_second() const -> double {
        auto elapsed_ms = get_elapsed_time().count();
        if (elapsed_ms == 0) return 0.0;
        return (static_cast<double>(operation_count_.load()) / elapsed_ms) * 1000.0;
    }
};

// Cache-friendly data structures
template<typename T, std::size_t Alignment = 64>
class aligned_vector {
private:
    std::vector<T> data_;
    
public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    
    explicit aligned_vector(std::size_t size = 0) : data_(size) {}
    
    auto push_back(const T& value) -> void {
        data_.push_back(value);
    }
    
    auto push_back(T&& value) -> void {
        data_.push_back(std::move(value));
    }
    
    auto operator[](std::size_t index) -> T& {
        return data_[index];
    }
    
    auto operator[](std::size_t index) const -> const T& {
        return data_[index];
    }
    
    auto size() const -> std::size_t {
        return data_.size();
    }
    
    auto begin() -> iterator {
        return data_.begin();
    }
    
    auto end() -> iterator {
        return data_.end();
    }
    
    auto begin() const -> const_iterator {
        return data_.begin();
    }
    
    auto end() const -> const_iterator {
        return data_.end();
    }
};

} // namespace dualstack::performance