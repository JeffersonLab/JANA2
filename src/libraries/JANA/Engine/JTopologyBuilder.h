
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JTOPOLOGYBUILDER_H
#define JANA2_JTOPOLOGYBUILDER_H

#include <JANA/Engine/JArrowTopology.h>
#include <memory>


class JTopologyBuilder : public JService {

	std::shared_ptr<JParameterManager> m_params;
	std::shared_ptr<JComponentManager> m_components;

public:

	virtual void acquire_services(JServiceLocator* sl) {
		m_components = sl->get<JComponentManager>();
		m_params = sl->get<JParameterManager>();
	};

	virtual JArrowTopology* build(int nthreads);

};


#endif //JANA2_JTOPOLOGYBUILDER_H
