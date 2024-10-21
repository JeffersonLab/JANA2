
#pragma once
#include <JANA/Components/JDataBundle.h>
#include <memory>

class JEvent;

namespace jana::components {


class JHasFactoryOutputs {
public:
    struct OutputBase {
    protected:
        std::vector<std::unique_ptr<JDataBundle>> m_databundles;
        bool m_is_variadic = false;
    public:
        const std::vector<std::unique_ptr<JDataBundle>>& GetDataBundles() const { return m_databundles; }
        virtual void StoreData(const JEvent& event) = 0;
        virtual void Reset() = 0;
    };

private:
    std::vector<OutputBase*> m_outputs;

public:
    const std::vector<OutputBase*>& GetOutputs() const {
        return m_outputs;
    }

    void RegisterOutput(OutputBase* output) {
        m_outputs.push_back(output);
    }
};



} // namespace jana::components


