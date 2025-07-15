
#include <jana2_tutorial_podio_datamodel/CalorimeterClusterCollection.h>

#include <JANA/Components/JOmniFactory.h>
#include <JANA/Calibrations/JCalibrationManager.h>


struct CalorimeterCluster_factory_filtered:
    public JOmniFactory<CalorimeterCluster_factory_filtered> {

    PodioInput<CalorimeterCluster> m_clusters_in {this};
    PodioOutput<CalorimeterCluster> m_clusters_out {this};

    Service<JCalibrationManager> m_calib_manager {this};
    double threshold = 100.0;

    void Configure() {}

    void ChangeRun(int32_t run_nr) {
        m_clusters_out.SetUniqueName("filtered_clusters");
        m_calib_manager->GetJCalibration(run_nr)
                       ->Get("MyCAL/cluster_threshold", threshold);
    }

    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {
        m_clusters_out()->setSubsetCollection(true);
        for (auto cluster : *m_clusters_in()) {
            if (cluster.getEnergy() >= threshold) {
                m_clusters_out()->push_back(cluster);
            }
        }
    }
};


