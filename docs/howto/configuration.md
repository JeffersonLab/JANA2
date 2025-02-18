
Configuring JANA
================

JANA provides a parameter manager so that configuration options may be controlled via code, command-line args, and 
config files in a consistent and self-documenting way. Plugins are free to request any existing parameters or register
their own. 

The following configuration options are used most commonly:

| Name | Type | Description |
|:-----|:-----|:------------|
| nthreads                  | int     | Size of thread team (Defaults to the number of cores on your machine) |
| plugins                   | string  | Comma-separated list of plugin filenames. JANA will look for these on the `$JANA_PLUGIN_PATH` |
| plugins_to_ignore         | string  | This removes plugins which had been specified in `plugins`. |
| event_source_type         | string  | Manually specify which JEventSource to use |
| jana:nevents              | int     | Limit the number of events each source may emit |
| jana:nskip                | int     | Skip processing the first n events from each event source |
| jana:status_fname         | string  | Named pipe for retrieving status information remotely |
| jana:loglevel | string | Set the log level (trace,debug,info,warn,error,fatal,off) for loggers internal to JANA |
| jana:global_loglevel | string | Set the default log level (trace,debug,info,warn,error,fatal,off) for all loggers |
| jana:show_ticker          | bool    | Controls whether the status ticker is shown |
| jana:ticker_interval          | int     | Controls how often the status ticker updates (in ms)  |
| jana:extended_report       | bool    | Controls whether to show extra details in the status ticker and final report |

JANA automatically provides each component with its own logger. You can control the logging verbosity of individual components
just like any other parameter. For instance, if your component prefixes its parameters with `BCAL:tracking`,
you can set its log level to `DEBUG` by setting `BCAL:tracking:loglevel=debug`. You can set the parameter prefix by calling `SetPrefix()`,
and if you need the logger name to differ from the parameter prefix for any reason, you can override it by calling `SetLoggerName()`.


The `JTest` plugin lets you test JANA's performance for different workloads. It simulates a typical reconstruction pipeline with four stages: parsing, disentangling, tracking, and plotting. Parsing and plotting are sequential, whereas disentangling and tracking are parallel. Each stage reads all of the data written during the previous stage. The time spent and bytes written (and random variation thereof) are set using the following parameters:
 
| Name | Type | Default | Description |
|:-----|:-----|:------------|:--------|
| jtest:parser:cputime_ms | int | 0 | Time spent during parsing |
| jtest:parser:cputime_spread | int | 0.25 | Spread of time spent during parsing |
| jtest:parser:bytes | int | 2000000 | Bytes written during parsing |
| jtest:parser:bytes_spread | double | 0.25 | Spread of bytes written during parsing |
| jtest:disentangler:cputime_ms | int | 20 | Time spent during disentangling |
| jtest:disentangler:cputime_spread | double | 0.25 | Spread of time spent during disentangling |
| jtest:disentangler:bytes | int | 500000 | Bytes written during disentangling |
| jtest:disentangler:bytes_spread | double | 0.25 | Spread of bytes written during disentangling |
| jtest:tracker:cputime_ms | int | 200 | Time spent during tracking |
| jtest:tracker:cputime_spread | double | 0.25 | Spread of time spent during tracking |
| jtest:tracker:bytes | int | 1000 | Bytes written during tracking |
| jtest:tracker:bytes_spread | double | 0.25 | Spread of bytes written during tracking |
| jtest:plotter:cputime_ms | int | 0 | Time spent during plotting |
| jtest:plotter:cputime_spread | double | 0.25 | Spread of time spent during plotting |
| jtest:plotter:bytes | int | 1000 | Bytes written during plotting |
| jtest:plotter:bytes_spread | double | 0.25 | Spread of bytes written during plotting |



The following parameters are used for benchmarking:

| Name | Type | Default | Description |
|:-----|:-----|:------------|:--------|
| benchmark:nsamples    | int    | 15 | Number of measurements made for each thread count |
| benchmark:minthreads  | int    | 1  | Minimum thread count |
| benchmark:maxthreads  | int    | ncores | Maximum thread count |
| benchmark:threadstep  | int    | 1  | Thread count increment |
| benchmark:resultsdir  | string | JANA_Test_Results | Directory name for benchmark test results |


The following parameters are more advanced, but may come in handy when doing performance tuning:

| Name | Type | Default | Description |
|:-----|:-----|:------------|:--------|
| jana:max_inflight_events          | int  | nthreads  | The number of events which may be in-flight at once. Should be at least `nthreads`, more gives better load balancing. |
| jana:affinity                     | int  | 0         | Thread pinning strategy. 0: None. 1: Minimize number of memory localities. 2: Minimize number of hyperthreads. |
| jana:locality                     | int  | 0         | Memory locality strategy. 0: Global. 1: Socket-local. 2: Numa-domain-local. 3. Core-local. 4. Cpu-local |
| jana:enable_stealing              | bool | 0         | Allow threads to pick up work from a different memory location if their local mailbox is empty. |
