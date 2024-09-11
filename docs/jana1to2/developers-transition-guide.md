

# Developers Transition Guide

***Key Syntax Changes between JANA1 to JANA2***

This guide outlines the critical syntax and functional updates between JANA1 and JANA2, helping developers navigate the transition smoothly.



## Namespace
##### **JANA1**
Adding following on top of all files was necessary in JANA1:

```
using namespace jana;
```
##### **JANA2**
This line is no longer required in JANA2.


## Application

### JApplication
#### Getting the JApplication

##### **JANA1**
In JANA1 there is a global variable `japp` which is a pointer to the project's `JApplication`. You can also obtain it from `JEventLoop::GetJApplication()`.

##### **JANA2**
In JANA2, `japp` is still around but we are strongly discouraging its future use. If you are within any JANA component (for instance: JFactory, JEventProcessor, JEventSource, JService) you can obtain the `JApplication` pointer from `this`, like so:

```cpp
auto app = GetApplication();
```

You can also obtain it from the `JEvent` the same way as you used to from `JEventLoop`, i.e. 

```cpp
auto app = event->GetJApplication();
```

### DApplication
### Getting DApplication
##### **JANA1**

```cpp
#include "DANA/DApplication.h"
dApplication = dynamic_cast<DApplication*>(locEventLoop->GetJApplication
```

##### **JANA2**
There is no `DApplication` anymore in JANA2. `JApplication` is `final`, which means you can't inherit from it. The functionality on `DApplication` has been moved into `JService`s, which can be accessed via `JApplication::GetService<ServiceT>()`. 

```cpp
auto dapp = GetApplication()->GetService<DApplication>();
```


## Parameter Manager
### Setting Parameters

##### **JANA1**

```cpp
gPARMS->GetParameter("OUTPUT_FILENAME", dOutputFileName);
```

##### **JANA2**
You should obtain parameters as shown below. 

```cpp
auto app = GetApplication();
app->SetDefaultParameter("component_prefix:value_name", ref_to_member_var);
```

We strongly recommend you register all parameters from inside the `Init()` callbacks of your JANA components, or from `InitPlugin()`. This helps JANA:

- Report back which parameters are available without having to process an input file.
- Emit a warning (or error!) when a user-provided parameter is misspelled
- Emit a warning when two plugins register the same parameter with conflicting default values.
- Inspect the exact parameter values used by a factory

If you register parameters in other contexts, this machinery might not work and you might get incomplete parameter lists and missing or spurious error messages. Worse, your code might not see the parameter values you expect it to see. Registering parameters from inside constructors is less problematic but still discouraged because it won't work with some upcoming new features.

### Getting Parameter Maps

##### **JANA1**
To obtain a map of all parameters that share a common prefix:

```cpp
//gets all parameters with this filter at the beginning of the key
gPARMS->GetParameters(locParameterMap, "COMBO_DEDXCUT:"); 
```

##### **JANA2**
In JANA2 this has been renamed to `FilterParameters` but the functionality is the same.
Note that this method is not on `JApplication` directly; you have to obtain the parameter manager like so:

```cpp
//gets all parameters with this filter at the beginning of the key
GetApplication()->GetJParameterManager()->FilterParameters(locParameterMap, "COMBO_DEDXCUT:");
```


## DGeometry
#### Getting DGeometry

##### **JANA1**
###### Using japp
```cpp
#include "DANA/DApplication.h"
#include "HDGEOMETRY/DGeometry.h"

DApplication* dapp=dynamic_cast<DApplication*>(japp);
const DGeometry *dgeom = dapp->GetDGeometry(runnumber);
```

###### Using eventLoop

```cpp
#include "DANA/DApplication.h"
#include "HDGEOMETRY/DGeometry.h"

DGeometry *locGeom = dApplication ? dApplication->GetDGeometry(eventLoop->GetJEvent().GetRunNumber()) : NULL;
```

##### **JANA2**
In JANA2, you obtain the `DGeometryManager` from the `JApplication` and from there you use the run number to obtain the corresponding `DGeometry`:

```cpp
#include "HDGEOMETRY/DGeometry.h"

auto runnumber = event->GetRunNumber();
auto app = event->GetJApplication();
auto geo_manager = app->GetService<DGeometryManager>();
auto geom = geo_manager->GetDGeometry(runnumber);
```

