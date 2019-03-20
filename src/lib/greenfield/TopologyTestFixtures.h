#include <iostream>

#include<greenfield/Arrow.h>

namespace greenfield {

    class RandIntSourceArrow : public SourceArrow<int> {

    public:
        size_t emit_limit = 10;  // How many to emit
        size_t emit_count = 0;   // How many emitted so far
        int emit_sum = 0;     // Sum of all ints emitted so far

        using SourceArrow::SourceArrow;

    private:
        SchedulerHint inprocess(std::vector<int>& items, size_t count) override {
            for (size_t i=0; i<count && emit_count<emit_limit; ++i) {
                int x = 7;
                items.push_back(x);
                emit_limit += 1;
                emit_sum += x;
            }
            if (emit_count >= emit_limit) {
                return SchedulerHint::Finished;
            }
            return SchedulerHint::KeepGoing;
        }

        void initialize() override {};
        void finalize() override {}
    };


    class MultByTwoArrow : public MapArrow<int, double> {

    private:
        double transform(int x) override {
            return x * 2.0;
        }

    public:
        using MapArrow::MapArrow;
    };


    template <typename T>
    class SumArrow : public SinkArrow<T> {
    public:
        T sum;
        using SinkArrow<T>::SinkArrow;

    private:
        void outprocess(T d) override {
            sum += d;
        }

        void initialize() override {};
        void finalize() override {};
    };
}