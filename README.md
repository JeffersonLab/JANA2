## Welcome to JANA!

JANA is a C++ framework for multi-threaded HENP (High Energy and Nuclear Physics)  event reconstruction.
Please see the [JANA website](https://jeffersonlab.github.io/JANA2/) for full documentation.

JANA is currently undergoing a complete rewrite. the new version will be JANA 2. The code is not ready for actual use yet, but you are free to 
check it out and give feedback as the project progresses.

```
auto tracks = jevent->Get<DTrack>(tracks);

for(auto t : tracks){
  // ... do something with a track
}
```
