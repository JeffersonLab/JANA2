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

## Problem domain

![Alt JANA Simple system with a single queue](images/queues1.png)
![Alt JANA system with multiple queues](images/queues2.png)

## Parallelism concept

## Factories

![Alt JANA Factory Model](images/factory_model.png)

## Event sources

## Event processors

## More complex topologies

### JObjects

JObjects are data containers for specific results. JObjects are close to being plain-old structs, except they include
some extra functionality for creating and traversing associations with other JObjects. They are immutable once 
outside of the JFactory created them. It may be beneficial to use multiple inheritance in order to take gain additional
 functionality, e.g. to delegate persistence to ROOT.

### JEventSource

