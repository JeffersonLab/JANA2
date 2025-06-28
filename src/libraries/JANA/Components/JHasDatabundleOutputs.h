
#pragma once
#include <JANA/Components/JDatabundle.h>
#include <JANA/Utils/JEventLevel.h>
#include <memory>

class JFactorySet;

namespace jana::components {


class JHasDatabundleOutputs {
public:
    struct OutputBase {
    public:
        std::string type_name;
        std::vector<std::string> databundle_names;
        std::vector<JDatabundle*> databundles;
        JEventLevel level = JEventLevel::None;
        bool is_variadic = false;

        const std::vector<JDatabundle*>& GetDatabundles() { return databundles; }

        virtual void StoreData(const JFactorySet&) = 0;
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


