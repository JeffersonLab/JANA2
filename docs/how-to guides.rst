How-to...
=============
This section walks the user through specific steps for solving a real-world problem.

Table of contents
-----------------
1. Download and install JANA
2. Use the JANA command-line program
3. Configure JANA
4. Benchmark a JANA program
5. Generate code skeletons for projects, plugins, components, etc
6. Run the JANA Status/Control/Debugger GUI
7. Work with factory metadata for collecting statistics, etc
8. Create a service which can be shared between different plugins
9. Handle both real and simulated data
10. Handle EPICS data
11. Detect when a group of events has finished
12. Use JANA with ROOT
13. Persist the entire DST using ROOT
14. Checkpoint the entire DST using ROOT
15. Stream data to and from JANA
16. Build and filter events (“L1 and L2 triggers”)
17. Process subevents
18. Migrate from JANA1 to JANA2

Using the JANA CLI
-------------------
JANA is typically run like this:
.. code-block:: console 

  jana -Pplugins=JTest -b   # (cancel with Ctrl-C)


Note that the JANA executable won’t do anything until you provide plugins. A simple plugin is provided called JTest, which verifies that everything is working and optionally does a quick performance benchmark. Additional simple plugins are provided in py:func:`src/examples`. Instructions on how to write your own are given in the Tutorial section.

Along with specifying plugins, you need to specify the input files containing the events you wish to process. Note that JTest ignores these and crunches randomly generated data instead.

The command-line flags are:

