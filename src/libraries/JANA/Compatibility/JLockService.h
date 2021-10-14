
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JLOCKSERVICE_H
#define JANA2_JLOCKSERVICE_H

#include <JANA/Services/JServiceLocator.h>

class JEventProcessor;

class JLockService : public JService {

public:

    JLockService() {
        pthread_rwlock_init(&m_rw_locks_lock, nullptr);
        m_app_rw_lock = CreateLock("app");
        m_root_rw_lock = CreateLock("root");
    }

    ~JLockService() override {
        delete m_app_rw_lock;
        delete m_root_rw_lock;
        for (const auto& pair : m_rw_locks) {
            delete pair.second;
        }
        for (const auto& pair : m_root_fill_rw_lock) {
            delete pair.second;
        }
    }

    inline pthread_rwlock_t *CreateLock(const std::string &name, bool throw_exception_if_exists = true);

    inline pthread_rwlock_t *ReadLock(const std::string &name);

    inline pthread_rwlock_t *WriteLock(const std::string &name);

    inline pthread_rwlock_t *Unlock(const std::string &name = std::string("app"));

    inline pthread_rwlock_t *RootReadLock() {
        pthread_rwlock_rdlock(m_root_rw_lock);
        return m_root_rw_lock;
    }

    inline pthread_rwlock_t *RootWriteLock() {
        pthread_rwlock_wrlock(m_root_rw_lock);
        return m_root_rw_lock;
    }

    inline pthread_rwlock_t *RootUnLock() {
        pthread_rwlock_unlock(m_root_rw_lock);
        return m_root_rw_lock;
    }

    inline pthread_rwlock_t *RootFillLock(JEventProcessor *proc);

    inline pthread_rwlock_t *RootFillUnLock(JEventProcessor *proc);

    pthread_rwlock_t* GetReadWriteLock(std::string &name) {
        return m_rw_locks.count( name ) == 0 ? nullptr : m_rw_locks[name];
    }
    pthread_rwlock_t* GetRootReadWriteLock() {
        return m_root_rw_lock;
    }
    pthread_rwlock_t* GetRootFillLock( JEventProcessor *proc ) {
        return m_root_fill_rw_lock.count( proc ) == 0 ? nullptr : m_root_fill_rw_lock[proc];
    }


private:

    std::map<std::string, pthread_rwlock_t *> m_rw_locks;
    pthread_rwlock_t *m_app_rw_lock;
    pthread_rwlock_t *m_root_rw_lock;
    pthread_rwlock_t m_rw_locks_lock {}; // control access to rw_locks
    std::map<JEventProcessor *, pthread_rwlock_t *> m_root_fill_rw_lock;

};

inline pthread_rwlock_t *JLockService::CreateLock(const std::string &name, bool throw_exception_if_exists) {
    // Lock the rw locks lock
    pthread_rwlock_wrlock(&m_rw_locks_lock);

    // Make sure a lock with this name does not already exist
    std::map<std::string, pthread_rwlock_t *>::iterator iter = m_rw_locks.find(name);
    pthread_rwlock_t *lock = (iter != m_rw_locks.end() ? iter->second : NULL);

    if (lock != NULL) {
        // Lock exists. Throw exception (if specified)
        if (throw_exception_if_exists) {
            pthread_rwlock_unlock(&m_rw_locks_lock);
            std::string mess = "Trying to create JANA rw lock \"" + name + "\" when it already exists!";
            throw JException(mess);
        }
    } else {
        // Lock does not exist. Create it.
        lock = new pthread_rwlock_t;
        pthread_rwlock_init(lock, NULL);
        m_rw_locks[name] = lock;
    }

    // Unlock the rw locks lock
    pthread_rwlock_unlock(&m_rw_locks_lock);

    return lock;
}

//---------------------------------
// ReadLock
//---------------------------------
inline pthread_rwlock_t *JLockService::ReadLock(const std::string &name) {
    /// Lock a global, named, rw_lock for reading. If a lock with that
    /// name does not exist, then create one and lock it for reading.
    ///
    /// This is a little tricky. Access to the map of rw locks must itself
    /// be controlled by a rw lock. This means we incure the overhead of two
    /// locks and one unlock for every call to this method. Furthermore, to
    /// keep this efficient, we want to try only read locking the map at
    /// first. If we fail to find the requested lock in the map, we must
    /// release the map's read lock and try creating the new lock.

    // Ensure the rw_locks map is not changed while we're looking at it,
    // lock the rw_locks_lock.
    pthread_rwlock_rdlock(&m_rw_locks_lock);

    // Find the lock. If it doesn't exist, set pointer to NULL
    std::map<std::string, pthread_rwlock_t *>::iterator iter = m_rw_locks.find(name);
    pthread_rwlock_t *lock = (iter != m_rw_locks.end() ? iter->second : NULL);

    // Unlock the locks lock
    pthread_rwlock_unlock(&m_rw_locks_lock);

    // If the lock doesn't exist, we need to create it. Because multiple
    // threads may be trying to do this at the same time, one may create
    // it while another waits for the locks lock. We flag the CreateLock
    // method to not throw an exception to accommodate this.
    if (lock == NULL) lock = CreateLock(name, false);

    // Finally, lock the named lock or print error message if not found
    if (lock != NULL) {
        pthread_rwlock_rdlock(lock);
    } else {
        std::string mess = "Unable to find or create lock \"" + name + "\" for reading!";
        throw JException(mess);
    }

    return lock;
}

