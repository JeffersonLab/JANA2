![C/C++ CI ubuntu-18.04](https://github.com/JeffersonLab/JANA2/workflows/C/C++%20CI%20ubuntu-18.04/badge.svg)<br>
![C/C++ CI macOS-latest](https://github.com/JeffersonLab/JANA2/workflows/C/C++%20CI%20macOS-latest/badge.svg)
## Welcome to JANA!

JANA is a C++ framework for multi-threaded HENP (High Energy and Nuclear Physics)  event reconstruction.
Please see the [JANA website](https://jeffersonlab.github.io/JANA2/) for full documentation.

JANA2 is a complete rewrite retaining successful features from the original while modernizing the framework for a new generation of experiments. The code is under active development, but is ready for use as is. You are welcome to 
check it out and give feedback to help us improve it.

Just to whet your appetite a little, the code snippet below is the most common signature of JANA. A large fraction of end users won't really need to know much more than this.

```
auto tracks = jevent->Get<DTrack>(tracks);

for(auto t : tracks){
  // ... do something with a track
}
```
