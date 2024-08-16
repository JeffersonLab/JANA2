
****
## Common Functions

### JANA Namespace
##### **JANA1**
Adding following on top of all files was necessary in JANA1

```
using namespace jana;
```
##### **JANA2**
No more required to add that line in JANA2

### Application
#### Getting application
##### **JANA1**
There was just a japp that could be used directly, am I right??
##### **JANA2**
###### _Within a particular context_
```
// Figure out what context, don't know yet ??
auto app = GetApplication();
```
###### _Out of that particular context, you can get it from event_, don't know which??
```
auto app = locEvent->GetJApplication();
```
### Parameters
#### Setting Parameter
##### **JANA1**
```
gPARMS->GetParameter("OUTPUT_FILENAME", dOutputFileName);
```
##### **JANA2**
######  _Inside Init_ ????? Is this right?
```
auto app = GetApplication();
app->SetDefaultParameter("OUTPUT_FILENAME", dOutputFileName);
```
###### _Inside Constructor_ ???
```
japp->SetDefaultParameter(locFullParamName, locKeyValue);
```
#### Getting Parameter
##### **JANA1**
```
//gets all parameters with this filter at the beginning of the key
gPARMS->GetParameters(locParameterMap, "COMBO_DEDXCUT:"); 
```
##### **JANA2**
###### _Inside Constructor_ ??? Is this right??
```
//gets all parameters with this filter at the beginning of the key
japp->GetJParameterManager()->FilterParameters(locParameterMap, "COMBO_DEDXCUT:");
```

### DApplication
#### Getting DApplication
##### **JANA1**

```
#include "DANA/DApplication.h"
dApplication = dynamic_cast<DApplication*>(locEventLoop->GetJApplication
```

##### **JANA2**
There is no DApplication anymore in JANA2

### DGeometry
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

