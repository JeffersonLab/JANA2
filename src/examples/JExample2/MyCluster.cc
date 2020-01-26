
#include <numeric>

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>

#include "MyCluster.h"
#include "MyHit.h"


// This method gets called to actually produce the MyCluster objects for the event.
// We use "partial template specialization" to define this here in place of the default
// one defined in the templated JFactoryT class.
template<>
void JFactoryT<MyCluster>::Process(const std::shared_ptr<const JEvent>& aEvent)
{
	// Get the MyHit objects for the event. We can grab as many different types of
	// objects we need to here in order to make the MyCluster objects. For this
	// example, only the MyHit objects are needed. Note how we do not need to specify
	// whether these came from a JEventSource or another JFactory (i.e. algorithm).
	// The type of the hits variable is a vector<const MyHit*> here. Using the
	// "auto" type means we only need to type "MyHit" once, minimizing potential
	// type conflicts from copy/paste errors.
	auto hits = aEvent->Get<MyHit>();

	// For simplicity, split hits into 3 groups and we'll make a MyCluster from each
	std::vector< decltype(hits) > sorted_hits(3); // i.e. vector< vector<const MyHit*> > with 3 elements
	for(size_t i=0; i<hits.size(); i++){
		sorted_hits[i%3].push_back( hits[i] ); // copy pointer to MyHit to appropriate internal vector
	}
	
	// Loop over groups of hits and make a MyCluster of each
	for( auto vhits : sorted_hits ){  // loop over groups of hits (should be 3)
		
		if( vhits.empty() ) continue; // ignore empty groups
		
		// Fancy way of adding energy of all hits and averaging x and t
		double Etot = std::accumulate(vhits.begin(), vhits.end(), 0.0, [](double s, const MyHit *hit){return s+hit->E;} );
		double x_avg = std::accumulate(vhits.begin(), vhits.end(), 0.0, [](double s, const MyHit *hit){return s+hit->x;} )/(double)vhits.size();
		double t_avg = std::accumulate(vhits.begin(), vhits.end(), 0.0, [](double s, const MyHit *hit){return s+hit->t;} )/(double)vhits.size();

		// Create a new MyCluster object
		auto cluster = new MyCluster(x_avg, Etot, t_avg);
		
		// Objects of the type this factory creates (MyCluster in this case) should be
		// stored in the mData member of the JFactoryT class. Note that this transfers
		// ownership to the framework and JANA will take care of deleting these later.
		mData.push_back( cluster );

		// One may add JObjects associated with this one to its associated objects list.
		// This is a convenient way to keep the list of pointers to the MyHit objects
		// used to create this cluster. You may also choose to do this in a more visible
		// member variable if you prefer. Adding them as "associated objects" though makes
		// JANA aware of the association, unlocking other JANA features that make use of
		// that.
		for( auto hit : vhits ) cluster->AddAssociatedObject( hit );
	}
}


