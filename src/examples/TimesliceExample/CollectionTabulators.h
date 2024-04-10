#pragma once

#include "DatamodelGlue.h"
#include <JANA/Utils/JTablePrinter.h>

JTablePrinter TabulateClusters(const ExampleClusterCollection* c);

JTablePrinter TabulateHits(const ExampleHitCollection* c);


