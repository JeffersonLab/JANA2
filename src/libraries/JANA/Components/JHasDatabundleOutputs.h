
#pragma once
#include <JANA/Components/JDatabundle.h>
#include <JANA/Components/JComponentSummary.h>
#include <JANA/Utils/JEventLevel.h>

class JFactorySet;

namespace jana::components {


class JHasDatabundleOutputs {
public:
    class OutputBase {

    private:
        std::vector<JDatabundle*> databundles;

    protected:
        JEventLevel m_level = JEventLevel::None;
        bool m_is_variadic = false;

    public:
        OutputBase() = default;
        virtual ~OutputBase() = default;

        std::vector<JDatabundle*>& GetDatabundles() { return databundles; }

        JEventLevel GetLevel() const { return m_level; }
        void SetLevel(JEventLevel level) { m_level = level; }

        bool IsVariadic() const { return m_is_variadic; }


        virtual void StoreData(JFactorySet&, JDatabundle::Status) = 0;
        virtual void StoreFromProcessor(JFactorySet&, JDatabundle::Status) {};
        virtual void Reset() = 0;
        virtual void Wire(JEventLevel level, std::string databundle_name) {};
        virtual void WireVariadic(JEventLevel level, std::vector<std::string> databundle_names) {};
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

    void SummarizeOutputs(JComponentSummary::Component& summary) const {
        for (auto* output : m_databundle_outputs) {
            for (auto* databundle : output->GetDatabundles()) {
                summary.AddOutput(new JComponentSummary::Collection(databundle->GetShortName(), databundle->GetUniqueName(), databundle->GetTypeName(), output->GetLevel()));
            }
        }
    }

    void WireDatabundleOutputs(JEventLevel component_level, const std::vector<std::string>& single_output_databundle_names, const std::vector<std::vector<std::string>>& variadic_output_databundle_names) {
        size_t single_output_index = 0;
        size_t variadic_output_index = 0;

        for (auto* output : m_databundle_outputs) {
            if (output->IsVariadic()) {
                output->WireVariadic(component_level, variadic_output_databundle_names.at(variadic_output_index++));
            }
            else {
                output->Wire(component_level, single_output_databundle_names.at(single_output_index++));
            }
        }
    }
};


} // namespace jana::components


