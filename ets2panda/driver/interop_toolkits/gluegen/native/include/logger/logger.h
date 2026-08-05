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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOGGER_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOGGER_H

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <ctime>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include <mutex>

#include "log_level.h"
#include "log_record.h"
#include "log_sink.h"
#include "mpmc_queue.h"

namespace ark::es2panda::gluegen::log {

// What to do when the async queue is full (producers outrunning the backend thread).
enum class OverflowPolicy {
    // Spin-retry until a slot frees up: never loses a record, but a slow sink can back-pressure
    // (briefly stall) producers. The safe default for a build tool where completeness > latency.
    BLOCK,
    // Drop the record and bump a counter (queryable via DroppedCount): bounds producer latency at
    // the cost of losing records under sustained overload. Choose for latency-critical paths.
    DROP,
};

// Tunables captured once at Logger construction. Defaults target gluegen: async on, an 8192-slot
// ring, INFO threshold, a 1s flush cadence, crash handler installed, and lossless BLOCK overflow.
struct LoggerConfig {
    bool async = true;
    std::size_t queueCapacity = 8192;
    LogLevel level = LogLevel::INFO;
    std::chrono::milliseconds flushInterval {1000};
    bool installCrashHandler = true;
    OverflowPolicy overflowPolicy = OverflowPolicy::BLOCK;
};

class Logger;

namespace detail {

// ---------------------------------------------------------------------------------------------
// Crash-safety plumbing.
//
// The hard rule inside a signal handler is that it may call only async-signal-safe functions --
// no malloc, no mutexes, no iostream, nothing that could deadlock or corrupt state if the signal
// interrupted that very facility. Formatting and flushing logs does all of those things, so the
// handler must NOT do the draining itself. Instead:
//
//   1. The handler only sets atomic flags (g_crashSignal + a per-logger crash-request flag) and
//      then spins on a bounded nanosleep loop waiting for the backend thread to acknowledge.
//   2. The backend thread, which is already allowed to allocate/flush, notices the request every
//      loop iteration, drains the queue, flushes every sink, and sets an "acknowledged" flag.
//   3. The handler restores the default disposition and re-raises the signal so the process still
//      crashes with the correct signal/core dump -- we only bought enough time to persist logs.
//
// This is the same delegation strategy Quill/folly-style crash-safe loggers use. Global state is
// a set of inline atomics (one definition across TUs thanks to C++17 inline variables). A single
// active logger is supported for crash handling; the last logger to install wins.
// ---------------------------------------------------------------------------------------------

// The logger whose backend thread should drain on crash. Raw pointer: set to the live Logger while
// it runs, cleared on shutdown. Only ever read by the async-signal context as an opaque token that
// gates on the flags below; the actual work happens on the backend thread.
inline std::atomic<Logger *> g_crashLogger {nullptr};
// The signal number caught (0 = none). Also the handler's "in progress" guard.
inline std::atomic<int> g_crashSignal {0};
// Set by the backend thread once it has drained + flushed in response to a crash.
inline std::atomic<bool> g_crashDrained {false};
// True once signal dispositions have been installed, so we install exactly once per process.
inline std::atomic<bool> g_handlersInstalled {false};

// The fatal signals we intercept. SIGTERM is included so an orderly kill still flushes; the rest
// are the classic hard faults. (Windows only defines a subset; the installer guards each one.)
inline const int *FatalSignals(std::size_t &count)
{
    static const int signals[] = {
#ifdef SIGSEGV
        SIGSEGV,
#endif
#ifdef SIGABRT
        SIGABRT,
#endif
#ifdef SIGFPE
        SIGFPE,
#endif
#ifdef SIGILL
        SIGILL,
#endif
#ifdef SIGBUS
        SIGBUS,
#endif
#ifdef SIGTERM
        SIGTERM,
#endif
    };
    count = sizeof(signals) / sizeof(signals[0]);
    return signals;
}

// Async-signal-safe ~1 ms pause used between drain checks: nanosleep on POSIX, a volatile
// busy-spin on Windows (which has no async-signal-safe sleep). Only async-signal-safe primitives.
inline void CrashWaitOneMs()
{
#if defined(_WIN32)
    static constexpr int kWinBusySpinIterations = 100000;
    volatile int spin = 0;
    for (int j = 0; j < kWinBusySpinIterations; ++j) {
        spin += j;
    }
#else
    static constexpr long sleepIntervalNs = 1000000L;  // 1 ms
    struct timespec ts {};
    ts.tv_sec = 0;
    ts.tv_nsec = sleepIntervalNs;
    nanosleep(&ts, nullptr);
#endif
}

// Bounded spin-wait (~2 s) for the backend thread to acknowledge a crash drain. Only touches
// atomics and CrashWaitOneMs, all async-signal-safe, so it is safe to call from a signal handler.
// NOLINTNEXTLINE
inline void CrashSpinWait()
{
    static constexpr int kMaxSpinIterations = 2000;  // 2000 × 1 ms ≈ 2 s max wait
    // Atomic with acquire order reason: observe the backend thread's flush completion before each wait
    for (int i = 0; i < kMaxSpinIterations && !g_crashDrained.load(std::memory_order_acquire); ++i) {
        CrashWaitOneMs();
    }
}

}  // namespace detail

// A structured, leveled, multi-sink logger with an asynchronous, lock-free hot path and crash-safe
// shutdown. Design mirrors mature async loggers (spdlog's async mode, Quill, zap): the calling
// thread only stamps + enqueues a fully-owned LogRecord (no formatting, no I/O, no lock on the fast
// path); a single backend thread dequeues, fans out to every sink, and flushes on a fixed cadence.
//
// Lifecycle: construct -> AddSink()* -> Start() -> Submit()* -> Stop()/destruct. Sinks must be
// added before Start(). The destructor Stops (drains + flushes + joins) so no record is lost on a
// clean exit. Copy/move are disabled -- a running backend thread and installed signal state make
// the object firmly identity-bound.
class Logger {
public:
    explicit Logger(LoggerConfig config = {}) : config_(config), level_(config.level), queue_(MakeQueue(config)) {}

