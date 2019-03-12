#pragma once

namespace futurejana {

typedef Event task_t;


void MapArrow::execute(Topology& topology) {

  Queue& input_queue = topology.queues[get_input_queue_name()];

  vector<task_t> tasks = input_queue.pop(x.chunksize);

  if (tasks.size() == 0) return;

  vector<task_t> results = map(tasks);

  Queue& output_queue = topology.queues[get_output_queue_name()];

  output_queue.push(results);
}


void ScatterArrow::execute(Topology& topology) {

  Queue& input_queue = topology.queues[get_input_queue_name()];

  vector<task_t> tasks = input_queue.pop(x.chunksize);

  if (tasks.size() == 0) return;

  multimap<string, task_t> results = scatter(tasks);

  for (auto item : results) {
    string output_queue_name = item.first;
    vector<task_t> tasks = item.second;
    Queue& output_queue = t.queues[output_queue_name];
    output_queue.push(tasks);

  }
}

}

