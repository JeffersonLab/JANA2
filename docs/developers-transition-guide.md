
****
# Developers Transition Guide: Key Syntax Changes between JANA1 to JANA2

This guide outlines the critical syntax and functional updates between JANA1 and JANA2, helping developers navigate the transition smoothly.

## Commonly Used Features
### JANA Namespace Handling
##### **JANA1**
Adding following on top of all files was necessary in JANA1:

```
using namespace jana;
```
##### **JANA2**
This line is no longer required in JANA2.
### Application Handling
#### Getting the JApplication
##### **JANA1**
In JANA1 there is a global variable `japp` which is a pointer to the project's `JApplication`. You can also obtain it from `JEventLoop::GetJApplication()`.
##### **JANA2**
In JANA2, `japp` is still around but we are strongly discouraging its future use. If you are within any JANA component (for instance: JFactory, JEventProcessor, JEventSource, JService) you can obtain the `JApplication` pointer from `this`, like so:
```
auto app = GetApplication();
```

You can also obtain it from the `JEvent` the same way as you used to from `JEventLoop`, i.e. 
```c++
auto app = event->GetJApplication();
```
### Parameter Management
#### Setting Parameters
##### **JANA1**
```
gPARMS->GetParameter("OUTPUT_FILENAME", dOutputFileName);
```
##### **JANA2**
You should obtain parameters as shown below. 

```
auto app = GetApplication();
app->SetDefaultParameter("component_prefix:value_name", ref_to_member_var);
```

We strongly recommend you register all parameters from inside the `Init()` callbacks of your JANA components, or from `InitPlugin()`. This helps JANA:

- Report back which parameters are available without having to process an input file.
- Emit a warning (or error!) when a user-provided parameter is misspelled
- Emit a warning when two plugins register the same parameter with conflicting default values.
- Inspect the exact parameter values used by a factory

If you register parameters in other contexts, this machinery might not work and you might get incomplete parameter lists and missing or spurious error messages. Worse, your code might not see the parameter values you expect it to see. Registering parameters from inside component constructors is less problematic but still discouraged because it won't work with some upcoming new features.
#### Getting Parameter Maps
##### **JANA1**
To obtain a map of all parameters that share a common prefix:

```
//gets all parameters with this filter at the beginning of the key
gPARMS->GetParameters(locParameterMap, "COMBO_DEDXCUT:"); 
```
##### **JANA2**
In JANA2 this has been renamed to `FilterParameters` but the functionality is the same.
Note that this method is not on `JApplication` directly; you have to obtain the parameter manager like so:

```
//gets all parameters with this filter at the beginning of the key
GetApplication()->GetJParameterManager()->FilterParameters(locParameterMap, "COMBO_DEDXCUT:");
```
### DApplication Handling
#### Getting DApplication
##### **JANA1**
```
#include "DANA/DApplication.h"
dApplication = dynamic_cast<DApplication*>(locEventLoop->GetJApplication
```
##### **JANA2**
There is no `DApplication` anymore in JANA2. `JApplication` is `final`, which means you can't inherit from it. The functionality on `DApplication` has been moved into `JService`s, which can be accessed via `JApplication::GetService<ServiceT>()`. 
### DGeometry Handling
#### Getting DGeometry
##### **JANA1**
###### Using japp
```
#include "DANA/DApplication.h"
#include "HDGEOMETRY/DGeometry.h"

DApplication* dapp=dynamic_cast<DApplication*>(japp);
const DGeometry *dgeom = dapp->GetDGeometry(runnumber);
```
###### Using eventLoop
```
#include "DANA/DApplication.h"
#include "HDGEOMETRY/DGeometry.h"

DGeometry *locGeom = dApplication ? dApplication->GetDGeometry(eventLoop->GetJEvent().GetRunNumber()) : NULL;
```
##### **JANA2**
In JANA2, you obtain the `DGeometryManager` from the `JApplication` and from there you use the run number to obtain the corresponding `DGeometry`:
```
#include "HDGEOMETRY/DGeometry.h"

auto runnumber = event->GetRunNumber();
auto app = event->GetJApplication();
auto geo_manager = app->GetService<DGeometryManager>();
auto geom = geo_manager->GetDGeometry(runnumber);
```

