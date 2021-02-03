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

## Frequently Asked Questions

Have a question? [File an issue!](https://github.com/JeffersonLab/JANA2/issues)


### I want to compile JANA using a compiler other than the default one on my system, but CMake is ignoring it

CMake by design won't use `$PATH` to find the compiler. You either need to set the `CXX` environment variable or 
the `CMAKE_CXX_COMPILER` CMake variable. 

