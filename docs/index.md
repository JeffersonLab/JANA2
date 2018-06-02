---
title: JANA: Multi-threaded HEP Event Reconstruction
---

## Welcome to JANA!

JANA is a framework for multi-threaded HENP (High Energy and Nuclear Physics)  event reconstruction.

JANA is currently undergoing a complete rewrite. The new version will be JANA 2. The code is not ready for actual use yet, but you are free to browse around to see how progress is going. 

```
auto tracks = jevent->Get<DTrack>(tracks);

for(auto t : tracks){
  // ... do something with a track
}
```
