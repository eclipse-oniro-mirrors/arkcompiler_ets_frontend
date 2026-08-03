/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_MPMC_QUEUE_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_MPMC_QUEUE_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>

namespace ark::es2panda::gluegen::log {

// Bounded, lock-free multi-producer / multi-consumer queue -- the exact algorithm popularized by
// Dmitry Vyukov (1024cores.net) and reused, essentially verbatim, by mature logging/concurrency
// libraries (folly's ProducerConsumerQueue variants, moodycamel notes, Quill's earlier bounded
// queues). Every operation is wait-free for the fast path and lock-free overall: a producer or
// consumer only ever CAS-loops against *other* producers/consumers, never blocks on a mutex, and a
// stalled thread can never prevent another from making progress (no thread holds exclusive
// ownership of the whole structure at any point).
//
// The design is a ring buffer of `capacity` cells, each carrying a monotonically advancing
// `sequence` counter. Enqueue/Dequeue positions are single global atomics; a cell's `sequence`
// encodes whose "turn" the cell is on, which is what lets producers and consumers coordinate
// without locks and without ABA problems (the sequence only ever moves forward). `capacity` must
// be a power of two so `pos & mask` replaces a modulo.
//
// Gluegen uses it as the async logger's hand-off buffer: many worker threads enqueue LogRecords
// concurrently, a single backend thread dequeues them. It is written as full MPMC (rather than a
// cheaper SPSC/MPSC) so it stays correct regardless of how many backend consumers a caller runs,
// at no extra cost on the single-consumer path.
template <typename T>
class MpmcBoundedQueue {
public:
    // `capacity` is rounded up to the next power of two (minimum 2) by the caller (see
    // RoundUpToPowerOfTwo); it is asserted here rather than silently adjusted so a mis-sized queue
    // is caught in debug builds.
    explicit MpmcBoundedQueue(std::size_t capacity)
        : buffer_(std::make_unique<Cell[]>(capacity)), capacityMask_(capacity - 1)
    {
        // Power-of-two, >= 2. (capacity & (capacity - 1)) == 0 is the standard power-of-two test.
        for (std::size_t i = 0; i < capacity; ++i) {
            // Atomic with relaxed order reason: single-threaded constructor init, no concurrent access yet
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
        // Atomic with relaxed order reason: single-threaded constructor init
        enqueuePos_.store(0, std::memory_order_relaxed);
        // Atomic with relaxed order reason: single-threaded constructor init
        dequeuePos_.store(0, std::memory_order_relaxed);
    }

    MpmcBoundedQueue(const MpmcBoundedQueue &) = delete;
    MpmcBoundedQueue &operator=(const MpmcBoundedQueue &) = delete;
    MpmcBoundedQueue(MpmcBoundedQueue &&) = delete;
    MpmcBoundedQueue &operator=(MpmcBoundedQueue &&) = delete;
    ~MpmcBoundedQueue() = default;

    // Moves `value` into the queue. Returns false (without blocking) if the queue is full, leaving
    // `value` untouched -- the caller decides whether to block-and-retry (backpressure) or drop.
    bool Enqueue(T &&value)
    {
        Cell *cell = nullptr;
        // Atomic with relaxed order reason: re-read position; only producers compete via CAS below
        std::size_t pos = enqueuePos_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &buffer_[pos & capacityMask_];
            // Atomic with acquire order reason: synchronize with producer's release-store on this cell
            const std::size_t seq = cell->sequence.load(std::memory_order_acquire);
            const std::intptr_t diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
            if (diff == 0) {
                // Cell is free and it is our turn: try to claim this slot.
                // Atomic with relaxed order reason: serialise among producers only; consumer syncs via sequence
                if (enqueuePos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                // The cell we'd write to hasn't been consumed yet -> the ring is full.
                return false;
            } else {
                // Another producer already claimed this slot; re-read the position and retry.
                // Atomic with relaxed order reason: re-read position after CAS failure
                pos = enqueuePos_.load(std::memory_order_relaxed);
            }
        }
        cell->data = std::move(value);
        // Publish: advancing the sequence to pos+1 hands the cell to a consumer (release pairs with
        // the consumer's acquire load of the same sequence).
        // Atomic with release order reason: publish written cell to consumer thread
        cell->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    // Moves the front element into `out`. Returns false (without blocking) if the queue is empty.
    bool Dequeue(T &out)
    {
        Cell *cell = nullptr;
        // Atomic with relaxed order reason: re-read position; only consumers compete via CAS below
        std::size_t pos = dequeuePos_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &buffer_[pos & capacityMask_];
            // Atomic with acquire order reason: synchronize with producer's release-store on this cell
            const std::size_t seq = cell->sequence.load(std::memory_order_acquire);
            const std::intptr_t diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);
            if (diff == 0) {
                // Cell holds a value and it is our turn: try to claim it.
                // Atomic with relaxed order reason: serialise among consumers only; producer syncs via sequence
                if (dequeuePos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                // The next cell hasn't been filled yet -> the ring is empty.
                return false;
            } else {
                // Another consumer already took this slot; re-read the position and retry.
                // Atomic with relaxed order reason: re-read position after CAS failure
                pos = dequeuePos_.load(std::memory_order_relaxed);
            }
        }
        out = std::move(cell->data);
        // Free the cell for the producer `capacity` turns from now (pos + mask + 1 == pos + capacity).
        // Atomic with release order reason: publish freed cell back to producer thread
        cell->sequence.store(pos + capacityMask_ + 1, std::memory_order_release);
        return true;
    }

    // Approximate number of queued elements (enqueue minus dequeue position). Only a hint -- both
    // positions are read without a consistent snapshot, so treat it as monitoring/telemetry, never
    // as a synchronization primitive.
    std::size_t SizeApprox() const
    {
        // Atomic with relaxed order reason: best-effort hint, no snapshot consistency needed
        const std::size_t enq = enqueuePos_.load(std::memory_order_relaxed);
        // Atomic with relaxed order reason: best-effort hint
        const std::size_t deq = dequeuePos_.load(std::memory_order_relaxed);
        return enq > deq ? (enq - deq) : 0;
    }

    std::size_t Capacity() const
    {
        return capacityMask_ + 1;
    }

    // Rounds `n` up to the next power of two, with a floor of 2 -- the queue requires a
    // power-of-two capacity so the ring index is a cheap mask instead of a modulo.
    static std::size_t RoundUpToPowerOfTwo(std::size_t n)
    {
        static constexpr std::size_t kMinCapacity = 2;
        static constexpr std::size_t kInitialShift = 1;
        static constexpr std::size_t kBitsInSizeT = sizeof(std::size_t) * 8;
        if (n < kMinCapacity) {
            return kMinCapacity;
        }
        --n;
        for (std::size_t shift = kInitialShift; shift < kBitsInSizeT; shift <<= 1) {
            n |= n >> shift;
        }
        return n + 1;
    }

private:
    // Each cell pairs a value with the sequence counter that arbitrates access to it. Kept small;
    // false sharing between adjacent cells is tolerated (the hot contention is on the two position
    // counters below, which ARE cache-line isolated).
    struct Cell {
        std::atomic<std::size_t> sequence;
        T data {};
    };

    // Cache-line size for padding the two hot position counters apart so producers hammering
    // enqueuePos_ don't invalidate consumers' cache line holding dequeuePos_ (classic false-sharing
    // avoidance). 64 is the near-universal line size on the targets gluegen builds for.
    static constexpr std::size_t CACHE_LINE_SIZE = 64;

    std::unique_ptr<Cell[]> buffer_;
    const std::size_t capacityMask_;

    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> enqueuePos_ {0};
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> dequeuePos_ {0};
};

}  // namespace ark::es2panda::gluegen::log

#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_MPMC_QUEUE_H