    ~Logger()
    {
        Stop();
    }

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    // Registers an output destination. Call before Start(); the sink list is not synchronized and
    // is meant to be built during single-threaded setup.
    void AddSink(std::shared_ptr<LogSink> sink)
    {
        sinks_.push_back(std::move(sink));
    }

    // Starts the backend thread (async mode) and, if configured, installs crash handlers. Idempotent
    // and a no-op in sync mode (records are dispatched inline by Submit there).
    void Start()
    {
        if (config_.installCrashHandler) {
            InstallCrashHandler();
        }
        if (!config_.async) {
            return;
        }
        // Atomic with acq_rel order reason: one-shot start gate, single writer
        if (running_.exchange(true, std::memory_order_acq_rel)) {
            return;  // already started
        }
        backend_ = std::thread([this] { BackendLoop(); });
    }

    // Stops the backend thread after draining every queued record and flushing all sinks, then
    // joins. Idempotent; also called by the destructor. Clears this logger from the crash slot.
    void Stop()
    {
        // Atomic with acq_rel order reason: one-shot stop gate, single writer
        if (config_.async && running_.exchange(false, std::memory_order_acq_rel)) {
            // Atomic with release order reason: publish stop request to backend thread
            stopRequested_.store(true, std::memory_order_release);
            if (backend_.joinable()) {
                backend_.join();
            }
        }
        // Atomic with acquire order reason: read current crash-logger pointer set by InstallCrashHandler
        if (detail::g_crashLogger.load(std::memory_order_acquire) == this) {
            // Atomic with release order reason: clear crash-logger pointer for next logger
            detail::g_crashLogger.store(nullptr, std::memory_order_release);
        }
    }

    // Runtime level control. ShouldLog is the cheap relaxed-atomic gate the logging macros consult
    // before building a record, so a below-threshold call costs a single atomic load and nothing
    // else (no allocation, no timestamp).
    void SetLevel(LogLevel level)
    {
        // Atomic with relaxed order reason: advisory threshold, no happens-before needed
        level_.store(level, std::memory_order_relaxed);  // relaxed: advisory threshold, no ordering needed
    }
    LogLevel GetLevel() const
    {
        // Atomic with relaxed order reason: advisory read, no ordering guarantee
        return level_.load(std::memory_order_relaxed);  // relaxed: advisory
    }
    bool ShouldLog(LogLevel level) const
    {
        // Atomic with relaxed order reason: advisory level gate, best-effort
        return level >= level_.load(std::memory_order_relaxed);  // relaxed: advisory
    }

