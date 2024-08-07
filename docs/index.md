
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

## Publications

Publications:
https://arxiv.org/abs/2202.03085 
https://doi.org/10.1051/epjconf/202125104011 Streaming Readout of the CLAS12 Forward Tagger Using TriDAS and JANA2

https://doi.org/10.1051/epjconf/202024507037 Offsite Data Processing for the GlueX Experiment

- [Running AI in JANA2](https://indico.cern.ch/event/1238718/contributions/5431992/attachments/2691231/4670186/2023-07-27_Running-AI-in-JANA2.pdf)
- [JANA2 Framework for Event Based and Triggerless Data Processing](https://doi.org/10.1051/epjconf/202024501022)
- [Streaming readout for next generation electron scattering experiments](https://arxiv.org/abs/2202.03085)
- [Streaming Readout of the CLAS12 Forward Tagger Using TriDAS and JANA2](https://arxiv.org/abs/2104.11388)
- [Offsite Data Processing for the GlueX Experiment](https://doi.org/10.1051/epjconf/202024507037)


