
#include <JANA/JEvent.h>
#include <JANA/Services/JComponentManager.h>

JEvent::JEvent(JApplication* app) : mInspector(&(*this)) {
    if (app != nullptr) {
        app->GetService<JComponentManager>()->configure_event(*this);
    }
    else {
        mFactorySet = new JFactorySet();
    }
}