    // Emits a fully-built record. Stamps timestamp/thread id if the caller left them unset, then
    // either enqueues it (async) or dispatches it inline (sync). Re-checks the level so direct
    // Submit callers are gated too. This is the single funnel every logging path goes through.
    void Submit(LogRecord &&record)
    {
        if (!ShouldLog(record.level)) {
            return;
        }
        if (record.timestamp.time_since_epoch().count() == 0) {
            record.timestamp = std::chrono::system_clock::now();
        }
        if (record.threadId == 0) {
            record.threadId = CurrentThreadId();
        }

        if (!config_.async) {
            DispatchSync(record);
            return;
        }

        if (config_.overflowPolicy == OverflowPolicy::BLOCK) {
            // Lossless: spin-retry (yielding) until the backend frees a slot.
            while (!queue_->Enqueue(std::move(record))) {
                std::this_thread::yield();
            }
        } else {
            if (!queue_->Enqueue(std::move(record))) {
                // Atomic with relaxed order reason: loss counter, best-effort telemetry
                dropped_.fetch_add(1, std::memory_order_relaxed);  // relaxed: loss counter, best-effort telemetry
            }
        }
    }

    // Requests an out-of-band flush of all sinks and blocks until the backend confirms it (async),
    // or flushes inline (sync). Uses a request/ack sequence-number handshake so the caller waits
    // for a flush that began after this call, not a stale one.
    void Flush()
    {
        if (!config_.async) {
            std::lock_guard<SpinLock> guard(dispatchLock_);
            FlushSinks();
            return;
        }
        // Atomic with acq_rel order reason: serialise flush request from caller
        const std::uint64_t target = flushRequest_.fetch_add(1, std::memory_order_acq_rel) + 1;
        // Atomic with acquire order reason: observe backend thread's ack for this flush
        while (running_.load(std::memory_order_acquire) && flushAck_.load(std::memory_order_acquire) < target) {
            std::this_thread::yield();
        }
    }

