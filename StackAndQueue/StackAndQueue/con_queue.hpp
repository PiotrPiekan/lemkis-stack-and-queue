#include <condition_variable>
#include <format>
#include <iostream>
#include <mutex>
#include <stdexcept>

#include "node.hpp"

template <typename T>
class ConcurrentQueue {
  private:
    std::unique_ptr<Node<T>> front_;
    Node<T>* rear_ = nullptr;
    std::size_t size_ = 0;

    // Synchronization primitives
    mutable std::mutex mutex_;
    std::condition_variable not_empty_cv_;

  public:
    // Constructor
    ConcurrentQueue() = default;

    // Destructor
    ~ConcurrentQueue() = default;

    // Copy constructor
    ConcurrentQueue(const ConcurrentQueue& other) {
        std::lock_guard<std::mutex> lock(other.mutex_);
        Node<T>* current = other.front_.get();
        while (current != nullptr) {
            push(current->data);
            current = current->next.get();
        }
    }

    // Move constructor
    ConcurrentQueue(ConcurrentQueue&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        front_ = std::move(other.front_);
        rear_ = other.rear_;
        size_ = other.size_;
        other.rear_ = nullptr;
        other.size_ = 0;
    }

    // Copy assignment operator
    ConcurrentQueue& operator=(const ConcurrentQueue& other) {
        if (this != &other) {
            // Need to lock both queues to avoid deadlock
            std::lock(mutex_, other.mutex_);
            std::lock_guard<std::mutex> lock_this(mutex_, std::adopt_lock);
            std::lock_guard<std::mutex> lock_other(other.mutex_,
                                                   std::adopt_lock);

            // Clear current queue
            front_.reset();
            rear_ = nullptr;
            size_ = 0;

            // Copy elements from other queue
            Node<T>* current = other.front_.get();
            while (current != nullptr) {
                push_unsafe(current->data);  // Using unsafe version since we
                                             // already have the lock
                current = current->next.get();
            }
        }
        return *this;
    }

    // Move assignment operator
    ConcurrentQueue& operator=(ConcurrentQueue&& other) noexcept {
        if (this != &other) {
            std::lock(mutex_, other.mutex_);
            std::lock_guard<std::mutex> lock_this(mutex_, std::adopt_lock);
            std::lock_guard<std::mutex> lock_other(other.mutex_,
                                                   std::adopt_lock);

            front_ = std::move(other.front_);
            rear_ = other.rear_;
            size_ = other.size_;
            other.rear_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    // Helper method for internal use when lock is already held
    void push_unsafe(T value) {
        auto newNode = std::make_unique<Node<T>>(std::move(value));
        if (front_ == nullptr) {
            front_ = std::move(newNode);
            rear_ = front_.get();
        } else {
            rear_->next = std::move(newNode);
            rear_ = rear_->next.get();
        }
        size_++;
    }

    // Add an element to the queue
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            push_unsafe(std::move(value));
        }
        // Notify one waiting thread that data is available
        not_empty_cv_.notify_one();
    }

    // Remove the front element (non-blocking)
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (empty_unsafe()) { return false; }

        value = std::move(front_->data);
        if (front_.get() == rear_) { rear_ = nullptr; }
        front_ = std::move(front_->next);
        size_--;
        return true;
    }

    // Remove the front element (blocking)
    void pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_cv_.wait(lock, [this] { return !empty_unsafe(); });

        value = std::move(front_->data);
        if (front_.get() == rear_) { rear_ = nullptr; }
        front_ = std::move(front_->next);
        size_--;
    }

    // Timed wait pop
    bool pop(T& value, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        bool success = not_empty_cv_.wait_for(
            lock, timeout, [this] { return !empty_unsafe(); });

        if (!success) {
            return false;  // Timed out
        }

        value = std::move(front_->data);
        if (front_.get() == rear_) { rear_ = nullptr; }
        front_ = std::move(front_->next);
        size_--;
        return true;
    }

    // Helper method to check if queue is empty when lock is already held
    bool empty_unsafe() const { return front_ == nullptr; }

    // Check if the queue is empty
    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return empty_unsafe();
    }

    // Get the size of the queue
    [[nodiscard]] std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }

    // Try to peek at front element without removing
    bool try_peek(T& value) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (empty_unsafe()) { return false; }
        value = front_->data;
        return true;
    }
};