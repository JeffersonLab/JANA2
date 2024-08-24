
#pragma once
#include <JANA/Components/JCollection.h>
#include <memory>

class JEvent;

namespace jana::components {


class JHasFactoryOutputs {
public:
    struct OutputBase {
    protected:
        std::vector<std::string> m_collection_names; // TODO: Possibly don't need anymore
        std::vector<std::unique_ptr<JCollection>> m_collections;
        bool m_is_variadic = false;
    public:
        const std::vector<std::unique_ptr<JCollection>>& GetCollections() const { return m_collections;}
        virtual void PutCollections(const JEvent& event) = 0;
        virtual void Reset() = 0;
    };

private:
    std::vector<OutputBase*> m_outputs;

public:
    const std::vector<OutputBase*>& GetOutputs() {
        return m_outputs;
    }

    void RegisterOutput(OutputBase* output) {
        m_outputs.push_back(output);
    }
};



} // namespace jana::components


