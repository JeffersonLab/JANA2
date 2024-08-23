
#pragma once
#include <JANA/Components/JCollection.h>
#include <memory>

namespace jana::components {

struct JHasFactoryOutputs {

private:
    std::vector<std::unique_ptr<JCollection>> m_output_collections;

public:
    const std::vector<std::unique_ptr<JCollection>>& GetOutputCollections() {
        return m_output_collections;
    }

    void RegisterCollection(std::unique_ptr<JCollection>&& coll) {
        m_output_collections.push_back(std::move(coll));
    }

};



} // namespace jana::components


