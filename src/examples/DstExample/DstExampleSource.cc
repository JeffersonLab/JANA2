

#include "DstExampleSource.h"

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include "DataObjects.h"

DstExampleSource::DstExampleSource(std::string resource_name, JApplication* app) : JEventSource(resource_name, app) {
    SetTypeName(NAME_OF_THIS); // Provide JANA with class name
}

void DstExampleSource::Open() {

    /// Open is called exactly once when processing begins.
    
    /// Get any configuration parameters from the JApplication
    // GetApplication()->SetDefaultParameter("DstExampleSource:random_seed", m_seed, "Random seed");

    /// For opening a file, get the filename via:
    // std::string resource_name = GetResourceName();
    /// Open the file here!
}

void DstExampleSource::GetEvent(std::shared_ptr <JEvent> event) {

    /// Calls to GetEvent are synchronized with each other, which means they can
    /// read and write state on the JEventSource without causing race conditions.
    
    /// Configure event and run numbers
    static size_t current_event_number = 1;
    event->SetEventNumber(current_event_number++);
    event->SetRunNumber(22);

    /// Limit ourselves to 1 event
    if (current_event_number > 2) {
        throw RETURN_STATUS::kNO_MORE_EVENTS;
    }

    /// Insert some canned MyJObjects
    std::vector<MyJObject*> my_jobjects;
    my_jobjects.push_back(new MyJObject(7,7,1.8));
    my_jobjects.push_back(new MyJObject(8,8,9.9));
    auto my_jobj_fac = event->Insert(my_jobjects);

    /// Enable automatic upcast to JObject
    my_jobj_fac->EnableGetAs<JObject>();

    /// Insert some canned MyRenderables
    std::vector<MyRenderable*> my_renderables;
    my_renderables.push_back(new MyRenderable(0,0,22.2));
    my_renderables.push_back(new MyRenderable(0,1,17.0));
    my_renderables.push_back(new MyRenderable(1,0,21.9));
    auto my_ren_fac = event->Insert(my_renderables);

    /// Enable automatic upcast to Renderable
    my_ren_fac->EnableGetAs<Renderable>();

    /// Insert some canned MyRenderableJObjects
    std::vector<MyRenderableJObject*> my_renderable_jobjects;
    my_renderable_jobjects.push_back(new MyRenderableJObject(1,1,1.1));
    auto my_both_fac = event->Insert(my_renderable_jobjects, "from_source");

    /// Enable automatic upcast to both JObjects and Renderable
    my_both_fac->EnableGetAs<JObject>();
    my_both_fac->EnableGetAs<Renderable>();
}

std::string DstExampleSource::GetDescription() {
    return "Dummy source for demonstrating DST example";
}


template <>
double JEventSourceGeneratorT<DstExampleSource>::CheckOpenable(std::string resource_name) {

    /// CheckOpenable() decides how confident we are that this EventSource can handle this resource.
    ///    0.0        -> 'Cannot handle'
    ///    (0.0, 1.0] -> 'Can handle, with this confidence level'
    
    /// To determine confidence level, feel free to open up the file and check for magic bytes or metadata.
    /// Returning a confidence <- {0.0, 1.0} is perfectly OK!
    
    return (resource_name == "DstExampleSource") ? 1.0 : 0.0;
}
