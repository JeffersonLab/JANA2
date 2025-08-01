
#pragma once
#include <JANA/Components/JDatabundle.h>
#include <JANA/Utils/JEventLevel.h>

class JFactorySet;

namespace jana::components {


class JHasDatabundleOutputs {
public:
    class OutputBase {

    private:
        std::vector<JDatabundle*> databundles;

    protected:
        JEventLevel level = JEventLevel::None;
        bool is_variadic = false;

    public:
        OutputBase() = default;
        virtual ~OutputBase() = default;

        std::vector<JDatabundle*>& GetDatabundles() { return databundles; }

        virtual void StoreData(JFactorySet&, JDatabundle::Status) = 0;
        virtual void StoreFromProcessor(JFactorySet&, JDatabundle::Status) {};
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


