
class JObject{}


class Cluster : JObject {
	double x,y,z,Etot;
}

class JCreatorBase {
}

template <typename T>
class JCreator<T> : JCreatorBase {

	virtual bool has_prereqs(const Event & e) = 0;
	virtual void create(const Event & e, T & result) = 0;

}

class DefaultClusterCreator : JCreator<Cluster> {
public:


	void create(const Event & e, Cluster & result) {
		auto a = e.Get<int>("something");
		auto b = e.Get<int>("sthelse");
		result.x = a;
		result.y = b;
	}
}





// ----------------------------

class Creatable {

public:

	std::vector<std::string> get_input_tags() = 0;
	std::string get_output_tag() = 0;
	void create(const Event & e) = 0;

}



class DetectorAClusterFromROOTFile : public Cluster, public Creatable {

public:

	std::vector<std::string> get_input_tags() {
		return {"detector_A_stuff", "detector_A_thingamabobs"};
	}

	std::string get_output_tag() {
		return "detector_A_cluster";
	}

	void create(const Event & e) {
		auto a = e.Get<vector<Hit>>("detectorAhits");
		auto b = e.Get<int>("detector_A_thingamabobs");
		x = a;
		y = b;
	}
}


class Event {

	const long runNumber;
	const long eventNumber;
	//JEventSource* evtSource;  // Not exposed to users!!! WANT THESE DECOUPLED
	std::map<std::string, JCreatable*> created;
	std::map<std::string, JObject*> provided;


public:

	Event(long runNumber, long eventNumber) : runNumber(runNumber), eventNumber(eventNumber) {};

	void put(std::string tag, JObject* item) {
		provided.insert({tag, item});
	}

	void put(std::string tag, JCreatable* item) {
		underlying.insert({tag, item});
		item->needs_create = true;
	}

	template<typename T> const T& get(std::string tag) {

		// static_assert that T <: JObject

		auto result = provided.find(tag);
		if (result == underlying.end()) {
			throw "Not found in Event!";
		}

		auto result = underlying.find(tag);
		if (result == underlying.end()) {
			throw "Not found in Event!";
		}

		auto result_as_t = dynamic_cast<T>(result);
		if (result_as_t == nullptr) {
			throw "tag is not of correct type";
		}

		if (result_as_t->needs_create) {
			result_as_t->create(this);
			result_as_t->needs_create = false;
		}

		return *result_as_t;
	}


};


enum class EventSourceStatus { NOT_READY, READY, EXHAUSTED }

class EventSource {

public:
	void init()=0;
	EventSourceStatus emit(Event& event)=0;
}









