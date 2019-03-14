
#pragma once
#include <vector>
#include <map>
#include <atomic>
#include "Queue.h"

namespace greenfield {

    class Arrow {

    protected:
        const bool _is_parallel = false;
        std::atomic<size_t> _chunksize {1};
        std::atomic<bool> _is_finished {false};

    public:
        bool is_finished() { return _is_finished; }
        bool is_parallel() { return _is_parallel; }
        void set_chunksize(int chunksize) { _chunksize = chunksize; }
        size_t get_chunksize() { return _chunksize; }

        explicit Arrow(bool is_parallel) : _is_parallel(is_parallel) {};
        virtual SchedulerHint execute() = 0;
    };


    template <typename T>
    class SourceArrow : public Arrow {

    private:
        Queue<T>* _output_queue;
        bool _is_initialized = false;

        virtual void initialize() = 0;
        virtual void finalize() = 0;
        virtual SchedulerHint inprocess(std::vector<T>& ts, size_t count) = 0;

    public:
        explicit SourceArrow(Queue<T>* output_queue) : Arrow(false), _output_queue(output_queue) {};

        SchedulerHint execute() final {
            if (_is_finished) {
                return SchedulerHint::Finished;
            }
            if (!_is_initialized) {
                initialize();
                _is_initialized = true;
            }
            std::vector<T> items(_chunksize);
            SchedulerHint in_status = inprocess(items, _chunksize);
            SchedulerHint out_status = _output_queue->push(std::move(items));

            if (in_status == SchedulerHint::Finished) {
                finalize();
                _is_finished = true; // prevent race conditions on _is_finished
                return SchedulerHint::Finished;
            }
            if (in_status == SchedulerHint::KeepGoing && out_status == SchedulerHint::KeepGoing) {
                return SchedulerHint::KeepGoing;
            }
            return SchedulerHint::ComeBackLater;
        }
    };

    template <typename T>
    class SinkArrow : public Arrow {

    private:
        Queue<T>* _input_queue;
        bool _is_initialized = false;

        virtual void initialize() = 0;
        virtual void finalize() = 0;
        virtual void outprocess(T t) = 0;

    public:
        explicit SinkArrow(Queue<T>* input_queue) :
            Arrow(false), _input_queue(input_queue) {};

        SchedulerHint execute() final {
            if (_is_finished) {
                return SchedulerHint::Finished;
            }
            if (!_is_initialized) {
                initialize();
                _is_initialized = true;
            }
            std::vector<T> items(_chunksize);
            SchedulerHint result = _input_queue->pop(items, _chunksize);
            for (T item : items) {
                outprocess(item);
            }
            if (result == SchedulerHint::Finished) {
                _is_finished = true;
                finalize();
            }
            return result;
        }
    };

    template <typename S, typename T>
    class MapArrow : public Arrow {

    private:
        Queue<S>* _input_queue;
        Queue<T>* _output_queue;

        virtual T transform(S s) = 0;

    public:
        MapArrow(Queue<S>* input_queue, Queue<T>* output_queue) :
            Arrow(true), _input_queue(input_queue), _output_queue(output_queue) {};

        SchedulerHint execute() final {
            std::vector<S> xs(_chunksize);
            std::vector<T> ys(_chunksize);
            SchedulerHint in_status = _input_queue->pop(xs, _chunksize);

            for (S& x : xs) {
                ys.push_back(transform(x));
            }
            SchedulerHint out_status = _output_queue->push(ys);
            if (in_status == SchedulerHint::Finished) {
                return SchedulerHint::Finished;
            }
            else if (in_status == SchedulerHint::KeepGoing && out_status == SchedulerHint::KeepGoing) {
                return SchedulerHint::KeepGoing;
            }
            else {
                return SchedulerHint::ComeBackLater;
            }
        }
    };

    template<typename T>
    class BroadcastArrow : public Arrow {

    private:
        Queue<T>* _input_queue;
        std::vector<Queue<T>*> _output_queues;

    public:
        BroadcastArrow(Queue<T>* input_queue, std::vector<Queue<T>*> output_queues) :
            Arrow(true), _input_queue(input_queue), _output_queues(output_queues) {};

        SchedulerHint execute() final {
            std::vector<T> items(_chunksize);
            SchedulerHint in_status = _input_queue->pop(items, _chunksize);
            bool output_status_keepgoing = true;

            for (Queue<T>* queue : _output_queues) {
                SchedulerHint out_status = queue->push(items);
                output_status_keepgoing &= (out_status == SchedulerHint::KeepGoing);
            }
            if (in_status == SchedulerHint::Finished) {
                return SchedulerHint::Finished;
            }
            else if (in_status == SchedulerHint::KeepGoing && output_status_keepgoing){
                return SchedulerHint::KeepGoing;
            }
            else {
                return SchedulerHint::ComeBackLater;
            }
        }
    };

    template <typename S, typename T>
    class ScatterArrow : public Arrow {

    private:
        Queue<S>* _input_queue;
        std::vector<Queue<T>*> _output_queues;

        virtual std::pair<size_t, T> scatter(S s) = 0;

    public:
        ScatterArrow(Queue<T>* input_queue, std::vector<Queue<T>*> output_queues) :
                Arrow(true), _input_queue(input_queue), _output_queues(output_queues) {};

        SchedulerHint execute() final {
            std::vector<S> items(_chunksize);
            SchedulerHint in_status = _input_queue->pop(items, _chunksize);
            for (S& item : items) {
                auto result = scatter(item);
                Queue<T>* output_queue = _output_queues[result.first];
                SchedulerHint out_status = output_queue->push({item}); // TODO: Add push(T t)
            }
            return in_status;
        }
    };


#if 0
    template <typename S, typename T>
    struct ReduceArrow : public Arrow {

        Queue<S>* input_queue;
        Queue<T>* output_queue;

        virtual void init() = 0;
        virtual std::vector<T> finish() = 0;
        virtual std::vector<T> reduce(std::vector<S> ss) = 0;
    };

    // SplitWrapper is one interesting option if we choose to make queues typed.
    // The main downside is that if we have multiple splits, we end up with
    // SplitWrapper<SplitWrapper<...>>

    template <typename T>
    struct SplitWrapper {
        T parent;
        int parent_id;
        int sibling_id;
        int sibling_count;
    };

    // Split has to be defined in terms of concrete types, or in terms of some SplitWrapper<T> which Merge undoes
    template <typename S, typename T>
    struct SplitArrow : public Arrow {

        Queue<S>* input_queue;
        Queue<SplitWrapper<T>>* output_queue;

        virtual T transform(S s) = 0;
    };

    template <typename S, typename T>
    struct MergeArrow : public Arrow {

        Queue<SplitWrapper<S>>* input_queue;
        Queue<T>* output_queue;

        virtual S merge(std::vector<T>) = 0;
    };

#endif
}
