#pragma once
#include <vector>
#include <map>

using std::string, std::vector, std::multimap, std::pair;


struct Arrow {
  int chunksize = 1;
  bool finished = false;
  
  virtual void execute(Topology& t);
};

// Sequential, stateful operations

struct SourceArrow : Arrow {
  virtual vector<Event> inprocess() = 0;
  virtual void init() = 0;
  virtual void finish() = 0;
  virtual string get_output_queue_name() = 0;
  virtual bool is_finished() = 0;
};

struct SinkArrow : Arrow {
  virtual void outprocess(vector<Event>) = 0;
  virtual void init() = 0;
  virtual void finish() = 0;
  virtual string get_input_queue_name() = 0;
  virtual bool is_finished() = 0;
}

struct ReduceArrow : Arrow {
  virtual vector<Event> reduce(vector<Event> e) = 0;
  virtual void init() = 0;
  virtual vector<Event> finish() = 0;
  virtual string get_input_queue_name() = 0;
  virtual string get_output_queue_name() = 0;
  virtual bool is_finished() = 0;
};

// Parallel, stateless operations

struct MapArrow : Arrow {
  virtual vector<Event> map(vector<Event> e) = 0;
  virtual string get_input_queue_name() = 0;
  virtual string get_output_queue_name() = 0;
};

struct ScatterArrow : Arrow {
  virtual multimap<string, Event> scatter(vector<Event> e) = 0;
  virtual string get_input_queue_name() = 0;
  virtual vector<string> get_output_queue_names() = 0;
};

struct GatherArrow : Arrow {
  virtual vector<Event> gather(multimap<string, Event> e) = 0;
  virtual vector<string> get_input_queue_names() = 0;
  virtual string get_output_queue_name() = 0;
};


