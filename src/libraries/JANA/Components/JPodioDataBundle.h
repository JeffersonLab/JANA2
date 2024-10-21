// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/Components/JDataBundle.h>
#include <podio/CollectionBase.h>
#include <podio/podioVersion.h>


class JPodioDataBundle : public JDataBundle {

private:
    const podio::CollectionBase* m_collection = nullptr;

public:
    size_t GetSize() const override {
        if (m_collection == nullptr) {
            return 0;
        }
        return m_collection->size();
    }

    virtual void ClearData() override {
        m_collection = nullptr;
        SetStatus(JDataBundle::Status::Empty);
        // Podio clears the data itself when the frame is destroyed.
        // Until then, the collection is immutable.
        //
        // Consider: Instead of putting the frame in its own JFactory, maybe we 
        // want to maintain a shared_ptr to the frame here, and delete the
        // the reference on ClearData(). Thus, the final call to ClearData()
        // for each events deletes the frame and actually frees the data. 
        // This would let us support multiple frames within one event, though 
        // it might also prevent the user from accessing frames directly.
    }

    const podio::CollectionBase* GetCollection() const { return m_collection; }
    void SetCollection(const podio::CollectionBase* collection) { m_collection = collection; }
};