To reduce the boilerplate, we've provided a handy helper function, `DEvent::GetDGeometry()`, that does all of these steps for you:

```
#include "DANA/DEvent.h"

DGeometry *locGeometry = DEvent::GetDGeometry(locEvent);
```

You should call `GetDGeometry` from `JFactory::BeginRun` or `JEventProcessor::BeginRun`.
### Calibration Handling
#### Accessing Calibration Data - `GetCalib` 
##### **JANA1**
```
locEventLoop->GetCalib(locTOFParmsTable.c_str(), tofparms);
```
##### **JANA2**
Analogously to `DGeometry`, you obtain the calibrations by obtaining the `JCalibrationManager` from `JApplication::GetService<>()` and then loading the calibration object corresponding to the event's run number. We recommend you use `DEvent::GetCalib` to avoid this boilerplate. You should call this from your `BeginRun` callback. 

```
#include "DANA/DEvent.h"

DEvent::GetCalib(event, locTOFParmsTable.c_str(), tofparms);
```

In principle, the calibrations could also be keyed off of event number intervals, in which case you should call it from `Process` and pass the event number as well:

```
#include <JANA/JEvent.h>
#include <JANA/Calibrations/JCalibrationManager.h>

auto event_number = event->GetEventNumber();
auto run_number = event->GetRunNumber();

auto app = GetApplication();
auto calibration = app->GetService<JCalibrationManager>()->GetJCalibration(run_number);
calibration->Get("BCAL/mc_parms", bcalparms, event_number)


```
### Status Bit Handling
#### Accessing Status Bit - `GetStatusBit`
##### **JANA1**
```
locEventLoop->GetJEvent().GetStatusBit(kSTATUS_PHYSICS_EVENT)
```
##### **JANA2**
Status bits are no longer an event member variable. Instead, they are inserted and retrieved just like the other collections:

```
#include "DANA/DStatusBits.h"


locEvent->GetSingle<DStatusBits>()->GetStatusBit(kSTATUS_PHYSICS_EVENT)

// or, for a little bit more safety:

locEvent->GetSingleStrict<DStatusBits>()->GetStatusBit(kSTATUS_REST);
```
### Magnetic Field Handling
#### Getting Magnetic Field - `GetBField`
##### **JANA1**
```
#include "DANA/DApplication.h"

dApplication = dynamic_cast<DApplication*>(locEventLoop->GetJApplication());
dMagFMap= dApplication->GetBfield(locEventLoop->GetJEvent().GetRunNumber());

```
##### **JANA2**
In JANA2, the magnetic field map is a resource handled analogously to `DGeometry` and `JCalibrationManager`. Thus, we recommend obtaining the 
field map by calling `DEvent::GetBField(event)` from inside `BeginRun`:

```
#include "DANA/DEvent.h"

dMagneticFieldMap = DEvent::GetBfield(locEvent);
```

## Updated Get Functions
### JObject Getters
#### 1. `GetSingleT`
##### **JANA1**
```
Was present in JANA1
```
##### **JANA2**
```
No longer available in JANA2; use GetSingle() instead
```
#### 2. `GetSingle`
##### **JANA1**
```
// Pass by reference
// Set passed param equal to pointer to object
const Df250PulsePedestal* PPobj = NULL;
digihit->GetSingle(PPobj);
```
##### **JANA2**
```
// Template function
// Return pointer to object
auto PPobj = digihit->GetSingle<Df250PulsePedestal>();
```
#### 3. `Get`
##### **JANA1**
```
// Pass by Reference
vector<const DBCALUnifiedHit*> assoc_hits;
point.Get(assoc_hits);
```
##### **JANA2**
```
// Templated Function
// Return value
vector<const DBCALUnifiedHit*> assoc_hits = point.Get<DBCALUnifiedHit>();
```
### JEvent Getters
#### 1. `GetSingle`
##### **JANA1**
```
// Pass by Reference, 
// passed param got assigned pointer to object

const DTTabUtilities* locTTabUtilities = NULL;
loop->GetSingle(locTTabUtilities); 
```
##### **JANA2**
```
// Return the pointer to object based on template type

const DTTabUtilities* locTTabUtilities = event->GetSingle<DTTabUtilities>();
```
#### 2. `Get`
##### **JANA1**
```
// Pass by Reference
vector<const DBCALShower*> showers;
loop->Get(showers, "IU");
```
##### **JANA2**
```
// JANA1 Get is still available but will be deprecated soon
// So better to use the one given below

// Templated function
// Return value
auto showers = event->Get<DBCALShower>("IU");
```

