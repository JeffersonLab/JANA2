#include "JHasOutputs.h"
#include <JANA/JFactory.h>

void jana::components::UpdateFactoryStatusOnEulerianStore(JFactory* fac) {
    // We need to set the factory status separately from the databundle status so 
    // that the factory doesn't accidentally get re-run.
    // We do this inside a weird little free function because we need to avoid creating
    // a circular definition of JFactory in our templates.
    // Eventually we will need to refactor JFactory::Status and CreationStatus.

    fac->SetStatus(JFactory::Status::Inserted);
    fac->SetCreationStatus(JFactory::CreationStatus::Inserted);
}


