---
title: JANA: Multi-threaded HENP Event Reconstruction
---

<center>
<table border="0" width="90%" align="center">
<TH width="25%"><href src="index.html">Home</href></TH>
<TH width="25%"><href src="GettingStarted.html">Getting Started</href></TH>
<TH width="25%"><href src="Download.html>Download</href></TH>
<TH width="25%"><href src="FAQ.html">FAQ</href></TH>
</table>
</center>

## Welcome to JANA!

JANA is a framework for multi-threaded HENP (High Energy and Nuclear Physics)  event reconstruction.
It is very efficient at multi-threading with a design that makes it easy for less experienced programmers
to contribute pieces to the larger reconstruction project. The same JANA program can be used to easily
do partial or full reconstruction, fully maximizing the available cores for the current job.

It's design strives to be extremely easy to setup when first getting started, yet have a depth of customization
options that allow for more complicated reconstruction as your project grows. The itent is to make it easy to
run on a laptop for local code development, but to be highly efficent when deploying to large computing
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
