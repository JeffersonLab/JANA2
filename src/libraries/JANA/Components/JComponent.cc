
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
    auto wiring = wiring_svc->GetWiring(m_prefix);
    bool use_short_names = wiring_svc->UseShortNames();

    if (wiring != nullptr) {
        wiring->is_used = true;

        if (wiring->action == services::JWiringService::Action::Remove) {
            m_is_enabled = false;
        }
        else {
            m_is_enabled = true;
            // The user can provide a component which is disabled by default,
            // in which case the wiring file is the only way to enable it.
        }

        m_level = wiring->level;

        for (auto param : m_parameters) {
            param->Wire(wiring->configs, wiring_svc->GetSharedParameters());
        }

        if (auto* i = dynamic_cast<JHasInputs*>(this)) {
            i->WireInputs(wiring->level, wiring->input_levels, wiring->input_names,
                        wiring->variadic_input_levels, wiring->variadic_input_names);
        }

        if (auto* o = dynamic_cast<JHasOutputs*>(this)) {
            o->WireOutputs(wiring->level, wiring->output_names, wiring->variadic_output_names, use_short_names);
        }
    }
}

void jana::components::JComponent::DoInit() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_is_initialized) return;
    for (auto* parameter : m_parameters) {
        parameter->Init(*(m_app->GetJParameterManager()), m_prefix);
    }
    for (auto* service : m_services) {
        service->Fetch(m_app);
    }
    CallWithJExceptionWrapper("Init", [&](){ Init(); });

    // Don't set status until Init() succeeds, so that if an exception gets swallowed
    // once it doesn't get accidentally swallowed everywhere
    m_is_initialized = true;
}
