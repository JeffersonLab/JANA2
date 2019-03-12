#pragma once
#include <JANA3/Component.h>
#include <JANA3/Queue.h>

using std::vector, std::map, std::string, std::set;

namespace futurejana {

  struct Topology {

    map<string, Queue> queues;
    vector<Arrow*> components;

  private:
    void link(Arrow& arrow, vector<string> input_queue_names, vector<string> output_queue_names) {

      for (string& name : input_queue_names) {
        queues[name].consumers.push_back(&arrow);
      }
      for (string& name : output_queue_names) {
        queues[name].producers.push_back(&arrow);
      }
      components.push_back(&arrow);

      // TODO: Who owns arrow?
    }

  public:

    void configure_queue(string queue_name, size_t empty_threshold, size_t full_threshold) {

      queues[queue_name].empty_threshold = empty_threshold;
      queues[queue_name].full_threshold = full_threshold;
    }

    void add(SourceArrow a) {
      link(a, {}, {a.get_output_queue_name()});
    }

    void add(SinkArrow a) {
      link(a, {a.get_input_queue_name()}, {});
    }

    void add(ReduceArrow a) {
      link(a, {a.get_input_queue_name()}, {a.get_output_queue_name()});
    }

    void add(MapArrow a) {
      link(a, {a.get_input_queue_name()}, {a.get_output_queue_name()});
    }

    void add(GatherArrow a) {
      link(a, a.get_input_queue_names(), {a.get_output_queue_name()});
    }

    void add(ScatterArrow a) {
      link(a, {a.get_input_queue_name()}, a.get_output_queue_names());
    }
  };
}



