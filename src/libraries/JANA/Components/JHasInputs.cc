
#include "JHasInputs.h"

#include <JANA/JEvent.h>
#include <JANA/Utils/JEventLevel.h>

namespace jana::components {

JFactorySet* GetFactorySetAtLevel(const JEvent& event, JEventLevel desired_level) {

    if (desired_level == JEventLevel::None || desired_level == event.GetLevel()) {
        return event.GetFactorySet();
    }
    return event.GetParent(desired_level).GetFactorySet();
}

}
