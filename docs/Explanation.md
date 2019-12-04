
Explanation
===========

This section provides higher-level background and context for JANA, and discusses JANA's design philosophy and the
associated tradeoffs.

Many different event reconstruction frameworks exist. In particular, 

- Clara
- Fun4all


JANA's design philosophy can be boiled down to five values, ranked 

1. Simple to use

JANA chooses its battles carefully. First and foremost, JANA is about parallelizing computations over data organized
into events. From a 30000-foot view, it should look more like OpenMP or Thread Building Blocks or RaftLib than like ROOT. 
Unlike the aforementioned, JANA's vocabulary of abstractions is designed around the needs of physicists rather than 
general programmers. However, JANA does not attempt to meet _all_ of the needs of physicists.

JANA recognizes when coinciding concerns ought to be handled orthogonally. A good example is persistence. JANA does not
seek to provide its own persistence layer, nor does it require the user to commit to a specific dependency such as ROOT
or Numpy or Apache Arrow. Instead, JANA tries to make it feasible for the user to choose their persistence layer independently.
This way, if a collaboration decides they wish to (for instance) migrate from ROOT to Arrow, they have a well-defined migration
path which keeps the core analysis code largely intact.

In particular, this means minimizing the complexity of the build system and minimizing orchestration. Building code
against JANA should require nothing more than implementing certain key interfaces, adding a single path to includes,
and linking against a single library. 

2. Well-organized

While JANA's primary goal is running code in parallel, its secondary goal is imposing an organizing principle on
the users' codebase. This can be invaluable in a large collaboration where members vary in programming skill.

- Separation of concerns
- Dependency Injection
- 


3. Safe

JANA recognizes that not all of its users are proficient parallel programmers, and it steers users towards patterns which
mitigate some of the pitfalls. Although the C++ type system isn't strong enough to statically guarantee data race or 
memory safety, cleanly separating per-event state from global state and explicitly describing object ownership semantics

4. Fast

JANA uses low-level tricks wherever it can in order to boost performance. 

5. Flexible

JANA seeks to be usable in a variety of different ways. Although originally designed around offline processing of 
batched events, it also supports soft real time streaming of events. 

Similarly, user code can be compiled into a standalone executable, compiled into one or more plugins which can be
run by a generic executable, or run from a Jupyter notebook. 

Since flexibility has the lowest priority

