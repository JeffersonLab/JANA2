---
title: JANA: Multi-threaded HENP Event Reconstruction
---

<center>
<table border="0" width="100%" align="center">
<TH width="20%"><A href="index.html">Welcome</A></TH>
<TH width="20%"><A href="Tutorial.html">Tutorial</A></TH>
<TH width="20%"><A href="Howto.html">How-to guides</A></TH>
<TH width="20%"><A href="Explanation.html">Principles</A></TH>
<TH width="20%"><A href="Reference.html">Reference</A></TH>
</table>
</center>

## Welcome to JANA!

JANA is a C++ framework for multi-threaded HENP (High Energy and Nuclear Physics)  event reconstruction.
It is very efficient at multi-threading with a design that makes it easy for less experienced programmers
to contribute pieces to the larger reconstruction project. The same JANA program can be used to easily
do partial or full reconstruction, fully maximizing the available cores for the current job.

Its design strives to be extremely easy to setup when first getting started, yet have a depth of customization
options that allow for more complicated reconstruction as your project grows. The intent is to make it easy to
run on a laptop for local code development, but to also be highly efficent when deploying to large computing
sites like [NERSC](http://www.nersc.gov/){:target="_blank"}.

JANA is currently undergoing a complete rewrite. The new version will be JANA 2. The code is not quite ready
for actual use yet, but you are free to browse around to see how progress is going. The project is 
[hosted on GitHub](https://github.com/JeffersonLab/JANA2)

```
auto tracks = jevent->Get<DTrack>(tracks);

for(auto t : tracks){
  // ... do something with a track
}
```
