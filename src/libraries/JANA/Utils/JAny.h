
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <utility>
#include <stdexcept>
#include <type_traits>

/// Ideally we'd just use std::any, but we are restricted to C++14 for the time being
struct JAny {
    virtual ~JAny() = default;
};

/// Ideally we'd just use std::any, but we are restricted to C++14 for the time being
template <typename T>
struct JAnyT : JAny {
    T t;
    JAnyT(T&& tt) : t(std::move(tt)) {}
    ~JAnyT() override = default; // deletes the t
};


template <typename T>
class JOptional {
private:
    using StorageT = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    bool has_value;
    StorageT storage;

public:
    JOptional() : has_value(false) {}

    JOptional(const T& val) : has_value(true) {
        new (&storage) T(val);
    }

    JOptional(T&& val) : has_value(true) {
        new (&storage) T(std::move(val));
    }

    ~JOptional() {
        reset();
    }

    // Checks if there is a value
    bool hasValue() const { return has_value; }

    // Accesses the value, throws if not present
    T& get() {
        if (!has_value) {
            throw std::runtime_error("No value present");
        }
        return *reinterpret_cast<T*>(&storage);  // Access without launder (C++14)
    }

    const T& get() const {
        if (!has_value) {
            throw std::runtime_error("No value present");
        }
        return *reinterpret_cast<const T*>(&storage);  // Access without launder (C++14)
    }

    // Resets the optional (removes the value)
    void reset() {
        if (has_value) {
            reinterpret_cast<T*>(&storage)->~T();  // Explicitly call destructor
            has_value = false;
        }
    }

    // Set the value
    void set(const T& val) {
        reset();
        new (&storage) T(val);  // Placement new
        has_value = true;
    }

    // Set using move semantics
    void set(T&& val) {
        reset();
        new (&storage) T(std::move(val));  // Placement new
        has_value = true;
    }
};