## JEventProcessor
### JEventProcessor Header File
##### JANA1
```
#ifndef _JEventProcessor_myplugin_
#define _JEventProcessor_myplugin_

#include <JANA/JApplication.h>
#include <JANA/JFactory.h>
#include <JANA/JEventProcessor.h>
using namespace jana;
  

class JEventProcessor_myplugin : public jana::JEventProcessor{
public:
	JEventProcessor_myplugin();
	~JEventProcessor_myplugin();
	const char* className(void){return "JEventProcessor_myplugin";}

private:
	jerror_t init(void); 
	jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber); 
	jerror_t evnt(jana::JEventLoop *eventLoop, uint64_t eventnumber); 
	jerror_t erun(void); 
	jerror_t fini(void); 

};

#endif // _JEventProcessor_myplugin_
```
##### JANA2
```
#ifndef _JEventProcessor_myplugin_
#define _JEventProcessor_myplugin_
#include <JANA/JEventProcessor.h>

class JEventProcessor_myplugin : public JEventProcessor {

public:
	JEventProcessor_myplugin() { 
        SetTypeName(NAME_OF_THIS);
    }
	~JEventProcessor_myplugin() = default;

private:
	void Init() override;
	void BeginRun(const std::shared_ptr<const JEvent>& event) override; 
	void Process(const std::shared_ptr<const JEvent>& event) override; 
	void EndRun() override; 
	void Finish() override; 
};

#endif // _JEventProcessor_myplugin_
```

## JObject
### JObject Header File
##### JANA1
```
#include <JANA/JObject.h>
#include <JANA/JFactory.h>

using namespace jana;

class DCereHit: public JObject {
public:

    JOBJECT_PUBLIC(DCereHit);
    int sector; 
    float pe; 
    float t;

    void toStrings(vector<pair<string, string> >&items) const { 
        AddString(items, "sector", "%d", sector);
        AddString(items, "pe", "%1.3f", pe);
        AddString(items, "t", "%1.3f", t);
    }
};
```
##### JANA2
```
#include <JANA/JObject.h>

class DCereHit: public JObject {
public:

    JOBJECT_PUBLIC(DCereHit);
    int sector;
    float pe;
    float t;

    void Summarize(JObjectSummary& summary) const override {
        summary.add(sector, "sector", "%d");
        summary.add(pe, "pe", "%1.3f");
        summary.add(t, "t", "%1.3f");
    }
};
```
### JObject ID Handling
##### **JANA1**
```
// JANA1 JObject had a data member id

DBCALShower *shower = new DBCALShower;
shower->id = id++;
```
##### **JANA2**
JANA2 no longer provides a built-in object id. However, you can always add one as a field to your `JObject` and set it yourself.

```
// id data member does not exist in JObject anymore in JANA2

DBCALShower *shower = new DBCALShower;
shower->id = id++; // Throw Error: no id member
```

Similarly, the `JObject::oid_t` type has been removed from JANA2 as well. If you need it, it lives on in the halld_recon codebase:

```
#include "DANA/DObjectID.h"

oid_t // Not inside JObject anymore!
```