    // Number of records discarded so far under OverflowPolicy::DROP (always 0 for BLOCK). Telemetry
    // to detect an undersized queue or a chronically slow sink.
    std::uint64_t DroppedCount() const
    {
        // Atomic with relaxed order reason: advisory telemetry read
        return dropped_.load(std::memory_order_relaxed);  // relaxed: advisory telemetry
    }

private:
    // Tiny non-recursive spinlock used ONLY on the sync-mode dispatch path (and sync Flush) to
    // serialize sink writes across caller threads. The async path never touches it -- there, the
    // single backend thread is the only writer, so no lock is needed at all. A spinlock (not a
    // mutex) keeps the sync path header-only and dependency-free; sync mode is not the hot path.
    class SpinLock {
    public:
        void lock()
        {
            // Atomic with acquire order reason: spinlock acquire
            while (flag_.test_and_set(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
        }
        void unlock()
        {
            // Atomic with release order reason: spinlock release
            flag_.clear(std::memory_order_release);
        }

    private:
        std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
    };

    static std::unique_ptr<MpmcBoundedQueue<LogRecord>> MakeQueue(const LoggerConfig &config)
    {
        if (!config.async) {
            return nullptr;
        }
        const std::size_t capacity = MpmcBoundedQueue<LogRecord>::RoundUpToPowerOfTwo(config.queueCapacity);
        return std::make_unique<MpmcBoundedQueue<LogRecord>>(capacity);
    }

    // Fan a single record out to every sink. Each sink applies its own level filter internally.
    void DispatchToSinks(const LogRecord &record)
    {
        for (const auto &sink : sinks_) {
            sink->Emit(record);
        }
    }

    void FlushSinks()
    {
        for (const auto &sink : sinks_) {
            sink->Flush();
        }
    }

    // Sync-mode dispatch: serialize writes so interleaved caller threads don't corrupt sink output.
    void DispatchSync(const LogRecord &record)
    {
        std::lock_guard<SpinLock> guard(dispatchLock_);
        DispatchToSinks(record);
    }

    // The backend thread body (async mode). Drains as many records as are available, flushes on the
    // configured cadence, honors out-of-band Flush() requests, and cooperates with crash handling
    // by draining + flushing immediately when a crash flag is raised. Sleeps briefly only when idle
    // so a burst is drained tightly while an empty queue doesn't busy-spin a core.
    void BackendLoop()
    {
        auto lastFlush = std::chrono::steady_clock::now();
        LogRecord record;
        for (;;) {
            bool didWork = false;
            // Drain everything currently queued before considering a sleep.
            while (queue_->Dequeue(record)) {
                DispatchToSinks(record);
                didWork = true;
            }

            // Crash requested: the faulting thread is spinning in the signal handler waiting for us.
            // Drain (already done above) + flush, acknowledge, and keep the flush durable.
            // Atomic with acquire order reason: observe crash signal set by CrashSignalHandler
            if (detail::g_crashSignal.load(std::memory_order_acquire) != 0) {
                FlushSinks();
                // Atomic with release order reason: acknowledge drain complete to signal handler
                detail::g_crashDrained.store(true, std::memory_order_release);
            }

            // Honor an explicit Flush() request via the request/ack handshake.
            // Atomic with acquire order reason: observe caller's flush request sequence number
            const std::uint64_t req = flushRequest_.load(std::memory_order_acquire);
            const auto now = std::chrono::steady_clock::now();
            const bool intervalElapsed = (now - lastFlush) >= config_.flushInterval;
            // Atomic with acquire order reason: observe last acknowledged flush sequence
            if (req > flushAck_.load(std::memory_order_acquire) || intervalElapsed) {
                FlushSinks();
                lastFlush = now;
                // Atomic with release order reason: publish flush acknowledgement to caller
                flushAck_.store(req, std::memory_order_release);
            }

            // Atomic with acquire order reason: observe stop request published by Stop()
            if (stopRequested_.load(std::memory_order_acquire)) {
                // Final drain to catch anything enqueued after the last Dequeue returned empty.
                while (queue_->Dequeue(record)) {
                    DispatchToSinks(record);
                }
                FlushSinks();
                // Atomic with acquire order reason: observe caller's flush request sequence number
                const std::uint64_t finalReq = flushRequest_.load(std::memory_order_acquire);
                // Atomic with release order reason: publish final drain ack before backend exit
                flushAck_.store(finalReq, std::memory_order_release);
                return;
            }

            if (!didWork) {
                constexpr int64_t idleSleep = 200;
                std::this_thread::sleep_for(std::chrono::microseconds(idleSleep));
            }
        }
    }

    // Installs process signal handlers exactly once, and records `this` as the logger to drain on
    // crash. Safe to call from multiple loggers; the most recently started logger becomes the crash
    // target (single-logger crash handling, as documented on the class).
    void InstallCrashHandler()
    {
        // Atomic with release order reason: publish crash-logger pointer to signal handler
        detail::g_crashLogger.store(this, std::memory_order_release);
        // Atomic with acq_rel order reason: one-shot signal-handler installation guard
        if (detail::g_handlersInstalled.exchange(true, std::memory_order_acq_rel)) {
            return;  // already installed for this process
        }
        std::size_t count = 0;
        const int *signals = detail::FatalSignals(count);
        for (std::size_t i = 0; i < count; ++i) {
            std::signal(signals[i], &Logger::CrashSignalHandler);
        }
    }

    // Async-signal-safe crash handler. Does the ABSOLUTE MINIMUM: record the signal, ask the
    // backend thread to drain+flush, spin-wait a bounded time for it to acknowledge, then restore
    // the default disposition and re-raise so the process still dies with the right signal/core.
    // No allocation, no locks, no iostream -- only atomic stores/loads, nanosleep, signal, raise.
    static void CrashSignalHandler(int signal)
    {
        // First crasher wins the drain; a re-entrant/second signal just proceeds to re-raise.
        int expected = 0;
        // Atomic with acq_rel order reason: first crasher wins the drain; re-entrant signal skips
        if (detail::g_crashSignal.compare_exchange_strong(expected, signal, std::memory_order_acq_rel)) {
            // Atomic with acquire order reason: read crash-logger pointer published by InstallCrashHandler
            if (detail::g_crashLogger.load(std::memory_order_acquire) != nullptr) {
                detail::CrashSpinWait();
            }
        }
        // Restore default handling and re-raise so the original fault/termination still occurs.
        std::signal(signal, SIG_DFL);
        std::raise(signal);
    }

    LoggerConfig config_;
    std::atomic<LogLevel> level_;
    std::unique_ptr<MpmcBoundedQueue<LogRecord>> queue_;
    std::vector<std::shared_ptr<LogSink>> sinks_;

    std::thread backend_;
    std::atomic<bool> running_ {false};
    std::atomic<bool> stopRequested_ {false};

    // Flush request/ack sequence numbers: Flush() bumps the request and waits for ack to catch up.
    std::atomic<std::uint64_t> flushRequest_ {0};
    std::atomic<std::uint64_t> flushAck_ {0};
    std::atomic<std::uint64_t> dropped_ {0};

    SpinLock dispatchLock_;
};

}  // namespace ark::es2panda::gluegen::log

#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOGGER_H
