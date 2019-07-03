
## JANA ZeroMQ Plugin

zmq2jana enables JANA users to stream data from a ZeroMQ queue. It currently uses pub/sub and a message format which is 
based around the needs of INDRA.

It can be used in two different ways:
* Receive hits from different detectors and process each hit independently as its own "event"
* Receive hits from different detectors and automatically build events containing all hits within a configurable time 
interval

Support for building events using only 'fast' detector hits, applying custom filters, and then merging with 'slow'
detector hits is planned.


###Usage

zmq2jana is a plain old plugin. While it aims to be relatively generic, it is _not_ an official part of the JANA
API and is subject to breaking changes. It is recommended to copy the `zmq2jana` directory into your own project's 
repository and modify as needed. To compile, you will need to have installed `libzmq` and `zmq.hpp`. The simplest 
way to test your setup is as follows:

```bash
$ cd JANA2
$ scons -j4
$ $JANA_HOME/bin/jana -Pplugins=ZmqMain
```

The entry point is `ZmqMain.cc`, which registers different components with the JANA framework.

### Components

The different components available are:


##### JZmqEventSource
A `JEventSource` which polls a ZeroMQ socket and emits `JEvent`s which fuse hits across all detectors over a 
configurable time interval. This uses `JEventBuilder` under the hood.

##### ZmqHitSource
A `JEventSource` which polls a ZeroMQ socket and emits degenerate `JEvent`s, each containing a single detector hit.

##### DummyZmqPublisher
A ZeroMQ producer which randomly generates hits in the `ZmqMessage` format and publishes them to a configurable
ZeroMQ socket. This can be run either in a separate thread, or in a separate program entirely.

##### DummyHitExtractor
A `JFactory` which reads `ZmqMessage`s and produces `DummyHit`s tagged as `'raw_hits'`. This is a simulacrum of 
real parsing work.

##### DummyHitCalibrator
A `JFactory` which reads `DummyHit`s and produces modified `DummyHits` tagged as `'calibrated_hits'`. This is a 
simulacrum of real calibration work.