## JFactory
### Transition from JFactory to JFactoryT
##### **JANA1**
In JANA1, `JFactory_base` was a common factory base class, and `JFactory` was a template that inherits from `JFactory_base` and adds functionality specific to the type of the factory's output collection.
##### **JANA2**
In JANA2, the base class was renamed to `JFactory`, and the template was renamed to `JFactoryT`. JANA2 adds some new functionality, but the old functionality is largely consistent with JANA1. One important difference is that in JANA1, there is always one factory set assigned to each thread, whereas in JANA2, there is one factory set for each event in flight. The number of in-flight events is a parameter controlled separately from the thread count.
### Factory Header File
##### JANA1
```
#ifndef _myfactory_factory_
#define _myfactory_factory_

#include <JANA/JFactory.h>
#include "myobject.h"

class myfactory_factory : public jana::JFactory<myobject> {

public:
    myfactory_factory() = default;
    ~myfactory_factory() = default;

private:
    jerror_t init(void); 
    jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber); 
    jerror_t evnt(jana::JEventLoop *eventLoop, uint64_t eventnumber); 
    jerror_t erun(void); 
    jerror_t fini(void); 
};

#endif
```
##### JANA2
```
#ifndef _thirdFacTest_factory_
#define _thirdFacTest_factory_

#include <JANA/JFactoryT.h>
#include "myobject.h"

class thirdFacTest_factory : public JFactoryT<myobject> {
public:
    thirdFacTest_factory() { 
        SetTag("MyTag");
    }
    ~thirdFacTest_factory() = default;

private:
    void Init() override;
    void BeginRun(const std::shared_ptr<const JEvent>& event) override; 
    void Process(const std::shared_ptr<const JEvent>& event) override; 
    void EndRun() override; 
    void Finish() override; 
};

#endif
```
### Registering a New Factory
This section require some explanation like how factory was getting added before and how it is now with complete explanation
##### **JANA1**
###### Using EventLoop
```
jerror_t BCAL_init(JEventLoop *loop)
{
/// Create and register BCAL data factories
loop->AddFactory(new JFactory<DBCALDigiHit>());
loop->AddFactory(new JFactory<DBCALTDCDigiHit>());
loop->AddFactory(new DBCALHit_factory());

}
```
###### Using Factory Generator
```
#include "myfactory_factory.h"
#include "JFactoryGenerator_myfactory.h"
#include <JANA/JApplication.h>
using namespace jana;



extern "C"{
	void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddFactoryGenerator(new JFactoryGenerator_myfactory());
	}
} // "C"
```
##### **JANA2**
###### Using Factory Set
```
#include <JANA/JFactorySet.h>
#include <JANA/Compatibility/JGetObjectsFactory.h>

void BCAL_init(JFactorySet *factorySet)

{

/// Create and register BCAL data factories
factorySet->Add(new JGetObjectsFactory<DBCALDigiHit>());
factorySet->Add(new JGetObjectsFactory<DBCALTDCDigiHit>());
factorySet->Add(new DBCALHit_factory());

}
```
###### Using Factory Generator
```
#include "thirdFacTest_factory.h"
#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

extern "C"{
	void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->Add(new JFactoryGeneratorT<thirdFacTest_factory>());
	}
} // "C"
```
### Getting All Factories
##### **JANA1**
```
vector<JFactory_base*> locFactories = locEventLoop->GetFactories();
//JFactory_base to particular factory type
JFactory<DReaction>* locFactory = dynamic_cast<JFactory<DReaction>*>(locFactories[0]);
```
##### **JANA2**
```
// Directly give particular factory type
vector<JFactoryT<DReaction>*> locFacs = locEvent->GetFactoryAll<DReaction>();
```
### Saving Created Object(s) to Factory
##### _Single Object_
###### **JANA1**
```
_data.push_back(locAnalysisResults);
```
###### **JANA2**
```
Insert(locAnalysisResults);
```
##### _Array of Objects_
```
std::vector<fac1*> results;
results.push_back(new fac1(...));
```
###### **JANA1**
```
for (auto obj: results){
	_data.push_back(obj); 
}
```
###### **JANA2**
```
Set(results)
```
### Getting Created Objects from Factory
##### **JANA1**
```
vector<const DReaction*> locReactionsSubset;
locFactory->Get(locReactionsSubset);
locReactions.insert(locReactions.end(), locReactionsSubset.begin(), locReactionsSubset.end());
```
##### **JANA2**
```
vector<const DReaction*> locReactionsSubset;
auto iters = locFactory->GetOrCreate(locEvent, locEvent->GetJApplication(), locEvent->GetRunNumber());
locReactions.insert(locReactions.end(), iters.first, iters.second);
```
### Factory Tag Handling
#### 1. Setting Tag
##### **JANA1**
```
//Inside factory class definition this was required to be added
const char* Tag(void){return "Combo";}
```
##### **JANA2**
```
// Have to set inside factory constructor now 
DChargedTrack_factory_Combo(){
	SetTag("Combo");
}
```
#### 2. Getting Tag
##### **JANA1**
```
locFactory->Tag()
```
##### **JANA2**
```
locFactory->GetTag()
```
### Factory Object Name Handling
#### 1. Setting Object Name
##### **JANA1**
```
// Was not present in JANA1

DReaction_factory_Thrown(){
//prevents JANA from searching the input file for these objects
use_factory = 1;
}
```
##### **JANA2**
```
// Present in JANA2 and is set inside constructor. Write what it mainly do? 

DReaction_factory_Thrown(){
SetObjectName("DReaction");
SetTag("Thrown");
}
```

