
# JANA2

### C++ Reconstruction framework in Nuclear and High Energy Physics

## Welcome to JANA2!

JANA2 is a C++ framework for multi-threaded HENP (High Energy and Nuclear Physics)  event reconstruction.
It is very efficient at multi-threading with a design that makes it easy for less experienced programmers
to contribute pieces to the larger reconstruction project. The same JANA2 program can be used to easily
do partial or full reconstruction, fully maximizing the available cores for the current job.

Its design strives to be extremely easy to setup when first getting started, yet have a depth of customization
options that allow for more complicated reconstruction as your project grows. The intent is to make it easy to
run on a laptop for local code development, but to also be highly efficent when deploying to large computing
sites like [NERSC](http://www.nersc.gov/ ":target=_blank").

The project is [hosted on GitHub](https://github.com/JeffersonLab/JANA2)

```cpp
auto tracks = jevent->Get<DTrack>();

for(auto t : tracks){
  // ... do something with a track
}
```


## Design philosophy

JANA2's design philosophy can be boiled down to five values, ordered by importance:

### Simple to use

JANA2 focuses on making parallel computations over event-based\* data simple. 
Unlike the aforementioned, JANA2's vocabulary of abstractions is designed around the needs of physicists rather than 
general programmers. However, JANA2 does not attempt to meet _all_ of the needs of physicists.

JANA2 recognizes that some tasks, like data persistence, should be handled separately. 
As example, instead of providing its own persistence layer or requiring specific dependencies like ROOT, Numpy, or Apache Arrow, 
JANA2 allows users to choose their preferred tools. 
This flexibility ensures that if a team wants to switch from one tool to another (e.g., from ROOT to Arrow), 
the core analysis code remains largely unaffected.

To keep things simple, JANA minimizes the complexity of its build system and orchestration. 
Using JANA should be straightforward: implement a several key interfaces, add an include path, and link against a single library.

?> **Tip** The term `event-based` in JANA2 doesn't strictly refer to _physics_ or _trigger_ events. 
In JANA2, `event` is used in a broader computer science context, aligning with the streaming readout paradigm 
and supporting concepts like event nesting and sub-event parallelization.


### Well-organized

While JANA's primary goal is running code in parallel, its secondary goal is imposing an organizing principle on the users' codebase. 
This can be invaluable in a large collaboration where members vary in programming skill. Specifically, 
JANA organizes processing logic into decoupled units. JFactories are agnostic of how and when their prerequisites are 
computed, are only run when actually needed, and cache their results for reuse. Different analyses can coexist in separate
JEventProcessors. Components can be compiled into independent plugins, to be mixed and matched at runtime. All together, 
JANA enforces an organizing principle that enables groups to develop and test their code with both freedom and discipline.


### Safe

JANA recognizes that not all of its users are proficient parallel programmers, and it steers users towards patterns which
mitigate some of the pitfalls. Specifically, it provides:

- **Modern C++ features** such as smart pointers and judicious templating, to discourage common classes of bugs. JANA seeks to
make its memory ownership semantics explicit in the type system as much as possible.

- **Internally managed locks** to reduce the learning curve and discourage tricky parallelism bugs.

- **A stable API** with an effort towards backwards-compatibility, so that everybody can benefit from new features
and performance/stability improvements.


### Fast

JANA uses low-level optimizations wherever it can in order to boost performance. 

### Flexible

The simplest use case for JANA is to read a file of batched events, process each event independently, and aggregate 
the results into a new file. However, it can be used in more sophisticated ways. 

- Disentangling: Input data is bundled into blocks (each containing an array of entangled events) and we want to 
parse each block in order to emit a stream of events (_flatmap_)

- Software triggers: With streaming data readout, we may want to accept a stream of raw hit data and let JANA 
determine the event boundaries. Arbitrary triggers can be created using existing JFactories. (_windowed join_)

- Subevent-level parallelism: This is necessary if individual events are very large. It may also play a role in 
effectively utilizing a GPU, particularly as machine learning is adopted in reconstruction (_flatmap+merge_)

JANA is also flexible enough to be compiled and run different ways. Users may compile their code into a standalone 
executable, into one or more plugins which can be run by a generic executable, or run from a Jupyter notebook. 


## Comparison to other frameworks

Many different event reconstruction frameworks exist. The following are frequently compared and contrasted with JANA:

- [Clara](https://claraweb.jlab.org/clara/) While JANA specializes in thread-level parallelism, Clara
 uses node-level parallelism via a message-passing interface. This higher level of abstraction comes with some performance
 overhead and significant orchestration requirements. On the other hand, it can scale to larger problem sizes and 
 support more general stream topologies. JANA is to OpenMP as Clara is to MPI.
 

## History

[JANA](https://halldweb.jlab.org/DocDB/0011/001133/002/Multithreading_lawrence.pdf) (**J**Lab **ANA**lysis framework) 
was one of the earliest implementations of a multi-threaded, event processing framework designed to run on commercial
CPU hardware. The framework, which was developed since 2005, has been used for many aspects of [the GlueX experiment](https://arxiv.org/abs/1911.11239)
at Jefferson Lab which was commissioned in 2016. In addition to the reconstruction of both
acquired and simulated data, JANA is used for the online monitoring system and was also used
to develop a L3 trigger (also known as a high level software trigger). With over a decade of
experience in developing and using JANA, an effort has begun to develop a next-generation
framework, JANA2. JANA2 leverages language features introduced in the modern C++11 standards
to produce a framework that can help carry the next generation of Nuclear Physics experiments
in the coming decade. Currently JANA2 is used as a backbone for the 
[reconstruction framework at Electron Ion Collider](https://github.com/eic/EICrecon), 
[TriDAS Streaming Readout DAQ](https://arxiv.org/abs/2104.11388),
GluEx and other experiments. 


## Publications

- [JANA2: Multithreaded Event Reconstruction](https://indico.cern.ch/event/708041/papers/3276151/files/9134-JANA2___ACAT2019_Proceedings.pdf)
- [Running AI in JANA2](https://indico.cern.ch/event/1238718/contributions/5431992/attachments/2691231/4670186/2023-07-27_Running-AI-in-JANA2.pdf)
- [Electron Ion Collider Software Overview](https://www.epj-conferences.org/articles/epjconf/pdf/2024/05/epjconf_chep2024_03011.pdf)
- [JANA2 Framework for Event Based and Triggerless Data Processing](https://doi.org/10.1051/epjconf/202024501022)
- [Streaming readout for next generation electron scattering experiments](https://arxiv.org/abs/2202.03085)
- [Streaming Readout of the CLAS12 Forward Tagger Using TriDAS and JANA2](https://arxiv.org/abs/2104.11388)
- [Offsite Data Processing for the GlueX Experiment](https://doi.org/10.1051/epjconf/202024507037)

Curated publications list on [EPSCI Wiki](https://wiki.jlab.org/epsciwiki/index.php/EPSCI_publications_page)
