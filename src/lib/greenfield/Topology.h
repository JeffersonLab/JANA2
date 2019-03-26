#pragma once

#include <greenfield/Arrow.h>
#include <greenfield/Queue.h>
#include <greenfield/Components.h>
#include <greenfield/JLogger.h>


namespace greenfield {

    struct Topology {


        struct QueueStatus {
            uint32_t queue_id;
            uint64_t message_count;
            uint64_t message_count_threshold;
            bool is_finished;
        };


        struct ArrowStatus {
            uint32_t arrow_id;
            std::string arrow_name;
            bool is_finished;
            bool is_parallel;
            uint32_t thread_count;
            uint64_t messages_completed;
            double short_term_avg_latency;
            double long_term_avg_latency;
        };


        /// Outward interface:

        ///   finalize()               <= Could make this a Ctor and make EventTopology
        ///                               be a Builder instead of a subclass

        ///   get_queue_status()
        ///   get_arrow_status()
        ///   get_graph()              <= use these to build an interactive visual

        ///   log_queue_status()       <= use these for debugging test cases
        ///   log_arrow_status()

        ///   run_arrow(string arrow_name)       <= use these for debugging your scientific code
        ///   run_message(string source_name)
        ///   run_sequentially()
        ///   run()                              <= Chooses scheduler from parameter?!


        /// Inward interface:

        ///   report_arrow_finished(Arrow* arrow)
        ///   update(Arrow* arrow, double latency, uint64_t event_count)
        ///   get_num_threads(Arrow* arrow)  ???

        std::map<std::string, Arrow*> arrows;
        std::vector<QueueBase*> queues;
        std::vector<Component*> components;
        std::shared_ptr<JLogger> logger;

        std::vector<ArrowStatus> _arrow_statuses;
        std::vector<bool> finished_queues;
        std::vector<bool> finished_matrix;
        int arrow_count;
        int queue_count;


        ~Topology() {

            for (auto component : components) {
                // Topology owns _some_ components, but not necessarily all.
                delete component;
            }
            for (auto pair : arrows) {
                // Topology owns all arrows.
                delete pair.second;
            }
            for (auto queue : queues) {
                // Topology owns all queues.
                delete queue;
            }
        }



        void finalize()  {
            queue_count = queues.size();
            arrow_count = arrows.size();


            for (int i=0; i<queue_count; ++i) {
                finished_queues.push_back(false);
            }
            for (int i=0; i<queue_count*arrow_count; ++i) {
                finished_matrix.push_back(true);
            }

            LOG_INFO(logger) << "Set up finished queues, matrix data structures" << LOG_END;

            for (auto & pair : arrows) {
                auto arrow = pair.second;
                if (!arrow->is_finished()) {
                    for (QueueBase* queue : arrow->get_output_queues()) {
                        LOG_INFO(logger) << "Found queue " << queue->get_id() << LOG_END;
                        finished_matrix[arrow->get_index()*queue_count + queue->get_id()] = false;
                    }
                }
            }
            LOG_INFO(logger) << "Traversed graph" << LOG_END;
        }
        void report_arrow_finished(Arrow* arrow) {

            LOG_DEBUG(logger) << "Arrow reported finished: " << arrow->get_name() << LOG_END;
            // once an arrow is finished, any downstream queues may finish once they empty
            int arrow_id = arrow->get_index();
            _arrow_statuses[arrow_id].is_finished = true; // TODO: Can I please get rid of arrow->is_finished()?
            for (int qi = 0; qi < queue_count; ++qi) {
                finished_matrix[arrow_id*queue_count + qi] = true;
            }
            // for each unfinished queue, check if finished (forall arrows)
            for (int qi = 0; qi < queue_count; ++qi) {
                bool finished = true;
                for (int ai = 0; ai < arrow_count; ++ai) {
                    finished &= finished_matrix[ai*queue_count+qi];
                }
                if (finished) {
                    LOG_INFO(logger) << "Topology determined that a queue is finished: " << qi << LOG_END;
                    queues[qi]->set_finished(true);
                    finished_queues[qi] = true;
                }
            }

        }

        void update(Arrow* arrow, StreamStatus last_result, double latency, uint64_t messages_completed) {

            ArrowStatus& arrowStatus = _arrow_statuses[arrow->get_index()];
            arrowStatus.messages_completed += messages_completed;
            arrowStatus.long_term_avg_latency += latency;
            arrowStatus.short_term_avg_latency = latency / messages_completed;
            if (last_result == StreamStatus::Finished && arrowStatus.thread_count == 0) {
                report_arrow_finished(arrow);
            }
        }



