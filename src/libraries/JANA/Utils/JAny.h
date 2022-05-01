
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JANY_H
#define JANA2_JANY_H

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

#endif //JANA2_JANY_H
