
#include <JANA/JApplication.h>
#include <JANA/Components/JOmniFactory.h>
#include <JANA/Components/JOmniFactoryGeneratorT.h>
#include <PodioDatamodel/ExampleClusterCollection.h>


struct PodioClusteringConfig {
    double scale = 1.0;
    double offset = 0.0;
};

struct PodioClusteringFactory : public JOmniFactory<PodioClusteringFactory, PodioClusteringConfig> {

    PodioInput<ExampleCluster> m_protoclusters_in {this};
    PodioOutput<ExampleCluster> m_clusters_out {this};

    ParameterRef<double> m_scale {this, "scale", config().scale, "Scaling factor"};
    ParameterRef<double> m_offset {this, "offset", config().offset, "Amount to offset [mm]"};

    void Configure() {
    }

    void ChangeRun(int32_t /*run_nr*/) {
    }

    void Execute(int32_t /*run_nr*/, uint64_t /*evt_nr*/) {

        auto cs = std::make_unique<ExampleClusterCollection>();

        for (auto protocluster : *m_protoclusters_in()) {
            auto cluster = cs->create();
            cluster.energy((m_scale() * protocluster.energy()) + m_offset());
        }

        m_clusters_out() = std::move(cs);
    }
};


