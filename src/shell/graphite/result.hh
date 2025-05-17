#pragma once

#include <string>

/* Container namespace for result types */
namespace result {

template <typename T>
struct ok {
    ok(const T& val) : val(val) {}
    ok(T&& val) : val(std::move(val)) {}

    T val;
};

template <>
struct ok<void> {};

struct err {};

}  // namespace result

namespace hidden {

/* "hidden" global error message string. (avoids storing it inside the result union) */
thread_local extern std::string error;

}

/* Function for making an Ok result */
template <typename T, typename CT = typename std::decay<T>::type>
inline result::ok<CT> Ok(T&& val) {
    return result::ok<CT>(std::forward<T>(val));
}

/* Function for making a void Ok result */
inline result::ok<void> Ok() { return result::ok<void>(); }

/* Function for making an Err result */
result::err Err(const std::string fmt, ...);
result::err Err(const char* msg);

/**
 * @brief Rust inspired `Result<T>` type for propagating errors.
 *
 * @warning This version of `Result<T>` doesn't have a configurable `Error` type.
 * Also, only 1 `Error` can exist at once on a thread, it is stored as a `static thread_local`.
 */
template <typename T>
class Result {
    /* '0' = Ok, '>0' = Err */
    uint8_t ok = 0u;
    T inner;

   public:
    Result() : ok(0u) {}
    Result(result::ok<T>&& ok) : ok(0u) { inner = std::move(ok.val); }
    Result(result::err&& err) : ok(1u) {}

    /** @return True if the result contains an error. */
    inline bool is_err() const { return ok > 0u; }
    /** @return True if the result contains a value. */
    inline bool is_ok() const { return !is_err(); }

    /**
     * @brief Boolean operator.
     * @return True if this result is ok.
     */
    inline explicit operator bool() const { return is_ok(); }

    /**
     * @brief <UNSAFE> Way to extract value from result.
     * (Will terminate the program if the result is an error)
     */
    inline T unwrap(bool noexpect = false) const {
        if (noexpect == false && is_err()) {
            std::fprintf(stderr, "unwrap failed!\nreason: %s\n", hidden::error.c_str());
            std::terminate();
        }
        return inner;
    }

    /**
     * @brief <UNSAFE> Way to extract value from result.
     * (Will terminate the program if the result is an error)
     */
    inline T expect(std::string_view msg, bool noexpect = false) const {
        if (noexpect == false && is_err()) {
            std::fprintf(stderr, "error: %s\nreason: %s\n", msg.data(), hidden::error.c_str());
            std::terminate();
        }
        return inner;
    }

    /**
     * @brief Way to extract value from result.
     * (Returns the default value if the result is an error)
     */
    inline T unwrap_or_default() const {
        if (is_err()) return T{};
        return inner;
    }

    /**
     * @brief <UNSAFE> Way to extract error from result.
     * (Will terminate the program if the result is not an error)
     */
    inline const std::string& unwrap_err() const {
        if (is_ok()) {
            std::fprintf(stderr, "unwrap_err failed!\n");
            std::terminate();
        }
        return hidden::error;
    }
};

/**
 * @brief Rust inspired `Result<void>` type for propagating errors.
 *
 * @warning This version of `Result<void>` doesn't have a configurable `Error` type.
 * Also, only 1 `Error` can exist at once on a thread, it is stored as a `static thread_local`.
 */
template <>
class Result<void> {
    /* '0' = Ok, '>0' = Err */
    uint8_t ok = 0u;

   public:
    Result() : ok(0u) {}
    Result(result::ok<void>&& ok) : ok(0u) {}
    Result(result::err&& err) : ok(1u) {}

    /** @return True if the result contains an error. */
    inline bool is_err() const { return ok > 0u; }
    /** @return True if the result contains a value. */
    inline bool is_ok() const { return !is_err(); }

    /**
     * @brief Boolean operator.
     * @return True if this result is ok.
     */
    inline explicit operator bool() const { return is_ok(); }

    /**
     * @brief <UNSAFE> Way to extract value from result.
     * (Will terminate the program if the result is an error)
     */
    inline void unwrap() const {
        if (is_err()) {
            std::fprintf(stderr, "unwrap failed!\nreason: %s\n", hidden::error.c_str());
            std::terminate();
        }
    }

    /**
     * @brief <UNSAFE> Way to extract value from result.
     * (Will terminate the program if the result is an error)
     */
    inline void expect(std::string_view msg) const {
        if (is_err()) {
            std::fprintf(stderr, "error: %s\nreason: %s\n", msg.data(), hidden::error.c_str());
            std::terminate();
        }
    }

    /**
     * @brief <UNSAFE> Way to extract error from result.
     * (Will terminate the program if the result is not an error)
     */
    inline const std::string& unwrap_err() const {
        if (is_ok()) {
            std::fprintf(stderr, "unwrap_err failed!\n");
            std::terminate();
        }
        return hidden::error;
    }
};
