
#pragma once
#include <JANA/Components/JDatabundle.h>
#include <memory>

class JEvent;

namespace jana::components {


class JHasDatabundleOutputs {
public:
    struct OutputBase {
    protected:
        std::vector<std::unique_ptr<JDatabundle>> m_databundles;
        bool m_is_variadic = false;
    public:
        const std::vector<std::unique_ptr<JDatabundle>>& GetDatabundles() const { return m_databundles; }
        virtual void StoreData(const JEvent& event) = 0;
        virtual void Reset() = 0;
    };

private:
    std::vector<OutputBase*> m_databundle_outputs;

public:
    const std::vector<OutputBase*>& GetDatabundleOutputs() const {
        return m_databundle_outputs;
    }

    void RegisterOutput(OutputBase* output) {
        m_databundle_outputs.push_back(output);
    }
};


} // namespace jana::components