To reduce the boilerplate, we've provided a handy helper function, `DEvent::GetDGeometry()`, that does all of these steps for you:

```cpp
#include "DANA/DEvent.h"

DGeometry *locGeometry = DEvent::GetDGeometry(locEvent);
```

You should call `GetDGeometry` from `JFactory::BeginRun` or `JEventProcessor::BeginRun`.


## Calibration
#### Accessing Calibration Data - `GetCalib` 
##### **JANA1**

```cpp
locEventLoop->GetCalib(locTOFParmsTable.c_str(), tofparms);
```

##### **JANA2**
Analogously to `DGeometry`, you obtain the calibrations by obtaining the `JCalibrationManager` from `JApplication::GetService<>()` and then loading the calibration object corresponding to the event's run number. We recommend you use `DEvent::GetCalib` to avoid this boilerplate. You should call this from your `BeginRun` callback. 

```cpp
#include "DANA/DEvent.h"

DEvent::GetCalib(event, locTOFParmsTable.c_str(), tofparms);
```

In principle, the calibrations could also be keyed off of event number intervals, in which case you should call it from `Process` and pass the event number as well:

```cpp
#include <JANA/JEvent.h>
#include <JANA/Calibrations/JCalibrationManager.h>

auto event_number = event->GetEventNumber();
auto run_number = event->GetRunNumber();

auto app = GetApplication();
auto calibration = app->GetService<JCalibrationManager>()->GetJCalibration(run_number);
calibration->Get("BCAL/mc_parms", bcalparms, event_number)
```


## Status Bit
#### Accessing Status Bit - `GetStatusBit`

##### **JANA1**

```cpp
locEventLoop->GetJEvent().GetStatusBit(kSTATUS_PHYSICS_EVENT)
```

##### **JANA2**
Status bits are no longer an event member variable. Instead, they are inserted and retrieved just like the other collections:

```cpp
#include "DANA/DStatusBits.h"


locEvent->GetSingle<DStatusBits>()->GetStatusBit(kSTATUS_PHYSICS_EVENT)

// or, for a little bit more safety:

locEvent->GetSingleStrict<DStatusBits>()->GetStatusBit(kSTATUS_REST);
```


## Magnetic Field
#### Getting Magnetic Field - `GetBField`

##### **JANA1**

```cpp
#include "DANA/DApplication.h"

dApplication = dynamic_cast<DApplication*>(locEventLoop->GetJApplication());
dMagFMap= dApplication->GetBfield(locEventLoop->GetJEvent().GetRunNumber());

```
##### **JANA2**
In JANA2, the magnetic field map is a resource handled analogously to `DGeometry` and `JCalibrationManager`. Thus, we recommend obtaining the 
field map by calling `DEvent::GetBField(event)` from inside `BeginRun`:

```cpp
#include "DANA/DEvent.h"

dMagneticFieldMap = DEvent::GetBfield(locEvent);
```


## Get Functions
### JObject Getters

#### 1. `GetSingleT`

`GetSingleT` has been replaced with `GetSingle`. Templated methods no longer have a `T` suffix; templated classes (such as `JFactoryT`) still do.
The new `GetSingle` returns a pointer rather than updating an out parameter. If there isn't exactly one associated item of that type, it will return 
`nullptr`.

##### **JANA1**

Was present in JANA1


##### **JANA2**

No longer available in JANA2; use ```GetSingle()``` instead


#### 2. `GetSingle`
##### **JANA1**

```cpp
// Pass by reference
// Set passed param equal to pointer to object
const Df250PulsePedestal* PPobj = NULL;
digihit->GetSingle(PPobj);
```

##### **JANA2**
```cpp
// Template function
// Return pointer to object
auto PPobj = digihit->GetSingle<Df250PulsePedestal>();
```


#### 3. `Get`

Similarly to `GetSingle`, `Get` returns a vector rather than updating an out parameter.
##### **JANA1**

```cpp
// Pass by Reference
vector<const DBCALUnifiedHit*> assoc_hits;
point.Get(assoc_hits);
```

##### **JANA2**
```cpp
// Templated Function
// Return value
vector<const DBCALUnifiedHit*> assoc_hits = point.Get<DBCALUnifiedHit>();
```
### JEvent Getters


#### 1. `GetSingle`
##### **JANA1**

