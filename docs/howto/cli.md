
Using the JANA CLI
==================

Collaborations often choose to create their own executables that call JANA2 under the hood. However, if they are taking advantage of JANA2's plugin architecture, they also have the option to use JANA2's builtin CLI tool to run their plugins directly. JANA2's CLI tool is typically run like this:

~~~ bash
$JANA_HOME/bin/jana -Pplugins=JTest -Pnthreads=8 ~/data/inputfile.txt
~~~

Note that the JANA executable won't do anything until you provide plugins.
A simple plugin is provided called JTest, which verifies that everything is working and optionally does a quick
performance benchmark. Additional simple plugins that demonstrate different pieces of JANA2's functionality are provided in `src/examples`. Instructions on how to write your
own plugin are given in the Tutorial section.

Along with specifying plugins, you need to specify the input files containing the events you wish to process.
Note that JTest ignores these and crunches randomly generated data instead.


The built-in CLI tool accepts the following command-line flags:

| Short | Long | Meaning  |
|:------|:-----|:---------|
| -h    | --help               | Display help message |
| -v    | --version            | Display version information |
| -c    | --configs            | Display configuration parameters |
| -l    | --loadconfigs <file> | Load configuration parameters from file |
| -d    | --dumpconfigs <file> | Dump configuration parameters to file |
| -b    | --benchmark          | Run JANA in benchmark mode |
| -P    |                      | Specify a configuration parameter (see below) |


