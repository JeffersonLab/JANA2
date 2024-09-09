#pragma once

#include <PodioDatamodel/ExampleClusterCollection.h>
#include <PodioDatamodel/ExampleHitCollection.h>
#include <JANA/Utils/JTablePrinter.h>

JTablePrinter TabulateClusters(const ExampleClusterCollection* c);

JTablePrinter TabulateHits(const ExampleHitCollection* c);