```cpp
// Pass by Reference, 
// passed param got assigned pointer to object

const DTTabUtilities* locTTabUtilities = NULL;
loop->GetSingle(locTTabUtilities); 
```

##### **JANA2**
```cpp
// Return the pointer to object based on template type

const DTTabUtilities* locTTabUtilities = event->GetSingle<DTTabUtilities>();
```

#### 2. `Get`
##### **JANA1**

```cpp
// Pass by Reference
vector<const DBCALShower*> showers;
loop->Get(showers, "IU");
```

##### **JANA2**
```cpp
// JANA1 Get is still available but will be deprecated soon
// So better to use the one given below

// Templated function
// Return value
auto showers = event->Get<DBCALShower>("IU");
```

## JEventProcessor
### JEventProcessor Header File

`JEventProcessors` in JANA2 have a similar feel to JANA1. The key differences are:

1. The names and type signatures of the callbacks have changed
2. The event number and run number now live on the JEvent object rather than as separate arguments
3. JANA2 doesn't use `jerror_t` anywhere for several reasons: there's too many options to exhaustively check, most 
error codes don't apply to JEventProcessors, there's no clear indication which error codes represent failure conditions,
and no reasonable strategy JANA can take if a failure is reached other than a premature exit. Thus the callbacks return
void. 
4. If a failure condition is reached in user code, the user should throw a `JException` with a detailed message. JANA will
add additional context information (such as the component name, callback, plugin, and backtrace), log everything, and shut
down cleanly.
5. The user no longer provides a `className` callback. Instead, they call `SetTypeName(NAME_OF_THIS);` from the constructor.

##### JANA1

```cpp
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

```cpp
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


`JObjects` in JANA2 are also conceptually similar to their JANA1 counterparts, with a
syntax refresh aimed at future extensibility. The `toStrings` callback has been
replaced with a `Summarize` callback as shown below. Note that in JANA2, data model classes
do not need to inherit from `JObject` anymore - `JObject` merely provides clean, user-specified formatting, 
and convenient simple tracking of object associations. JANA2 also supports using [PODIO](https://github.com/AIDASoft/podio)
based data models, which provide free serialization/deserialization, a first-class Python interface, automatic
memory management and immutability guarantees, and relational object associations.

##### JANA1
```cpp
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

```cpp
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

In JANA1, JObjects provided an `id` field. However it was left up to the user how to set and use it. Because JANA can't guarantee
that it is set reliably or consistently, it is fundamentally part of the data model and not part of the framework. Thus, JANA2 
no longer provides a built-in object id. If needed, users are welcome to add their own `id` field to their data model classes and
establish a convention for how and when it gets set. For reliable, automatic object IDs for all data model classes, consider using PODIO.


##### **JANA1**
```cpp
// JANA1 JObject had a data member id

DBCALShower *shower = new DBCALShower;
shower->id = id++;
```
##### **JANA2**

```cpp
// id data member does not exist in JObject anymore in JANA2

DBCALShower *shower = new DBCALShower;
shower->id = id++; // Compiler error
```

Similarly, the `JObject::oid_t` type has been removed from JANA2 as well. If you need it, it lives on in the halld_recon codebase:

```cpp
#include "DANA/DObjectID.h"

oid_t // Not inside JObject anymore!
```

## JFactory
### Transition from JFactory to JFactoryT
##### **JANA1**
In JANA1, `JFactory_base` was a common factory base class, and `JFactory` was a template that inherits from `JFactory_base` and adds functionality specific to the type of the factory's output collection.
##### **JANA2**
In JANA2, the base class was renamed to `JFactory`, and the template was renamed to `JFactoryT`. JANA2 adds some new functionality, but the old functionality is largely consistent with JANA1. 

The syntax differences between JANA1 and JANA2 `JFactory`s' are consistent with those for `JEventProcessor`s. 

1. The names and type signatures of the callbacks have changed
2. The event number and run number now live on the JEvent object rather than as separate arguments
3. JANA2 doesn't use `jerror_t` anywhere for several reasons: there's too many options to exhaustively check, most 
error codes don't apply to JEventProcessors, there's no clear indication which error codes represent failure conditions,
and no reasonable strategy JANA can take if a failure is reached other than a premature exit. Thus the callbacks return
void. 
4. If a failure condition is reached in user code, the user should throw a `JException` with a detailed message. JANA will
add additional context information (such as the component name, callback, plugin, and backtrace), log everything, and shut
down cleanly.
5. The user no longer provides a `className` callback. Instead, they call `SetTypeName(NAME_OF_THIS);` from the constructor.

Another important difference is how factory tags are set. In JANA1, the user provides a `Tag()` callback, whereas in JANA2,
the user sets the tag by calling `SetTag()` from the constructor. The user can retrieve the factory tag by calling `GetTag()`.

One important semantic difference is that in JANA1, there is always one factory set assigned to each thread, whereas in JANA2, 
there is one factory set for each event in flight. The number of in-flight events is a parameter controlled separately 
from the thread count. This is important for ensuring that there is always work for threads to do when running 
multi-level processing topologies. Another consequence of this design is that factories don't belong to just one thread and hence 
shouldn't reference thread-local variables - instead, one thread can pop an event from a queue, process it, and push it to a 
different queue, where it could later be picked up by a different thread. Importantly, however, only one thread has access to any 
given JEvent and its corresponding factory set at any time, so you don't need to worry about thread safety in JFactories.

### Factory Header File

##### JANA1

```cpp
#ifndef _myfactory_factory_
#define _myfactory_factory_

#include <JANA/JFactory.h>
#include "myobject.h"

class myfactory_factory : public jana::JFactory<myobject> {

public:
    myfactory_factory() = default;
    ~myfactory_factory() = default;

    const char* Tag() { return "MyTag"; }

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

```cpp
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

In JANA1, the user creates a `JFactoryGenerator` with a `GenerateFactories()` callback. Within this callback, 
the user instantiates new factories and adds them to a `JEventLoop`. (In `halld_recon`, there is a `DFactoryGenerator`
which instantiates factories for all fundamental detector plugins, calling each plugin's corresponding 
`$DETECTOR_init()` function.)

In JANA2, the user is still free to create their own `JFactoryGenerator`. It's syntax has been modified (notably, 
`GenerateFactories()` provides a `JFactorySet` argument that the user adds the factories to), but the core concepts
are the same. However, there is now also a `JFactoryGeneratorT` utility which reduces the boilerplate. As long
as the factory is default constructible, the user merely needs to pass the factory class as a template argument to
`JFactoryGeneratorT`, and JANA will do the rest of the work. JANA2 is perfectly fine with users passing a single 
`JFactoryGenerator` that instantiates every single factory in the system, but there are future benefits if users
pass separate `JFactoryGeneratorT`s for each factory instead.


