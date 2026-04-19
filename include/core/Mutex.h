/*
    MultiScoper - Thread-safety annotation-friendly mutex wrappers.

    Provides Clang thread-safety-analysis-capable wrappers around std::mutex
    and std::shared_mutex. Clang's thread-safety analysis (-Wthread-safety)
    only fires when the underlying type advertises the
    [[clang::capability("mutex")]] attribute and its lock/unlock methods carry
    matching acquire/release annotations; plain std::mutex and
    std::shared_mutex do not. See Abseil's absl/base/thread_annotations.h for
    the reference pattern.

    All annotations collapse to no-ops under compilers other than Clang, so
    this header is a strict superset of the std library types in terms of
    portability. Locking behavior is identical to the std library primitives.
*/

#pragma once

#include <mutex>
#include <shared_mutex>

// ============================================================================
// Annotation macros
//
// Expand to Clang thread-safety attributes under Clang, to empty otherwise.
// Use these on fields, functions, and scoped-lock types to drive
// -Wthread-safety diagnostics.
// ============================================================================

#if defined(__clang__) && (!defined(SWIG))
    #define MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(x) __attribute__((x))
#else
    #define MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(x)
#endif

// Marks a type as a capability (mutex) recognized by the analysis.
#define MULTISCOPER_CAPABILITY(x) MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(capability(x))

// Marks a field as guarded by the named capability (must be locked when accessed).
#define MULTISCOPER_GUARDED_BY(x) MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(guarded_by(x))

// Declares that a function requires exclusive ownership of listed capabilities on entry.
#define MULTISCOPER_REQUIRES(...) MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(requires_capability(__VA_ARGS__))

// Declares shared-ownership requirement on entry.
#define MULTISCOPER_REQUIRES_SHARED(...) \
    MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(requires_shared_capability(__VA_ARGS__))

// Acquire/release annotations for methods.
#define MULTISCOPER_ACQUIRE(...) MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(acquire_capability(__VA_ARGS__))
#define MULTISCOPER_RELEASE(...) MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(release_capability(__VA_ARGS__))
#define MULTISCOPER_ACQUIRE_SHARED(...) MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(acquire_shared_capability(__VA_ARGS__))
#define MULTISCOPER_RELEASE_SHARED(...) MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(release_shared_capability(__VA_ARGS__))
#define MULTISCOPER_TRY_ACQUIRE(...) MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(try_acquire_capability(__VA_ARGS__))
#define MULTISCOPER_TRY_ACQUIRE_SHARED(...) \
    MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(try_acquire_shared_capability(__VA_ARGS__))

// Function must be called without the listed capabilities held (avoids re-entry).
#define MULTISCOPER_EXCLUDES(...) MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(locks_excluded(__VA_ARGS__))

// Marks RAII scoped-lock types so acquire/release on ctor/dtor is understood.
#define MULTISCOPER_SCOPED_CAPABILITY MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(scoped_lockable)

// Disables the analysis for a given function (escape hatch — use sparingly).
#define MULTISCOPER_NO_THREAD_SAFETY_ANALYSIS MULTISCOPER_THREAD_ANNOTATION_ATTRIBUTE(no_thread_safety_analysis)

namespace multiscoper
{

/**
 * Exclusive mutex wrapper annotated as a clang thread-safety capability.
 *
 * Behavior is identical to std::mutex — this wrapper only exists so that
 * -Wthread-safety can reason about exclusive ownership of guarded members.
 */
class MULTISCOPER_CAPABILITY("mutex") Mutex
{
public:
    Mutex() noexcept = default;
    ~Mutex() = default;

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    void lock() MULTISCOPER_ACQUIRE() { m_.lock(); }
    void unlock() MULTISCOPER_RELEASE() { m_.unlock(); }
    bool try_lock() MULTISCOPER_TRY_ACQUIRE(true) { return m_.try_lock(); }

    /// Raw access for adapters that need std::unique_lock / std::scoped_lock directly.
    /// Prefer the multiscoper::ScopedLock wrapper for annotated code.
    std::mutex& native() noexcept { return m_; }

private:
    std::mutex m_;
};

/**
 * Shared (reader-writer) mutex wrapper annotated as a clang capability.
 *
 * Behavior is identical to std::shared_mutex. Exclusive and shared
 * acquire/release are annotated separately so analysis can distinguish
 * writer from reader sections.
 */
class MULTISCOPER_CAPABILITY("mutex") SharedMutex
{
public:
    SharedMutex() noexcept = default;
    ~SharedMutex() = default;

    SharedMutex(const SharedMutex&) = delete;
    SharedMutex& operator=(const SharedMutex&) = delete;
    SharedMutex(SharedMutex&&) = delete;
    SharedMutex& operator=(SharedMutex&&) = delete;

    // Exclusive (writer) operations.
    void lock() MULTISCOPER_ACQUIRE() { m_.lock(); }
    void unlock() MULTISCOPER_RELEASE() { m_.unlock(); }
    bool try_lock() MULTISCOPER_TRY_ACQUIRE(true) { return m_.try_lock(); }

    // Shared (reader) operations.
    void lock_shared() MULTISCOPER_ACQUIRE_SHARED() { m_.lock_shared(); }
    void unlock_shared() MULTISCOPER_RELEASE_SHARED() { m_.unlock_shared(); }
    bool try_lock_shared() MULTISCOPER_TRY_ACQUIRE_SHARED(true) { return m_.try_lock_shared(); }

    std::shared_mutex& native() noexcept { return m_; }

private:
    std::shared_mutex m_;
};

/**
 * RAII exclusive scoped lock annotated for thread-safety analysis.
 *
 * Template parameter accepts either multiscoper::Mutex or multiscoper::SharedMutex (for
 * writer-side locking on a reader-writer mutex).
 */
template <typename MutexT>
class MULTISCOPER_SCOPED_CAPABILITY ScopedLock
{
public:
    explicit ScopedLock(MutexT& m) MULTISCOPER_ACQUIRE(m) : m_(m) { m_.lock(); }
    ~ScopedLock() MULTISCOPER_RELEASE() { m_.unlock(); }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;
    ScopedLock(ScopedLock&&) = delete;
    ScopedLock& operator=(ScopedLock&&) = delete;

private:
    MutexT& m_;
};

/// Deduction guide: `multiscoper::ScopedLock lock(mutex_);` without angle brackets.
template <typename MutexT>
ScopedLock(MutexT&) -> ScopedLock<MutexT>;

/**
 * RAII shared (reader) scoped lock annotated for thread-safety analysis.
 *
 * Template parameter must be a shared-capable mutex (multiscoper::SharedMutex).
 */
template <typename SharedMutexT>
class MULTISCOPER_SCOPED_CAPABILITY ScopedSharedLock
{
public:
    explicit ScopedSharedLock(SharedMutexT& m) MULTISCOPER_ACQUIRE_SHARED(m) : m_(m) { m_.lock_shared(); }
    ~ScopedSharedLock() MULTISCOPER_RELEASE() { m_.unlock_shared(); }

    ScopedSharedLock(const ScopedSharedLock&) = delete;
    ScopedSharedLock& operator=(const ScopedSharedLock&) = delete;
    ScopedSharedLock(ScopedSharedLock&&) = delete;
    ScopedSharedLock& operator=(ScopedSharedLock&&) = delete;

private:
    SharedMutexT& m_;
};

/// Deduction guide: `multiscoper::ScopedSharedLock lock(mutex_);` without angle brackets.
template <typename SharedMutexT>
ScopedSharedLock(SharedMutexT&) -> ScopedSharedLock<SharedMutexT>;

} // namespace multiscoper