## JEvent
### Transition from JEventLoop to JEvent
##### **JANA1**
```
JEventLoop is JANA1 thing, write what it was doing mainly
#include <JANA/JEventLoop.h> -- you have to import this header file in factory and where else?
```
##### **JANA2**
```
JANA2 has replaced JEventLoop with JEvent,

#include <JANA/JEvent.h> --- you have to import it in factory, need to include anywhere else too??

write how it is different that JEventLoop? I think mainly now factoryies are inside factory set instead of having them inside JEvent as it was for JEventLoop.
```
### Getting Run Number
##### **JANA1**
```
run_number = locEventLoop->GetJEvent().GetRunNumber());
```
##### **JANA2**
```
locEvent->GetRunNumber()
```
#### Getting Event Number
##### **JANA1**
```
run_number = locEventLoop->GetJEvent().GetEventNumber());
```
##### **JANA2**
```
locEvent->GetEventNumber()
```

## Acquiring Locks
### Initialization
##### **JANA1**
```
#include "DANA/DApplication.h"


dApplication = dynamic_cast<DApplication*>(locEventLoop->GetJApplication
```
##### **JANA2**
```
#include <JANA/Compatibility/JLockService.h> 


auto app = locEvent->GetJApplication();
jLockService = app->GetService<JLockService>();
// or 
jLockService = japp->GetService<JLockService>()
```
### Available Locks & Their Acquiring
#### 1. ReadLock
##### **JANA1**
```
dActionLock = japp->ReadLock(locLockName); 
pthread_rwlock_unlock(dActionLock); //unlock
```
##### **JANA2**
```
dActionLock = app->GetService<JLockService>()->ReadLock(locLockName); 
pthread_rwlock_unlock(dActionLock); //unlock
```
#### 2. WriteLock
##### **JANA1**
```
japp->WriteLock("DAnalysisResults
japp->Unlock("DAnalysisResults");
```
##### **JANA2**
```
jLockService->WriteLock("DAnalysisResults");
jLockService->Unlock("DAnalysisResults");
```
#### 3.RootWriteLock
##### **JANA1**
```
dApplication->RootWriteLock();  //lock
dApplication->RootUnLock(); //unlock
```
##### **JANA2**
###### Using JLockService
```
jLockService->RootWriteLock(); //lock
jLockService->RootUnLock(); //unlock
```
###### Using DEvent
```
#include "DANA/DEvent.h"
DEvent::GetLockService(locEvent)->RootWriteLock(); 
DEvent::GetLockService(locEvent)->RootUnLock();
```

## Rarely Used Features
### `use_factory`
##### **JANA1**
```
Could be used in JANA1


//prevents JANA from searching the input file for these objects
DEventWriterROOT_factory(){use_factory = 1;}; 
```
##### **JANA2**
```
No more available in JANA2, how is the functionality that it was providing is getting used?
DEventWriterROOT_factory() = default;
```
### `Unknown` Handling
##### **JANA1**
```
//Was a plain enum so could be accessed without any scope resolution operator

#include "particleType.h"

return ((locFirstStep->Get_TargetPID() != Unknown) || (locFirstStep->Get_SecondBeamPID() != Unknown));
```
##### **JANA2**
```
// Is Enum class now so could be accessed like this `Particle_t::Unknown` only

#include "particleType.h"

return ((locFirstStep->Get_TargetPID() != Particle_t::Unknown) || (locFirstStep->Get_SecondBeamPID() != Particle_t::Unknown));
```