



Work package: GPU arrows
------------------------

Problem: Arrows can only have _one_ next_input at any given time
Solution:
- GPU tasks can all be pushed by GPUArrow to _one_ queue per physical GPU

Design:
- GPUMapArrow
    - Knows which factory needs the GPU
    - Knows which factories need to run upstream
    - Knows how to pack and unpack the offloaded data

- GPUTapArrow
    - Knows what to do with the packed input data. How does it know this? 
    - Doesn't worry about pipelining the memory movement for now

- GPUFactory
    - I'm hoping we can reuse Inputs and Outputs for pack() and unpack(), because
      they need to be in the JEvent _somehow_


Problem: GPUTapArrow doesn't know what to do! Needs some kind of indication of
    - What factory needs to be run on this thing?
    - What arrow (what queue, technically) does the JEvent need to be returned to?
    
	One of the reasons this is a problem is because JEvent databundles are supposed to 
	be immutable, so this cannot be a factory output. Probably has to be a separate
	field on the JEvent. Who sets this field, though? And via what interface?

	Note that the GPUMapArrow knows the factory and output queue (which MIGHT just be
	its own input queue. Is this always the case?). Maybe GPUMapArrow should set this.

	So what should this be? (databundle_name, next_input_queue). 
	Quickly check whether next_input_queue is a arrow-local port index or a topology-wide queue id.
	- Sadly it is an arrow-local port index
	- The queue doesn't know it's own id
	- So it's the responsibility of the TopologyBuilder to inform the GPUTapArrow what the output queue index is
	- OR we only store the factoryname, and the GPUTapArrow looks up the correct port index for the given factoryname

Problem:
I'm not sure if the concept of a continuation name is general enough to stick onto JEvent. Let's
think about the calibration workflow a little more. That might require re-running factories that have already run,
which poses a problem for immutability, but which we might be able to solve via event levels. 


Suppose I unroll the loop and avoid having continuations. What a relief! Now each GPUTapArrow knows exactly what to do. 
However, now I need to do a lot more in my JTopologyBuilder (to figure out this unroll) and my scheduler needs to enforce
mutual exclusion _across_ arrows. Both of these are very unpleasant to implement. I might need them eventually. But not today.


Resuming this!
Goal: Check whether I call Preprocess on a JFactory




  










