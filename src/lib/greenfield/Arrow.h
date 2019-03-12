
#pragma once
#include <vector>
#include <map>

namespace greenfield {

    template <typename T> struct Topology;

    template <typename T>
    struct Arrow {
        int chunksize = 1;
        bool is_finished = false;
        const bool is_parallel = true;
        virtual void execute(Topology<T>& t);
    };

    template <typename T>
    struct SeqArrow : public Arrow<T> {
        const bool is_parallel = false;
        virtual void init() = 0;
        virtual void finish() = 0;
    };

    template <typename T>
    struct SourceArrow : public SeqArrow<T> {
        virtual int get_output_queue_id() = 0;
        virtual T inprocess() = 0;
    };

    template <typename T>
    struct SinkArrow : public SeqArrow<T> {
        virtual int get_input_queue_id() = 0;
        virtual void outprocess(T t) = 0;
    };

    template <typename T>
    struct MapArrow : public Arrow<T> {
        virtual int get_input_queue_id() = 0;
        virtual int get_output_queue_id() = 0;
        virtual T transform(T t) = 0;
    };

    template <typename T>
    struct ReduceArrow : public SeqArrow<T> {
        virtual int get_input_queue_id() = 0;
        virtual int get_output_queue_id() = 0;
        virtual std::vector<T> reduce(std::vector<T> ts) = 0;
    };

    template <typename T>
    struct GatherScatterArrow : public Arrow<T> {
        std::vector<int> get_input_queue_ids() = 0;
        std::vector<int> get_output_queue_ids() = 0;
        virtual std::multimap<int, T> gatherscatter(std::multimap<int, T> ts) = 0;
    };

    template <typename T>
    struct SplitArrow : public Arrow<T> {
        virtual int

    };

    template<typename T>
    struct BroadcastArrow : public Arrow<T> {


    };






}
