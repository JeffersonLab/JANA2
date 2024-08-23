
#pragma once
#include <JANA/Components/JCollection.h>

namespace jana::components {

struct JHasFactoryOutputs {

private:
    std::vector<std::unique_ptr<JCollection>> m_output_collections;

public:
    const std::vector<std::unique_ptr<JCollection>>& GetOutputCollections() {
        return m_output_collections;
    }

};



} // namespace jana::components


