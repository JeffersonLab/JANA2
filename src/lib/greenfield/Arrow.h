
#pragma once
#include <vector>
#include <map>
#include "Queue.h"

namespace greenfield {

    template <typename T> struct Topology;

    struct Arrow {

        int chunksize = 1;
        bool is_finished = false;

        virtual bool is_parallel();
        virtual void execute();
    };

    template <typename T>
    struct SourceArrow : public Arrow {

        Queue<T>* output_queue;
        bool is_parallel() {return false;}

        virtual void init() = 0;
        virtual void finish() = 0;
        virtual T inprocess() = 0;
    };

    template <typename T>
    struct SinkArrow : public Arrow {

        Queue<T>* input_queue;
        bool is_parallel() {return false;}

        virtual void init() = 0;
        virtual void finish() = 0;
        virtual void outprocess(T t) = 0;
    };

    template <typename S, typename T>
    struct MapArrow : public Arrow {

        Queue<S>* input_queue;
        Queue<T>* output_queue;
        bool is_parallel() {return true;}

        virtual T transform(S s) = 0;
    };

    template <typename S, typename T>
    struct ReduceArrow : public Arrow {

        Queue<S>* input_queue;
        Queue<T>* output_queue;
        bool is_parallel() {return false;}

        virtual void init() = 0;
        virtual std::vector<T> finish() = 0;
        virtual std::vector<T> reduce(std::vector<S> ss) = 0;
    };

    template <typename S, typename T>
    struct GatherScatterArrow : public Arrow {

        std::vector<Queue<S>*> input_queues;
        std::vector<Queue<T>*> output_queue;
        bool is_parallel() {return true;}

        virtual std::multimap<int, T> gatherscatter(std::multimap<int, S> ss) = 0;
    };

    template<typename T>
    struct BroadcastArrow : public Arrow {

        Queue<T>* input_queue;
        std::vector<Queue<T>*> output_queues;
        bool is_parallel() {return false;}

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
        bool is_parallel() {return true;}

        virtual T transform(S s) = 0;
    };

    template <typename S, typename T>
    struct MergeArrow : public Arrow {

        Queue<SplitWrapper<S>>* input_queue;
        Queue<T>* output_queue;
        bool is_parallel() {return false;}

        virtual S merge(std::vector<T>) = 0;
    };


}
