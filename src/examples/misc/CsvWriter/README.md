
CsvWriter
=========

CsvWriter demonstrates how to write an event processor that writes a number of output collections to a file.
For simplicity, each collection is represented as CSV with a little extra outer structure. Specifically, `####` lines separate events, `====` lines separate collections, and `----` lines separate headers from table bodies.

This example demonstrates how to generically work with JObjects without committing to a particular data model. Specifically, it uses `JFactory::GetAs()` and `JFactory::Summarize()` together in order to manipulate collections without having their concrete type. JANA does something similar internally for its `janacontrol` plugin and its interactive `JInspector`. It is useful to compare this to `TutorialProcessor`, which does know the exact types of its output collections.

This example requires that the data model classes inherit from `JObject`. If they don't, an exception will be thrown. 


