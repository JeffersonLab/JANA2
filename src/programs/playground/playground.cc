
#include <iostream>
#include <JANA/JThread.h>
#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>

using namespace std;

struct MyEvent : public JEvent {
};

struct Hit : public JObject {
	
	int t;
	double v;
	double x;
	double y;
	double z;

	Hit(int t, double v, double x, double y, double z) :
		t(t), v(v), x(x), y(y), z(z) {};

	friend ostream& operator<<(ostream& dest, Hit& e) {
		dest << e.t << ":" << e.v << 
			" @ (" << e.x << "," << e.y << "," << e.z << ")";
		return dest;
	}
};

struct Cluster : public JObject {
	double mean_x;
	double mean_y;
	double mean_z;
	double Etot;

	Cluster() {};
	Cluster(double mx, double my, double mz, double e) : 
		mean_x(mx), mean_y(my), mean_z(mz), Etot(e) {};

	friend ostream& operator<<(ostream& dest, Cluster& c) {
		dest << c.Etot << "@ (" << c.mean_x << "," << c.mean_y 
			<< "," << c.mean_z << ")";
		return dest;
	}
};

class HitFactory : public JFactoryT<Hit> {

	virtual void Init() {
		cout << "HitFactory.init()" << endl;

		vector<Hit*> dummydata;
		dummydata.push_back(new Hit {0, 0.1, 2.0, 0.0, 9.0});
		dummydata.push_back(new Hit {1, 0.2, 1.0, 1.0, 0.0});
		dummydata.push_back(new Hit {2, 0.4, 0.0, 0.0, 1.0});
		dummydata.push_back(new Hit {3, 0.1, 1.0, 0.0, 0.0});
		dummydata.push_back(new Hit {4, 0.3, 1.0, 0.0, 0.0});

		Set(move(dummydata));
		SetCreated(true);
	}
};

class ClusterFactory : public JFactoryT<Cluster> {

	virtual void Process(const shared_ptr<const JEvent>& event) {
		cout << "ClusterFactory.process()" << endl;

		auto cluster = new Cluster(); 
		auto hits = event->Get<Hit>();
		int n = 0;
		for (auto it = hits.first; it != hits.second; ++it) {
			auto hit = *it;
			cluster->mean_x += hit->x;
			cluster->mean_y += hit->y;
			cluster->mean_z += hit->z;
			cluster->Etot += hit->v;
			++n;
		}
		cluster->mean_x /= n;
		cluster->mean_y /= n;
		cluster->mean_z /= n;
		mData.push_back(cluster); 
		// Or call Set()...
		// Factory is responsible for deletion
	}
};


#include<JANA/JLogger.h>
#include<JANA/JCpuInfo.h>

int main(int narg, char *argv[]) {

	std::cout << "Core id: " << JCpuInfo::GetCpuID() 
		  << " of " << JCpuInfo::GetNumCpus() << std::endl;
	std::cout << "Numa node: " << JCpuInfo::GetNumaNodeID() 
		  << " of " << JCpuInfo::GetNumNumaNodes() << std::endl;
	return 0;
	std::shared_ptr<JLogger> logger(new JLogger());

	LOG_INFO(logger) << "Launching a minimal JANA instance!" << LOG_END;
	LOG_DEBUG(logger) << "You shouldn't see this" << LOG_END;
	LOG_ERROR(logger) << "Here is an error message!" << LOG_END;

	JLogMessage(logger, JLogLevel::WARN) << "This also works" << JLogMessageEnd();
	LOG_FATAL(shared_ptr<JLogger>(new JLogger())) << "Exiting now!" << LOG_END;

	auto app = new JApplication();
	InitJANAPlugin(app);
	auto event = make_shared<JEvent>(app);

	JFactorySet factories;
	factories.Add(new ClusterFactory());
	factories.Add(new HitFactory());

	event->SetFactorySet(&factories);

	Cluster* result = *(event->Get<Cluster>().first);
	cout << *result << endl;
	delete app;
	return 0;
}




