
#pragma once
#include <JANA/Components/JDatabundle.h>
#include <JANA/Components/JComponentSummary.h>
#include <JANA/Utils/JEventLevel.h>

class JFactorySet;

namespace jana::components {


class JHasOutputs {
public:

    class OutputBase {
    private:
        JDatabundle* m_databundle;
        JEventLevel m_level = JEventLevel::None; // By default we inherit this from the component that owns this

    public:
        virtual ~OutputBase() {
            delete m_databundle;
        }
        JDatabundle* GetDatabundle() { return m_databundle; }
        JEventLevel GetLevel() const { return m_level; }

        void SetDatabundle(JDatabundle* databundle) { m_databundle = databundle; }
        void SetLevel(JEventLevel level) { m_level = level; }
        void SetShortName(std::string short_name) { m_databundle->SetShortName(short_name); }
        void SetUniqueName(std::string unique_name) { m_databundle->SetUniqueName(unique_name); }

        virtual void LagrangianStore(JFactorySet&, JDatabundle::Status) {}
        virtual void EulerianStore(JFactorySet&) {}

        void ClearData() { m_databundle->ClearData(); }
    };


    class VariadicOutputBase {
    private:
        std::vector<JDatabundle*> m_databundles;
        JEventLevel m_level = JEventLevel::PhysicsEvent;

    public:
        virtual ~VariadicOutputBase() {
            for (auto* db : m_databundles) {
                delete db;
            }
        }
        std::vector<JDatabundle*>& GetDatabundles() { return m_databundles; }
        JEventLevel GetLevel() const { return m_level; }

        void SetLevel(JEventLevel level) { m_level = level; }
        virtual void SetShortNames(std::vector<std::string>) {}
        virtual void SetUniqueNames(std::vector<std::string>) {}

        std::vector<std::string> GetUniqueNames() {
            std::vector<std::string> results;
            for (auto* databundle: m_databundles) {
                results.push_back(databundle->GetUniqueName());
            }
            return results;
        };

        virtual void LagrangianStore(JFactorySet&, JDatabundle::Status) {}
        virtual void EulerianStore(JFactorySet&) {}

        void ClearData() {
            for (auto* databundle : m_databundles) {
                databundle->ClearData();
            }
        }
    };

private:
    std::vector<OutputBase*> m_outputs;;
    std::vector<VariadicOutputBase*> m_variadic_outputs;
    std::vector<std::pair<OutputBase*, VariadicOutputBase*>> m_ordered_outputs;

public:

    const std::vector<OutputBase*>& GetOutputs() const {
        return m_outputs;
    }

    const std::vector<VariadicOutputBase*>& GetVariadicOutputs() const {
        return m_variadic_outputs;
    }

    JDatabundle* GetFirstDatabundle() const {
        if (m_outputs.size() > 0) {
            return m_outputs.at(0)->GetDatabundle();
        }
        return m_variadic_outputs.at(0)->GetDatabundles().at(0);
        // TODO: This will except if our first databundle is an empty variadic one
    }

    void RegisterOutput(OutputBase* output) {
        m_outputs.push_back(output);
        m_ordered_outputs.push_back({output, nullptr});
    }

    void RegisterOutput(VariadicOutputBase* output) {
        m_variadic_outputs.push_back(output);
        m_ordered_outputs.push_back({nullptr, output});
    }

    void SummarizeOutputs(JComponentSummary::Component& summary) const {

        for (auto* output : m_outputs) {
            auto* databundle = output->GetDatabundle();
            summary.AddOutput(new JComponentSummary::Collection(databundle->GetShortName(), 
                                                                  databundle->GetUniqueName(), 
                                                             databundle->GetTypeName(), 
                                                                 output->GetLevel()));
        }

        for (auto* output : m_variadic_outputs) {
            for (auto* databundle : output->GetDatabundles()) {
                summary.AddOutput(new JComponentSummary::Collection(databundle->GetShortName(), 
                                                                      databundle->GetUniqueName(), 
                                                                 databundle->GetTypeName(), 
                                                                     output->GetLevel()));
            }
        }
    }

    void WireOutputs(JEventLevel component_level,
                               const std::vector<std::string>& single_output_databundle_names,
                               const std::vector<std::vector<std::string>>& variadic_output_databundle_names,
                               bool use_short_names) {

        if (m_variadic_outputs.size() == 1 && variadic_output_databundle_names.size() == 0) {
            // Obtain variadic databundle names from excess single-output databundle names
            int variadic_databundle_count = single_output_databundle_names.size() - m_outputs.size();
            int current_databundle_index = 0;

            for (auto& pair : m_ordered_outputs) {
                auto* single_output = pair.first;
                auto* variadic_output = pair.second;

                if (variadic_output != nullptr) {

                    variadic_output->SetLevel(component_level);
                    std::vector<std::string> variadic_names;
                    for (int i=0; i<variadic_databundle_count; ++i) {
                        variadic_names.push_back(single_output_databundle_names.at(current_databundle_index+i));
                    }
                    if (use_short_names) {
                        variadic_output->SetShortNames(variadic_names);
                    } else {
                        variadic_output->SetUniqueNames(variadic_names);
                    }
                    current_databundle_index += variadic_databundle_count;
                }
                else {
                    single_output->SetLevel(component_level);
                    if (use_short_names) {
                        single_output->SetShortName(single_output_databundle_names.at(current_databundle_index));
                    }
                    else {
                        single_output->SetUniqueName(single_output_databundle_names.at(current_databundle_index));
                    }
                    current_databundle_index += 1;
                }
            }
        }
        else {
            // Do the obvious, sensible thing instead
            if (!single_output_databundle_names.empty()) {
                if (single_output_databundle_names.size() != m_outputs.size()) {
                    throw JException("Wrong number of (nonvariadic) output databundle names! Expected %d, found %d", m_outputs.size(), single_output_databundle_names.size());
                }

                size_t i = 0;
                for (auto* output : m_outputs) {
                    output->SetLevel(component_level);
                    if (use_short_names) {
                        output->SetShortName(single_output_databundle_names.at(i));
                    }
                    else {
                        output->SetUniqueName(single_output_databundle_names.at(i));
                    }
                    i += 1;
                }
            }

            if (!variadic_output_databundle_names.empty()) {
                if (variadic_output_databundle_names.size() != m_variadic_outputs.size()) {
                    throw JException("Wrong number of lists of variadic output databundle names! Expected %d, found %d", m_variadic_outputs.size(), variadic_output_databundle_names.size());
                }
                size_t i = 0;
                for (auto* variadic_output : m_variadic_outputs) {
                    variadic_output->SetLevel(component_level);
                    if (use_short_names) {
                        variadic_output->SetShortNames(variadic_output_databundle_names.at(i));
                    }
                    else {
                        variadic_output->SetUniqueNames(variadic_output_databundle_names.at(i));
                    }
                    i += 1;
                }
            }
        }
    }
};

} // namespace jana::components

