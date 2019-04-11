#pragma once

class Event {
  const int _run_id;
  const int _event_id;
  std::map<std::string, Creatable*> _created;

 public:
  Event(int run_id, int event_id) : _run_id(run_id), _event_id(event_id) {}

  template<typename T> T get(std::string name);
  bool put(std::string name, Creatable* value);

}
