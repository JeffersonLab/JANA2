
#include <JANA/JApplication.h>
#include <JANA/Components/JComponentFwd.h>
#include <JANA/Components/JHasInputs.h>
#include <JANA/Components/JHasOutputs.h>
#include <JANA/Services/JWiringService.h>


void jana::components::JComponent::Wire(JApplication* app) {
    // Prefix is already set (either in the ctor or in a {Factory,EventSource}Generator
    // TODO: Support multiple event levels for Fold/Unfold/Source
    // TODO: Support short names in wiring file

    m_app = app;

    // Configure logger
    m_logger = m_app->GetService<JParameterManager>()->GetLogger(GetLoggerName());

    auto wiring_svc = m_app->GetService<services::JWiringService>();
    auto wiring = wiring_svc->GetWiringForExistingInstance(m_prefix);

    if (wiring != nullptr) {

        m_level = wiring->level;

        for (auto param : m_parameters) {
            param->Wire(wiring->configs, wiring_svc->GetSharedParameters());
        }

        if (auto* i = dynamic_cast<JHasInputs*>(this)) {
            i->WireInputs(wiring->level, wiring->input_levels, wiring->input_names,
                        wiring->variadic_input_levels, wiring->variadic_input_names);
        }

        if (auto* o = dynamic_cast<JHasOutputs*>(this)) {
            o->WireOutputs(wiring->level, wiring->output_names, wiring->variadic_output_names, false);
        }
    }
}

void jana::components::JComponent::DoInit() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_status == Status::Initialized) return;
    for (auto* parameter : m_parameters) {
        parameter->Init(*(m_app->GetJParameterManager()), m_prefix);
    }
    for (auto* service : m_services) {
        service->Fetch(m_app);
    }
    CallWithJExceptionWrapper("Init", [&](){ Init(); });

    // Don't set status until Init() succeeds, so that if an exception gets swallowed
    // once it doesn't get accidentally swallowed everywhere
    m_status = Status::Initialized;
}
