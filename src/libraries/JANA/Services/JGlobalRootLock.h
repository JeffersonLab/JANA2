
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef BDXRECO_JGLOBALROOTLOCK_H
#define BDXRECO_JGLOBALROOTLOCK_H

#include "JServiceLocator.h"


class JGlobalRootLock : public JService {

public:

    JGlobalRootLock() {
        m_root_rw_lock = new pthread_rwlock_t;
        pthread_rwlock_init(m_root_rw_lock, nullptr);
    }

    inline void acquire_read_lock() { pthread_rwlock_rdlock(m_root_rw_lock); }

    inline void acquire_write_lock() { pthread_rwlock_wrlock(m_root_rw_lock); }

    inline void release_lock() { pthread_rwlock_unlock(m_root_rw_lock); }

private:

    pthread_rwlock_t* m_root_rw_lock;

};

#endif //BDXRECO_JGLOBALROOTLOCK_H



