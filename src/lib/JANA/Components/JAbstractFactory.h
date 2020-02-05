//
// Created by Nathan Brei on 2/4/20.
//

#ifndef JANA2_JABSTRACTFACTORY_H
#define JANA2_JABSTRACTFACTORY_H

#include <vector>

#include <JANA/JFactory.h>
#include <JANA/Utils/JTypeInfo.h>


class JApplication;

/// Class template for metadata. This constrains JAbstractFactoryT<T> to use the same (user-defined)
/// metadata structure, JMetadata<T> for that T. This is essential for retrieving metadata from
/// JAbstractFactoryT's without breaking the Liskov substitution property.
template<typename T>
struct JMetadata {};

template<typename T>
class JAbstractFactoryT : virtual public JFactory {
public:

    using IteratorType = typename std::vector<T*>::const_iterator;
    using PairType = std::pair<IteratorType, IteratorType>;

    explicit JAbstractFactoryT(const std::string& aName = JTypeInfo::demangle<T>(), const std::string& aTag = "")
            : JFactory(aName, aTag) {}

    ~JAbstractFactoryT() override = default;

    std::type_index GetObjectType() const override {
        return std::type_index(typeid(T));
    }

    /// GetOrCreate handles all the preconditions and postconditions involved in calling the user-defined Open(),
    /// ChangeRun(), and Process() methods. These include making sure the JFactory JApplication is set, Init() is called
    /// exactly once, exceptions are tagged with the originating plugin and eventsource, ChangeRun() is
    /// called if and only if the run number changes, etc.
    virtual PairType GetOrCreate(const std::shared_ptr<const JEvent>& event, JApplication* app, uint64_t run_number) = 0;

    void ClearData() override = 0;


    /// Set the JFactory's metadata. This is meant to be called by user during their JAbstractFactoryT::Process
    /// Metadata will *not* be cleared on ClearData(), but will be destroyed when the JAbstractFactoryT is.
    void SetMetadata(JMetadata<T> metadata) { mMetadata = metadata; }

    /// Get the JFactory's metadata. This is meant to be called by user during their JAbstractFactoryT::Process
    /// and also used by JEvent under the hood.
    /// Metadata will *not* be cleared on ClearData(), but will be destroyed when the JAbstractFactoryT is.
    JMetadata<T> GetMetadata() { return mMetadata; }


protected:
    JMetadata<T> mMetadata;
};



#endif //JANA2_JABSTRACTFACTORY_H
