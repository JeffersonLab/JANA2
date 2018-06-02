---
title: JANA: Multi-threaded HEP Event Reconstruction
---

## Welcome to JANA!

JANA is a framework for multi-threaded processing of event based data from accelerator based nuclear physics experiments.

JANA is currently undergoing a complete rewrite. the new version will be JANA 2. The code is not ready for actual use yet, but you are free to browse around to see how progress is going. 

```
auto tracks = jevent->Get<DTrack>(tracks);

for(auto t : tracks){
  // ... do something with a track
}
```
