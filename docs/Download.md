# Download

### Latest master (unstable)

- `git clone https://github.com/JeffersonLab/JANA2/`

### Latest release
- `git clone --branch latest_release https://github.com/JeffersonLab/JANA2`
- [See release on GitHub](https://github.com/JeffersonLab/JANA2/tree/latest_release)
- [See online doxygen documentation](http://www.jlab.org/JANA/jana_doc_latest/index.html)
- [Download doxygen documentation](http://www.jlab.org/JANA/jana_doc_latest.tar.gz)

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


