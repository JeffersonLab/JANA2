Principles
=========

This section provides higher-level background and context for JANA, and discusses JANA’s design philosophy and the associated tradeoffs.

.. autosummary::
   :toctree: generated

.. JANA concepts:

JANA concepts
--------------
* JObjects are data containers for specific resuts, e.g. clusters or tracks. They may be plain-old structs or they may optionally inherit from (e.g.) ROOT or NumPy datatypes.

* JEventSources take a file or messaging producer which provides raw event data, and exposes it to JANA as a stream.

* JFactories calculate a specific result on an event-by-event basis. Their inputs may come from an EventSource or may be computed via other JFactories. All results are evaluated lazily and cached until the entire event is finished processing. in order to do so. Importantly, JFactories are decoupled from one another via the JEvent interface. It should make no difference to the JFactory where its input data came from, as long as it has the correct type and tag. While the `Factory Pattern <https://en.wikipedia.org/wiki/Factory_method_pattern>`_ usually abstracts away the subtype of the class being created, in our case it abstracts away the number of instances created instead. For instance, a ClusterFactory may take m Hit objects and produce n Cluster objects, where m and n vary per event and won’t be known until that event gets processed.

* JEventProcessors run desired JFactories over the event stream and write the results to an output file or messaging consumer. JFactories form a lazy directed acyclic graph, whereas JEventProcessors trigger their actual evaluation.

Object lifecycles
------------------
JObjects are data containers for specific resuts, e.g. clusters or tracks. They may be plain-old structs or they may optionally inherit from (e.g.) ROOT or NumPy datatypes.

By default, a JFactory owns all of the JObjects that it created during :py:func:`Process()`. Once all event processors have finished processing a :py:func:`JEvent`, all :py:func:`JFactories` associated with that :py:func:`JEvent` will clears and delete their :py:func:`JObjects`. However, you can change this behavior by setting one of the factory flags:

* :py:func:`PERSISTENT`: Objects are neither cleared nor deleted. This is usually used for calibrations and translation tables. Note that if an object is persistent, :py:func:`JFactory::Process` will not be re-run on the next :py:func:`JEvent`. The user may still update the objects manually, via :py:func:`JFactory::BeginRun`, and must delete the objects manually via :py:func:`JFactory::EndRun` or :py:func:`JFactory::Finish`.

* :py:func:`NOT_OBJECT_OWNER`: Objects are cleared from the :py:func:`JFactory` but not deleted. This is useful for “proxy” factories (which reorganize objects that are owned by a different factory) and for :py:func:`JEventGroups`. :py:func:`JFactory`::Process will be re-run for each :py:func:`JEvent`. As long as the objects are owned by a different :py:func:`JFactory`, the user doesn’t have to do any cleanup.

The lifetime of a :py:func:`JFactory` spans the time that a :py:func:`JEvent` is in-flight. No other guarantees are made: :py:func:`JFactories` might be re-used for multiple :py:func:`JEvents` for the sake of efficiency, but the implementation is free to not do so. In particular, the user must never assume that one :py:func:`JFactory` will see the entire :py:func:`JEvent` stream.

The lifetime of a :py:func:`JEventSource` spans the time that all of its emitted :py:func:`JEvents` are in-flight.

The lifetime of a :py:func:`JEventProcessor` spans the time that any :py:func:`JEventSources` are active.

The lifetime of a :py:func:`JService` not only spans the time that any :py:func:`JEventProcessors` are active, but also the lifetime of :py:func:`JApplication` itself. Furthermore, because JServices use :py:func:`shared_ptr`, they are allowed to live even longer than :py:func:`JApplication`, which is helpful for things like writing test cases.

Design philosophy
-----------------
JANA’s design philosophy can be boiled down to five values, ordered by importance:

Simple to use
______________
JANA chooses its battles carefully. First and foremost, JANA is about parallelizing computations over data organized into events. From a 30000-foot view, it should look more like OpenMP or Thread Building Blocks or RaftLib than like ROOT. Unlike the aforementioned, JANA’s vocabulary of abstractions is designed around the needs of physicists rather than general programmers. However, JANA does not attempt to meet *all* of the needs of physicists.

JANA recognizes when coinciding concerns ought to be handled orthogonally. A good example is persistence. JANA does not seek to provide its own persistence layer, nor does it require the user to commit to a specific dependency such as ROOT or Numpy or Apache Arrow. Instead, JANA tries to make it feasible for the user to choose their persistence layer independently. This way, if a collaboration decides they wish to (for instance) migrate from ROOT to Arrow, they have a well-defined migration path which keeps the core analysis code largely intact.

In particular, this means minimizing the complexity of the build system and minimizing orchestration. Building code against JANA should require nothing more than implementing certain key interfaces, adding a single path to includes, and linking against a single library.

Well-organized
______________
While JANA’s primary goal is running code in parallel, its secondary goal is imposing an organizing principle on the users’ codebase. This can be invaluable in a large collaboration where members vary in programming skill. Specifically, JANA organizes processing logic into decoupled units. JFactories are agnostic of how and when their prerequisites are computed, are only run when actually needed, and cache their results for reuse. Different analyses can coexist in separate JEventProcessors. Components can be compiled into independent plugins, to be mixed and matched at runtime. All together, JANA enforces an organizing principle that enables groups to develop and test their code with both freedom and discipline.

Safe
____
JANA recognizes that not all of its users are proficient parallel programmers, and it steers users towards patterns which mitigate some of the pitfalls. Specifically, it provides:

* **Modern C++ features** such as smart pointers and judicious templating, to discourage common classes of bugs. JANA seeks to make its memory ownership semantics explicit in the type system as much as possible.

* **Internally managed locks** to reduce the learning curve and discourage tricky parallelism bugs.

* **A stable API** with an effort towards backwards-compatibility, so that everybody can benefit from new features and performance/stability improvements.

Fast
_____
JANA uses low-level optimizations wherever it can in order to boost performance.

Flexible
_________
* Disentangling: Input data is bundled into blocks (each containing an array of entangled events) and we want to parse each block in order to emit a stream of events (*flatmap*)

* Software triggers: With streaming data readout, we may want to accept a stream of raw hit data and let JANA determine the event boundaries. Arbitrary triggers can be created using existing JFactories. (*windowed join*)

* Subevent-level parallelism: This is necessary if individual events are very large. It may also play a role in effectively utilizing a GPU, particularly as machine learning is adopted in reconstruction (*flatmap+merge*)

JANA is also flexible enough to be compiled and run different ways. Users may compile their code into a standalone executable, into one or more plugins which can be run by a generic executable, or run from a Jupyter notebook.

Comparison to other frameworks
--------------------------------
Many different event reconstruction frameworks exist. The following are frequently compared and contrasted with JANA:

* `Clara <https://claraweb.jlab.org/clara/>`_ While JANA specializes in thread-level parallelism, Clara uses node-level parallelism via a message-passing interface. This higher level of abstraction comes with some performance overhead and significant orchestration requirements. On the other hand, it can scale to larger problem sizes and support more general stream topologies. JANA is to OpenMP as Clara is to MPI.

* `Fun4all <https://github.com/sPHENIX-Collaboration/Fun4All>`_ Comparison coming soon!
