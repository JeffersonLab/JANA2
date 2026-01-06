

![Dependency matrix tests](https://github.com/JeffersonLab/JANA2/actions/workflows/test_dependency_matrix.yml/badge.svg)

<br>

![ePIC integration tests](https://github.com/JeffersonLab/JANA2/actions/workflows/test_integration_epic.yml/badge.svg)

<br>

![GlueX integration tests](https://github.com/JeffersonLab/JANA2/actions/workflows/test_integration_gluex.yml/badge.svg)
<br>

[![DOI](https://zenodo.org/badge/117695469.svg)](https://zenodo.org/badge/latestdoi/117695469)
## Welcome to JANA!

JANA is a C++ framework for multi-threaded HENP (High Energy and Nuclear Physics) event reconstruction.
Please see the [JANA website](https://jeffersonlab.github.io/JANA2/) for full documentation.

JANA2 is a complete rewrite retaining successful features from the original while modernizing the framework for a new generation of experiments. The code is under active development, but is ready for use as is. You are welcome to 
check it out and give feedback to help us improve it.

Just to whet your appetite a little, the code snippet below is the most common signature of JANA. A large fraction of end users won't really need to know much more than this.

```c++
auto tracks = jevent->Get<DTrack>();

for(auto t : tracks){
  // ... do something with a track
}
```

To quickly download, build, install, and test on your system:

```bash
git clone https://github.com/JeffersonLab/JANA2
cd JANA2
mkdir build
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=`pwd`
cmake --build build --target install -j 8
bin/jana -Pplugins=JTest -Pjana:nevents=100
```

For a closer look, see our [tutorial](https://jeffersonlab.github.io/JANA2/#/tutorial).