##### **JANA1**

```cpp
#include <JANA/JApplication.h>
#include "JFactoryGenerator_myfactory.h"
#include "myfactory.h"

using namespace jana;

extern "C" {
void InitPlugin(JApplication *app) {
	InitJANAPlugin(app);
	app->AddFactoryGenerator(new JFactoryGenerator_myfactory());
}
} // "C"
```

##### **JANA2**

```cpp
#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>
#include "myfactory.h"

extern "C" {
void InitPlugin(JApplication *app) {
	InitJANAPlugin(app);
	app->Add(new JFactoryGeneratorT<my_factory>());
}
} // "C"
```

### Getting All Factories

Sometimes it is necessary to retrieve all factories that produce objects of a given type. In
JANA1 this was done by retrieving all factories and doing a `dynamic_cast`. In JANA2 there's a
dedicated method for this, `GetFactoryAll`. (Note that this will retrieve the factories without
automatically running them!). JANA2 also supports retrieving all objects of type `T` from all
factories using `GetAll()` and retrieving all objects descended from parent type `T` 
(e.g. `JObject` or `TObject`) using `GetAlllChildren()`.

##### **JANA1**

```cpp
vector<JFactory_base*> locFactories = locEventLoop->GetFactories();
// Cast JFactory_base to particular factory type
JFactory<DReaction>* locFactory = dynamic_cast<JFactory<DReaction>*>(locFactories[0]);
```

##### **JANA2**

```cpp
// Retrieve all factories producing T directly
vector<JFactoryT<DReaction>*> locFacs = locEvent->GetFactoryAll<DReaction>();
```

### Saving Created Object(s) to Factory

