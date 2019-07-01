

data StopwatchOp = Start | Measure | Stop

type Time = Integer
type ThreadCount = Integer
type EventCount = Integer
type Throughput = Double

data MetricsLog = MetricsLog { stopwatch_op :: StopwatchOp
                             , current_time :: Time
                             , event_count  :: EventCount
                             , thread_count :: ThreadCount
                             } deriving (Show)

data MetricsAcc = MetricsAcc { last_op      :: StopwatchOp
                             , start_time   :: Time
                             , last_time    :: Time
                             , delta_time   :: Time
                             , start_evts   :: EventCount
                             , last_evts    :: EventCount
                             , delta_evts   :: EventCount
                             , thread_count :: ThreadCount
                             } deriving (Show)

data MetricsSummary = MetricsSummary { avg_thru_hz  :: Throughput
                                     , last_thru_hz :: Throughput
                                     , thread_count :: ThreadCount
                                     , total_evts   :: EventCount
                                     } deriving (Show)

zero :: MetricsAcc
zero = MetricsAcc Stop 0 0 0 0 0

now :: Time
now = 22

combine :: MetricsAcc -> MetricsLog -> MetricsAcc

combine (MetricsAcc Stop _ _ _ _ _)
        (MetricsLog Start t nevt nthreads)
      = (MetricsAcc Start t t nevt nevt nthreads)

combine (MetricsAcc Start start_time last_time start_evts last_evts thread_count)
        (MetricsLog Measure t nevt nthreads)
      = (MetricsAcc Start start_time t start_evts nevt nthreads)

combine acc _ = acc

summarize :: MetricsAcc -> MetricsSummary
summarize a = MetricsSummary (