//---------------------------------
// WriteLock
//---------------------------------
inline pthread_rwlock_t *JLockService::WriteLock(const std::string &name) {
    /// Lock a global, named, rw_lock for writing. If a lock with that
    /// name does not exist, then create one and lock it for writing.
    ///
    /// This is a little tricky. Access to the map of rw locks must itself
    /// be controlled by a rw lock. This means we incure the overhead of two
    /// locks and one unlock for every call to this method. Furthermore, to
    /// keep this efficient, we want to try only read locking the map at
    /// first. If we fail to find the requested lock in the map, we must
    /// release the map's read lock and try creating the new lock.

    // Ensure the rw_locks map is not changed while we're looking at it,
    // lock the rw_locks_lock.
    pthread_rwlock_rdlock(&m_rw_locks_lock);

    // Find the lock. If it doesn't exist, set pointer to NULL
    std::map<std::string, pthread_rwlock_t *>::iterator iter = m_rw_locks.find(name);
    pthread_rwlock_t *lock = (iter != m_rw_locks.end() ? iter->second : NULL);

    // Unlock the locks lock
    pthread_rwlock_unlock(&m_rw_locks_lock);

    // If the lock doesn't exist, we need to create it. Because multiple
    // threads may be trying to do this at the same time, one may create
    // it while another waits for the locks lock. We flag the CreateLock
    // method to not throw an exception to accommodate this.
    if (lock == NULL) lock = CreateLock(name, false);

    // Finally, lock the named lock or print error message if not found
    if (lock != NULL) {
        pthread_rwlock_wrlock(lock);
    } else {
        std::string mess = "Unable to find or create lock \"" + name + "\" for writing!";
        throw JException(mess);
    }

    return lock;
}

//---------------------------------
// Unlock
//---------------------------------
inline pthread_rwlock_t *JLockService::Unlock(const std::string &name) {
    /// Unlock a global, named rw_lock

    // To ensure the rw_locks map is not changed while we're looking at it,
    // lock the rw_locks_lock.
    pthread_rwlock_rdlock(&m_rw_locks_lock);

    // Find the lock. If it doesn't exist, set pointer to NULL
    std::map<std::string, pthread_rwlock_t *>::iterator iter = m_rw_locks.find(name);
    pthread_rwlock_t *lock = (iter != m_rw_locks.end() ? iter->second : NULL);

    // Unlock the locks lock
    pthread_rwlock_unlock(&m_rw_locks_lock);

    // Finally, unlock the named lock or print error message if not found
    if (lock != NULL) {
        pthread_rwlock_unlock(lock);
    } else {
        std::string mess = "Unable to find lock \"" + name + "\" for unlocking!";
        throw JException(mess);
    }

    return lock;
}

//---------------------------------
// RootFillLock
//---------------------------------
inline pthread_rwlock_t *JLockService::RootFillLock(JEventProcessor *proc) {
    /// Use this to lock a rwlock that is used exclusively by the given
    /// JEventProcessor. This addresses the common case where many plugins
    /// are in use and all contending for the same root lock. You should
    /// only use this when filling a histogram and not for creating. Use
    /// RootWriteLock and RootUnLock for that.

    pthread_rwlock_t *lock;

    auto iter = m_root_fill_rw_lock.find(proc);
    if (iter == m_root_fill_rw_lock.end()) {
        lock = new pthread_rwlock_t;
        pthread_rwlock_init(lock, nullptr);
        m_root_fill_rw_lock[proc] = lock;
    }
    else {
        lock = iter->second;
    }
    pthread_rwlock_wrlock(lock);
    return lock;
}

//---------------------------------
// RootFillUnLock
//---------------------------------
inline pthread_rwlock_t *JLockService::RootFillUnLock(JEventProcessor *proc) {
    /// Use this to unlock a rwlock that is used exclusively by the given
    /// JEventProcessor. This addresses the common case where many plugins
    /// are in use and all contending for the same root lock. You should
    /// only use this when filling a histogram and not for creating. Use
    /// RootWriteLock and RootUnLock for that.
    std::map<JEventProcessor *, pthread_rwlock_t *>::iterator iter = m_root_fill_rw_lock.find(proc);
    if (iter == m_root_fill_rw_lock.end()) {
        throw JException(
                "Tried calling JLockService::RootFillUnLock with something other than a registered JEventProcessor!");
    }
    pthread_rwlock_t *lock = iter->second;
    pthread_rwlock_unlock(m_root_fill_rw_lock[proc]);
    return lock;
}


#endif //JANA2_JLOCKSERVICE_H
