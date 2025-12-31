# Download

### Latest master (unstable)

- `git clone https://github.com/JeffersonLab/JANA2/`

### Latest release

- `git clone --branch latest_release https://github.com/JeffersonLab/JANA2`
- [See release on GitHub](https://github.com/JeffersonLab/JANA2/tree/latest_release)
- [See online documentation](https://jeffersonlab.github.io/JANA2/)
- [See online doxygen documentation](https://jeffersonlab.github.io/JANA2/refcpp/)

### 2026.01.00

This release brings a couple of exciting new features along with some important bugfixes. The big JFactory/JDatabundle refactoring is
finally complete, and the bugs that it introduced in v2.4.3 are fixed. We are hoping that this release will be sufficient for merging 
EICrecon's timeframe splitter. This release is also notable for being our first release that uses calendar versioning, which hopefully 
matches our workflow better.

#### Bugfixes

- Fix problems with `Input` and `Output` tags introduced in v2.4.3 (PR #464)
- Use of deprecated `operator->()` on podio collections (PR #477)
- Template instantation error with podio LinkCollections (PR #462)
- Stringify array and vector parameters consistently with primitives (PR #457)

#### Features

- Extend support for external wiring to include non-JFactory components (PR #476, #473)
- Add support for preserving the original event ordering with multithreading enabled (PR #472)
- Add support for multilevel event sources (PR #467)

#### Refactoring

- Podio-collection-producing factories no longer inherit from JFactoryT (PR #460, #479)
- Removed obsolete `Streaming/` code (PR #468)

#### Examples

- Example: Hit reconstruction (PR #470)
- Example: Protoclustering (PR #469)

### 2.4.3

#### Behavior changes

- JHasOutputs represents variadic output names as `std::vector<std::vector<std::string>>`, which now allows for multiple variadic outputs with arbitrary lengths (Pull request #458)

- Removed the never-used JEventProcessorSequential and JEventProcessorSequentialRoot. All their functionality has been backported to JEventProcessor. (Pull request #456)

- JInspector is now launched using the parameter `jana:inspect=true` instead of the command-line argument `--interactive`, which works regardless of which executable runs JANA. (Pull request 459)

#### Bugfixes

- Relax JFactory::Create cycle detector for the sake of JEventSourceEVIOpp (Pull request #461)

- Fix JFactoryPodioT template error when using Podio LinkCollections (Pull request #462)

- JEventUnfolder erroneously inserted outputs even though KeepChildNextParent was returned (Pull request #458)

- JEventUnfolder supports parent events with zero children (Pull request #456)

- JTopologyBuilder creates a chain of TapArrows so that independent JEventProcessors can be pipelined, allowing new-style JEventProcessors to have comparable performance to the old-style ones. (Pull request 456)

#### Features

- Variadic inputs now support an EmptyInputPolicy, which allows components to optionally retrieve all databundles/collections for a given type and EventLevel, analogous to JEvent::GetFactoryAll(). (Pull request #454)

#### Usability improvements and refactoring

- Improve the JANA tutorial and examples by creating a new paradigm, in which a toy reconstruction codebase is systematically built up piece-by-piece. Two versions exist, one using a lightweight (GlueX-style) datamodel, and the other using a Podio datamodel. Apart from the datamodel choice, the two versions are functionally identical. (Pull request #453)

- Improve debugging by having the JANA1-style JEvent::GetSingle directly throw a JException instead of a size_t, thereby producing a full stack trace (Pull request #461)

- Renamed JEventProcessor::Process(const JEvent&) to ProcessSequential to reduce user confusion (Note that this feature was experimental and unused up until now) (Pull request #456)

- JTopologyBuilder::connect() wires arrows using port id instead of port index, reducing confusion while manually configuring topologies. (Pull request #456)

- JFactoryT uses JDatabundle under the hood, as part of a deeper long-term refactoring. (Pull request #458, Issues #254, #276)

### 2.4.2

#### Behavior changes
- `jana:max_inflight_events` now defaults `nthreads` regardless of whether `nthreads` was explicitly set. Previously it defaulted to 4 when `nthreads` was unset, and otherwise defaulted to `nthreads`. (Issue #443)
- `JEventSource::GetEventCount()` is deprecated and replaced by `GetSkippedCount()`, `GetEmittedEventCount()`, and `GetProcessedEventCount()`. These behave intuitively when `nskip` is used. (Issue #428)

#### Features
- Added `jana:output_processed_event_numbers` parameter to assist with debugging (Issue #425)
- Ported `janaroot` plugin from JANA1
- JFactory detects and excepts on cycles (Issue #423)
- Improved scale test visualizations, including plotting multiple scaling tests on the same plot and supporting log scaled axes.

#### Bugfixes
- Missing template argument in VariadicPodioOutput
- JEvent was being marked as warmed up prematurely
- JAutoactivator was being called last instead of first (Issue #440)
- If the user attempted to run without providing a JEventSource, processing would crash with an ArithmeticException instead of a helpful error message (Issue #437)
- `JEventSource::FinishEvent()` was being called spuriously (Issue #424)

#### Refactoring
- Preliminary support for random-access JEventSources is provided via `JEventSource::Skip()`. This feature should be considered experimental for now because it doesn't work with barrier events yet. (Issue #422)

### 2.4.1

This release enables CCDB caching, significantly improving performance and reducing memory usage for applications that frequently access calibration constants.

### 2.4.0

#### Features

- Externally wired factories using `JWiredFactoryGenerator` (#399, #400)

#### Bugfixes

- Fix parameter strictness check (#394)
- Fix Podio deprecation warnings (#389)
- Fix ODR violation (#396)
- Fix `JFactory::Create()` logic (#383)
- Fix `JEventProcessor` deletion order (#391)
- Fix double-free in `JLockService` destructor (#388)

#### Refactoring

- Migrate `JStreamLog` uses to `JLogger` (#390, #395, #398)
- Reorganize and deprecate `Compatibility/` headers (#392, #397)
- Refactor arrow execution machinery (#385, #387, #393)

### 2.3.3

#### Bugfixes
* Fix problem with user-defined factory generators (#366)
* JEventProcessor::Process() called before BeginRun() (#367)
* Lock overwrite in RootFillLock() (#369)
* JFactory::Finish() is called (#377)

#### Features
* JTopologyBuilder supports topologies with arbitrarily nested levels (#346)
* Barrier events are back (#371)

#### Refactoring
* Improved log output (#368)
* JTest uses new-style component interfaces (#374)
* JArrows now fire on individual events (#375, #378)

- [See release on GitHub](https://github.com/JeffersonLab/JANA2/releases/tag/v2.3.3)

### 2.3.2
This release includes the following:

#### Features
- Added a simple `JWiringService` which can be used to wire `JOmniFactories` via a TOML file. (#353, #363)
- Added `add_jana_plugin`, `add_jana_library`, and `add_jana_test` CMake macros (#364)

#### Bugfixes
- A multithreading bug in `JEventProcessor` has been fixed.
- `JFactory::Create` now checks `JEventSource::GetObjects` (#361)
- `JPluginLoader` no longer loads plugins twice in certain cases (#343)
- `JParameterManager::FilterParameters` marks parameters as 'used', thereby avoiding spurious 'unused parameter' warnings. (#331)
- `JTypeInfo::to_string_with_si_prefix` generates the correct SI prefix in certain cases (#348)

#### Refactoring
- Plugins and their headers are now installed to a directory that doesn't conflict with a system install (#330)
- `JPluginLoader` has been extensively rewritten (#339)
- `JCsvWriter` has been moved into `examples` (#350)
- JANA's internal performance testing RNG has been refactored to be more reproducible, and to avoid ASAN violations. (#315)
- `JPodioExample` has been split into several reusable examples. (#352)
- Code was moved from `Omni` and `Status` into `Components`, making the layered architecture clearer (#351)
- Documentation has been overhauled, including adding an extensive JANA1-to-JANA2 migration guide (#334, #336, #342, #354, #357, #359)
- CI testing has been extended (#332, #341)

#### Behavior changes:
- JANA now has one internal logger, configurable via the `jana:loglevel` parameter. External loggers are now configurable via the `jana:global_loglevel` parameter.
- Log output has been streamlined: oversized tables are now YAML, and essential information is now logged at `WARN` level. (#362)
- `JPluginLoader` now stops when a plugin fails to load, rather than continuing searching for another plugin with the same name.
- `JPluginLoader` no longer accepts paths as part of a valid plugin name
- `JFactorySet` is no longer silent when the user attempts to include duplicates of the same factory (#343)
- `JMetadata` is deprecated, to be replaced with `JMultifactory`. (#345)
- All `JFactories` now call `JEventSource::GetObjects`, not just `JGetObjectsFactory`. (#361)

- [See release on GitHub](https://github.com/JeffersonLab/JANA2/releases/tag/v2.3.2)
- [See online doxygen documentation](http://www.jlab.org/JANA/jana_doc_2.3.1/index.html)
- [Download doxygen documentation](http://www.jlab.org/JANA/jana_doc_2.3.1.tar.gz)

### 2.3.1
This release fixes a bug which caused the `janadot` plugin to stop producing output. It also drops support for Podio <= 00-17 by replacing the user-provided `PodioTypeMap` with the built-in `PodioT::collection_type`. 

- [See release on GitHub](https://github.com/JeffersonLab/JANA2/releases/tag/v2.3.1)
- [See online doxygen documentation](http://www.jlab.org/JANA/jana_doc_2.3.1/index.html)
- [Download doxygen documentation](http://www.jlab.org/JANA/jana_doc_2.3.1.tar.gz)

### 2.3.0 
- [See release on GitHub](https://github.com/JeffersonLab/JANA2/releases/tag/v2.3.0)
- [See online doxygen documentation](http://www.jlab.org/JANA/jana_doc_2.3.0/index.html)
- [Download doxygen documentation](http://www.jlab.org/JANA/jana_doc_2.3.0.tar.gz)

### JANA 1

- JANA 1 is deprecated but still in use with projects such as GlueX.
- [JANA 1 homepage](https://www.jlab.org/JANA/)
- [JANA 1 repo](https://github.com/JeffersonLab/JANA)


