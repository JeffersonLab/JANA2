

struct JContinuation {
  mutable int next_gpu_factory=0; // Who increments this?
  mutable std::string factory_prefix; // Or do we want databundle unique_name?
  mutable int originating_arrow_id; // Use this to figure out where to return the event to
};


