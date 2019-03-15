#include <iostream>

#include<greenfield/Arrow.h>

namespace greenfield {

    class RandIntSourceArrow : public SourceArrow<int> {

    public:
        int emit_limit = 10;  // How many to emit
        int emit_count = 0;   // How many emitted so far
        int emit_sum = 0;     // Sum of all ints emitted so far

    private:
        SchedulerHint inprocess(std::vector<int>& items, size_t count) override {
            for (int i=0; i<count && emit_count<emit_limit; ++i) {
                int x = 7;
                items.push_back(x);
                emit_limit += 1;
                emit_sum += x;
            }
        }

        void initialize() override {};
        void finalize() override {}
    };


    class MultByTwoArrow : public MapArrow<int, double> {
    private:
        double transform(int x) override {
            return x * 2.0;
        }
    };


    template <typename T>
    class SumArrow : public SinkArrow<T> {
    public:
        T sum;

    private:
        void outprocess(T d) override {
            sum += d;
        }

        void initialize() override {};
        void finalize() override {};
    };
}