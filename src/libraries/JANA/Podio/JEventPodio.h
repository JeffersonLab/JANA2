
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEVENTPODIO_H
#define JANA2_JEVENTPODIO_H

template <typename T>
podio::CollectionBase* GetCollection(std::string tag, const std::shared_ptr<const JEvent>& event) {
    auto fac = event->GetFactory<T>(tag, true);
    


}
#endif //JANA2_JEVENTPODIO_H
