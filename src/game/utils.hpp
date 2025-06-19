//
// Created by andy on 6/4/2025.
//

#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <mutex>

namespace game {
    template <typename T, std::size_t... Is>
    std::array<T, sizeof...(Is)> array_from_fn(const std::function<T(std::size_t)> &f, std::index_sequence<Is...>) {
        return {((void)Is, f(Is))...};
    }

    template <typename T, std::size_t N>
    std::array<T, N> array_from_fn(const std::function<T(std::size_t)> &f) {
        return array_from_fn<T>(f, std::make_index_sequence<N>());
    }

    // concurrent queue based on atomics (if the atomic types used are lock-free then this should be too).
    template <typename T>
    class concurrent_queue {
        struct node {
            T     value;
            node *prev;
            node *next;
        };

        std::atomic<node *> head = nullptr;
        std::atomic<node *> tail = nullptr;

      public:
        ~concurrent_queue() {
            auto* head = this->head.load();
            while (head) {
                auto *p = head;
                head    = head->next;
                delete p;
            }
        }

        concurrent_queue() = default;

        concurrent_queue(const concurrent_queue &other)                = delete;
        concurrent_queue(concurrent_queue &&other) noexcept            = default;
        concurrent_queue &operator=(const concurrent_queue &other)     = delete;
        concurrent_queue &operator=(concurrent_queue &&other) noexcept = default;

        [[nodiscard]] bool empty() const noexcept { return head == nullptr; }

        // 2 atomic ops per call
        void enqueue(T value) {
            node *node_  = new node();
            node_->value = std::move(value);
            node_->prev  = tail.exchange(node_);

            // set the head to node_ if the head is nullptr right now (first node). otherwise, we need to update the previous node's next
            if (node *np = nullptr; !head.compare_exchange_strong(np, node_)) {
                node_->prev->next = node_;
            }
        }

        // don't call this if the queue is not empty()
        // 2-3 atomic ops per call (2 unless the head and tail are the same, in which case we have an extra store).
        T dequeue() {
            node *node_ = head.exchange(head.load()->next);
            if (node_->next != nullptr) { // no weird locking necessary here
                node_->next->prev = nullptr;
            } else {
                tail.store(nullptr);
            }

            // move value out of node_ before we destroy node_
            T value = std::move(node_->value);
            delete node_;

            return std::move(value);
        }
    };
} // namespace game