dApplication = dynamic_cast<DApplication*>(eventLoop->GetJApplication
DGeometry *locGeom = dApplication ? dApplication->GetDGeometry(eventLoop->GetJEvent().GetRunNumber()) : NULL;

```
##### **JANA2**
###### Using japp
```
#include "HDGEOMETRY/DGeometry.h"

auto dgeoman = japp->GetService<DGeometryManager>();
const DGeometry *dgeom = dgeoman->GetDGeometry(event->GetRunNumber());
```
###### Using DEvent
```
#include "DANA/DEvent.h"
#include "HDGEOMETRY/DGeometry.h"

DGeometry *locGeometry = DEvent::GetDGeometry(locEvent);
```
###### Using app
```
#include "HDGEOMETRY/DGeometry.h"

auto runnumber = event->GetRunNumber();
auto app = event->GetJApplication();
auto geo_manager = app->GetService<DGeometryManager>();
auto geom = geo_manager->GetDGeometry(runnumber);
```

### Calibration
#### GetCalib
##### **JANA1**
```
locEventLoop->GetCalib(locTOFParmsTable.c_str(), tofparms);
```
##### **JANA2**
###### 1. Using DEvent
```
#include "DANA/DEvent.h"

DEvent::GetCalib(locEvent, locTOFParmsTable.c_str(), tofparms);
```
###### 2. Using GetApplication
Requires you to be inside a particular context. What context exactly???
```
#include <JANA/JEvent.h>
#include <JANA/Calibrations/JCalibrationManager.h>

auto event_number = event->GetEventNumber();
auto run_number = event->GetRunNumber

auto app = GetApplication();

auto calibration = app->GetService<JCalibrationManager>()->GetJCalibration(run_number);
calibration->Get("BCAL/mc_parms", bcalparms, event_number)


```

### Status Bit
#### GetStatusBit
##### **JANA1**
```
locEventLoop->GetJEvent().GetStatusBit(kSTATUS_PHYSICS_EVENT)
```
##### **JANA2**
```
#include "DANA/DStatusBits.h"


locEvent->GetSingle<DStatusBits>()->GetStatusBit(kSTATUS_PHYSICS_EVENT)

// or 

locEvent->GetSingleStrict<DStatusBits>()->GetStatusBit(kSTATUS_REST);
```

### BField
#### GetBField
##### **JANA1**
```
#include "DANA/DApplication.h"

dApplication = dynamic_cast<DApplication*>(locEventLoop->GetJApplication
dMagFMap= dApplication->GetBfield(locEventLoop->GetJEvent().GetRunNumber());

```
##### **JANA2**
```
#include "DANA/DEvent.h"

dMagneticFieldMap = DEvent::GetBfield(locEvent);
```



## Modified Gets

### JObject
#### 1. GetSingleT
##### **JANA1**
```
Was present in JANA1
```
##### **JANA2**
```
No more available in JANA2, use GetSingle instead
```
#### 2. Get Single
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
#### 3. Get
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

### JEvent 
#### 1. GetSingle
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
#### 2. Get
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
### Child JEventProcessor Basic Header File
##### JANA1
```
  

#ifndef _JEventProcessor_myplugin_
#define _JEventProcessor_myplugin_

#include <JANA/JApplication.h>
#include <JANA/JFactory.h>
#include <JANA/JEventProcessor.h>
using namespace jana;
  

class JEventProcessor_myplugin:public jana::JEventProcessor{
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

#ifndef _JEventProcessor_secondTest_
#define _JEventProcessor_secondTest_
#include <JANA/JEventProcessor.h>

class JEventProcessor_secondTest:public JEventProcessor{

public:
	JEventProcessor_secondTest();
	~JEventProcessor_secondTest();
	const char* className(void){return "JEventProcessor_secondTest";}

  

private:

	void Init() override;
	void BeginRun(const std::shared_ptr<const JEvent>& event) override; 
	void Process(const std::shared_ptr<const JEvent>& event) override; 
	void EndRun() override; 
	void Finish() override; 
};

  

#endif // _JEventProcessor_secondTest_
```


## JObject
### Child JObject Basic Header File
##### JANA1
```
#include <JANA/JObject.h>
#include <JANA/JFactory.h> // No more required in JANA2

using namespace jana; // No more required in JANA2

class DCereHit: public JObject {

public:

JOBJECT_PUBLIC (DCereHit);
int sector; 
float pe; 
float t;

//Changed to Summarize in JANA2
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

JOBJECT_PUBLIC (DCereHit);
int sector; 
float pe; 
float t; 

// This function was called toStrings in JANA1
void Summarize(JObjectSummary& summary) const override {
summary.add(sector, "sector", "%d");
summary.add(pe, "pe", "%1.3f");
summary.add(t, "t", "%1.3f");
}

};
```
### id
##### **JANA1**
```
// JANA1 JObject had a data member id

DBCALShower *shower = new DBCALShower;
shower->id = id++;
```
##### **JANA2**
```
// id data member does not exist in JObject anymore in JANA2

DBCALShower *shower = new DBCALShower;
shower->id = id++; // Throw Error: no id member
```
### oid_t
##### **JANA1**
```
JObject::oid_t 
```
##### **JANA2**
```
#include "DANA/DObjectID.h"

oid_t //Not inside JObject anymore?
```


## JFactory


### JFactory to JFactoryT
##### **JANA1**
```
Was existing and getting used in JANA1, write what for
```

##### **JANA2**
```
Now JFactoryT is getting used everywhere instead of JFactory, I think but not sure if their functionality is same. Write how it is same and different from previous one? Also apparently JFactory still exist in JANA2 why so? and how is it different compared to one in JANA1?
```

### Child Factory Basic Header File
##### JANA1
```
#ifndef _myfactory_factory_
#define _myfactory_factory_

#include <JANA/JFactory.h>
#include "myfactory.h"

class myfactory_factory:public jana::JFactory<myfactory>{

public:
myfactory_factory(){};
~myfactory_factory(){};

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
#include "thirdFacTest.h"

class thirdFacTest_factory:public JFactoryT<thirdFacTest>{
public:
thirdFacTest_factory(){
SetTag("");
}
~thirdFacTest_factory(){}


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

### Getting All factories
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

### Factory Tag
#### 1. SetTag
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
#### 2. GetTag
##### **JANA1**
```
locFactory->Tag()
```
##### **JANA2**
```
locFactory->GetTag()
```


### Factory ObjectName
#### 1. SetObjectName

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
### JEventLoop to JEvent 
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
### Pre Steps
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
### Locks
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

## Rarely used
### use_factory 
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
### Unknown Enum
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
