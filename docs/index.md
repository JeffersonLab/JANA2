
# JANA: Multi-threaded HENP Event Reconstruction


## Welcome to JANA!

JANA is a C++ framework for multi-threaded HENP (High Energy and Nuclear Physics)  event reconstruction.
It is very efficient at multi-threading with a design that makes it easy for less experienced programmers
to contribute pieces to the larger reconstruction project. The same JANA program can be used to easily
do partial or full reconstruction, fully maximizing the available cores for the current job.

Its design strives to be extremely easy to setup when first getting started, yet have a depth of customization
options that allow for more complicated reconstruction as your project grows. The intent is to make it easy to
run on a laptop for local code development, but to also be highly efficent when deploying to large computing
sites like [NERSC](http://www.nersc.gov/){:target="_blank"}.

JANA has undergone a large rewrite with the newer version dubbed JANA2. The code is now available for use
and you are free to browse around. The project is 
[hosted on GitHub](https://github.com/JeffersonLab/JANA2)

```cpp
auto tracks = jevent->Get<DTrack>();

for(auto t : tracks){
  // ... do something with a track
}
```

## History

[JANA](https://halldweb.jlab.org/DocDB/0011/001133/002/Multithreading_lawrence.pdf) (JLab ANAlysis framework) 
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
- [JANA2 Framework for Event Based and Triggerless Data Processing](https://doi.org/10.1051/epjconf/202024501022)
- [Streaming readout for next generation electron scattering experiments](https://arxiv.org/abs/2202.03085)
- [Streaming Readout of the CLAS12 Forward Tagger Using TriDAS and JANA2](https://arxiv.org/abs/2104.11388)
- [Offsite Data Processing for the GlueX Experiment](https://doi.org/10.1051/epjconf/202024507037)


