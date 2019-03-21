#pragma once

#include <greenfield/Arrow.h>
#include <greenfield/Queue.h>
#include <greenfield/JLogger.h>


namespace greenfield {

    struct Topology {


        struct QueueStatus {
            uint32_t queue_id;
            uint64_t height;
            uint64_t threshold;
            bool is_finished;
        };


        std::vector<QueueStatus> get_queue_status() {
            std::vector<QueueStatus> statuses;
            int i=0;
            for (QueueBase* q : queues) {
                QueueStatus qs;
                qs.queue_id = i++;
                qs.is_finished = q->is_finished();
                qs.height = q->get_item_count();
                qs.threshold = q->get_threshold();
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
                                 << std::setw(8) << qs.height << " | "
                                 << std::setw(9) << qs.threshold << " | "
                                 << std::setw(8) << qs.is_finished << " |" << LOG_END;
            }
            LOG_INFO(logger) << "  +------+----------+-----------+----------+" << LOG_END;
        }

        // TODO: Consider using shared_ptr instead of raw pointer
        // TODO: Figure out access control

        // #####################################################
        // These are meant to be used by ThreadManager
        std::map<std::string, Arrow*> arrows;
        std::vector<QueueBase*> queues;

        // #####################################################
        // These are meant to be used by the user

        std::shared_ptr<JLogger> logger;

        QueueBase* addQueue(QueueBase* queue) {
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

            arrows[name] = arrow;
            arrow->set_name(name);
        };

        /// The user may want to pause the topology and interact with it manually.
        /// This is particularly powerful when used from inside GDB.
        SchedulerHint step(const std::string& arrow_name) {
            Arrow* arrow = arrows[arrow_name];
            if (arrow == nullptr) {
                return SchedulerHint::Error;
            }
            return arrow->execute();
        }


    };
}