        std::vector<QueueStatus> get_queue_status() {
            std::vector<QueueStatus> statuses;
            int i=0;
            for (QueueBase* q : queues) {
                QueueStatus qs;
                qs.queue_id = i++;
                qs.is_finished = q->is_finished();
                qs.message_count = q->get_item_count();
                qs.message_count_threshold = q->get_threshold();
                statuses.push_back(qs);
            }
            return statuses;
        }


        void log_queue_status() {
            LOG_INFO(logger) << "  +------+----------+-----------+----------+" << LOG_END;
            LOG_INFO(logger) << "  |  ID  |  Height  | Threshold | Finished |" << LOG_END;
            LOG_INFO(logger) << "  +------+----------+-----------+----------+" << LOG_END;
            for (QueueStatus& qs : get_queue_status()) {
                LOG_INFO(logger) << "  | "
                                 << std::setw(4) << qs.queue_id << " | "
                                 << std::setw(8) << qs.message_count << " | "
                                 << std::setw(9) << qs.message_count_threshold << " | "
                                 << std::setw(8) << qs.is_finished << " |" << LOG_END;
            }
            LOG_INFO(logger) << "  +------+----------+-----------+----------+" << LOG_END;
        }






        std::vector<ArrowStatus> get_arrow_status() {

            std::vector<ArrowStatus> metrics;

            for (auto & status : _arrow_statuses) {
                metrics.push_back(status);
            }

            for (auto & metric : metrics) {
                // Convert from running total to average
                metric.long_term_avg_latency /= metric.messages_completed;
            }
            return metrics;
        }


        void log_arrow_status() {
            LOG_INFO(logger) << "  +------+--------------------------------+----------+---------+--------------------+-----------------+----------------+----------+" << LOG_END;
            LOG_INFO(logger) << "  |  ID  |              Name              | Parallel | Threads | Messages completed | Latency (short) | Latency (long) | Finished |" << LOG_END;
            LOG_INFO(logger) << "  +------+--------------------------------+----------+---------+--------------------+-----------------+----------------+----------+" << LOG_END;
            for (ArrowStatus& as : get_arrow_status()) {
                LOG_INFO(logger) << "  | "
                                 << std::setw(4) << as.arrow_id << " | "
                                 << std::setw(30) << std::left << as.arrow_name << " | "
                                 << std::setw(8) << std::right << as.is_parallel << " | "
                                 << std::setw(7) << as.thread_count << " |"
                                 << std::setw(19) << as.messages_completed << " |"
                                 << std::setw(16) << as.short_term_avg_latency << " |"
                                 << std::setw(15) << as.long_term_avg_latency << " |"
                                 << std::setw(9) << as.is_finished << " |" << LOG_END;
            }
            LOG_INFO(logger) << "  +------+--------------------------------+----------+---------+--------------------+-----------------+----------------+----------+" << LOG_END;
        }


        QueueBase* addQueue(QueueBase* queue) {
            queue->set_id(queues.size());
            queues.push_back(queue);
            return queue;
        }


        void addArrow(std::string name, Arrow* arrow) {
            // Note that arrow name lives on the Topology, not on the Arrow
            // itself: different instances of the same Arrow can
            // be assigned different places in the same Topology, but
            // the users are inevitably going to make the Arrow
            // name be constant w.r.t to the Arrow class unless we force
            // them to do it correctly.

            // TODO: Commenting out these lines is going to cause problems
            //arrow->set_name(name);
            //arrow->set_id(arrows.size());

            arrows[name] = arrow;
            _arrow_statuses.emplace_back();
            ArrowStatus& status = _arrow_statuses.back();

            status.arrow_name = arrow->get_name();
            status.arrow_id = arrow->get_index();
            status.is_parallel = arrow->is_parallel();
            status.is_finished = arrow->is_finished();
            status.short_term_avg_latency = 0;
            status.long_term_avg_latency = 0;
            status.thread_count = 0;
            status.messages_completed = 0;
        };

        void addManagedComponent(Component * component) {
            components.push_back(component);
        }

        /// The user may want to pause the topology and interact with it manually.
        /// This is particularly powerful when used from inside GDB.
        StreamStatus step(const std::string& arrow_name) {
            Arrow* arrow = arrows[arrow_name];
            if (arrow == nullptr) {
                return StreamStatus::Error;
            }
            StreamStatus result = arrow->execute();
            if (result == StreamStatus::Finished && _arrow_statuses[arrow->get_index()].thread_count == 0) {
                report_arrow_finished(arrow);
            }
            return result;
        }
    };
}



