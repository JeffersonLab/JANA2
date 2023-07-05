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


Design philosophy
-----------------
* :py:func:`PERSISTENT`: Objects are neither cleared nor deleted. This is usually used for calibrations and translation tables. Note that if an object is persistent, :py:func:`JFactory::Process` will not be re-run on the next :py:func:``JEvent`. The user
may still update the objects manually, via :py:func:`JFactory::BeginRun`, and must delete the objects manually via :py:func:`JFactory::EndRun` or :py:func:`JFactory::Finish`.

* :py:func:`NOT_OBJECT_OWNER`: Objects are cleared from the :py:func:`JFactory` but not deleted. This is useful for “proxy” factories (which reorganize objects that are owned by a different factory) and for :py:func:`JEventGroups`. :py:func:`JFactory`::Process will be re-run for each :py:func:`JEvent`. As long as the objects are owned by a different :py:func:`JFactory`, the user doesn’t have to do any cleanup.

Comparison to other frameworks
--------------------------------