When writing a factory, the user creates some data objects inside `Process()` and
then needs to pass them back to JANA. In JANA1, this was done by modifying the
`_data` member variable directly. In JANA2, we prefer using a dedicated setter method.
For the sake of porting GlueX without having to rewrite certain complex factories, 
we've left the member variable `protected`, though the member variable has been 
renamed to `mData`.

##### _Single Object_
###### **JANA1**

```cpp
_data.push_back(locAnalysisResults);
```

###### **JANA2**

```cpp
Insert(locAnalysisResults);

// Or (less preferred)
mData.push_back(locAnalysisResults);
```

##### _Array of Objects_

```cpp
std::vector<Hit*> results;
results.push_back(new Hit(...));
```

###### **JANA1**

```cpp
for (auto hit: results){
	_data.push_back(hit); 
}
```

###### **JANA2**

```cpp
Set(results)
```

## JEvent
### Transition from JEventLoop to JEvent
##### **JANA1**
In JANA1, all processing operated on a `JEventLoop` object. In JANA2, this has been changed to `JEvent`.
Conceptually, a `JEvent` is just a container for data that can be processed as a discrete unit, indepedently from the rest
of the stream (this usually correspondings to a physics event, but also potentially a timeslice, block, subevent, etc). This includes the 
event number, run number, all data read from the event source, all data created by factories so far, and all of
the factory state necessary to generate additional data belonging to the same context. The factories themselves are managed by a `JFactorySet`
which is one-to-one with and owned by the JEvent.

### Getting the Run Number
##### **JANA1**
```cpp
run_number = locEventLoop->GetJEvent().GetRunNumber());
```
##### **JANA2**
```cpp
locEvent->GetRunNumber()
```

#### Getting the Event Number
##### **JANA1**

```cpp
run_number = locEventLoop->GetJEvent().GetEventNumber());
```
##### **JANA2**

```cpp
locEvent->GetEventNumber()
```

## Acquiring Locks

JANA1 required the user to manually acquire and hold locks when accessing to shared resources in a JEventProcessor.
JANA2 offers a different interface which handles all locks internally. However, using this new callback interface
would require restructuring the existing `JEventProcessor` code. When migrating from JANA1 to JANA2 it 
is much safer to avoid making such deep changes, at least initially. The old-style user-managed locks can be migrated
to JANA2 as-is with one minor change: the various lock helper methods have been moved from `JApplication` to `JLockService`.


### ROOT Read/Write locks
##### JANA1

```cpp
dActionLock = japp->RootReadLock(); 
// ...
japp->RootUnLock(locLockName);
```
##### JANA2

```cpp
auto app = GetApplication(); // or event->GetJApplication()
auto lock_svc = app->GetService<JLockService>();

lock_svc->RootReadLock(); 
// ...
lock_svc->RootUnLock();
```

To reduce boilerplate, we've added a helper function:

```cpp
#include "DANA/DEvent.h"

DEvent::GetLockService(locEvent)->RootWriteLock(); 
DEvent::GetLockService(locEvent)->RootUnLock();
```

### Named locks

##### JANA1

```cpp
dActionLock = japp->ReadLock(locLockName); 
// ...
pthread_rwlock_unlock(dActionLock);
// Or: japp->Unlock(locLockName);
```

##### JANA2

```cpp
auto app = GetApplication(); // or event->GetJApplication()
auto lock_svc = app->GetService<JLockService>();

dActionLock = lock_svc->ReadLock(locLockName); 
// ...
pthread_rwlock_unlock(dActionLock);
// Or: lock_svc->Unlock(locLockName);
```

To reduce boilerplate:

```cpp
DEvent::GetLockService(locEvent)->ReadLock("app"); 
DEvent::GetLockService(locEvent)->Unlock("app");
```


## Rarely Used Features

### `Unknown` Handling

The enum in `particleType.h` was experiencing a name conflict with a JANA2 enum, so it has been changed
to an enum class.

##### **JANA1**

```cpp
//Was a plain enum so could be accessed without any scope resolution operator

#include "particleType.h"

return ((locFirstStep->Get_TargetPID() != Unknown) || (locFirstStep->Get_SecondBeamPID() != Unknown));
```

##### **JANA2**
```cpp
// Is Enum class now so could be accessed like this `Particle_t::Unknown` only

#include "particleType.h"

return ((locFirstStep->Get_TargetPID() != Particle_t::Unknown) || (locFirstStep->Get_SecondBeamPID() != Particle_t::Unknown));
```
