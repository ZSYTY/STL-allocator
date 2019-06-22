#include <ctime>
#include <iostream>
#include <random>
#include <vector>

#include "allocator.hpp"
#include "naive_allocator.hpp"

// include header of your allocator here
// template <class T>
// using MyAllocator =
// Mallocator<T>;  // replace the std::allocator with your allocator
using Point2D = std::pair<int, int>;

const int TestSize = 10000;
const int PickSize = 10000;

template <template <class> class MyAllocator>
class tester {
   public:
    void main() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, TestSize);

        // vector creation
        using IntVec = std::vector<int, MyAllocator<int>>;
        std::vector<IntVec, MyAllocator<IntVec>> vecints(TestSize);
        for (int i = 0; i < TestSize; i++) vecints[i].resize(dis(gen));

        using PointVec = std::vector<Point2D, MyAllocator<Point2D>>;
        std::vector<PointVec, MyAllocator<PointVec>> vecpts(TestSize);
        for (int i = 0; i < TestSize; i++) vecpts[i].resize(dis(gen));

        // vector resize
        for (int i = 0; i < PickSize; i++) {
            int idx = dis(gen) - 1;
            int size = dis(gen);
            vecints[idx].resize(size);
            vecpts[idx].resize(size);
        }

        // vector element assignment
        {
            int val = 10;
            int idx1 = dis(gen) - 1;
            int idx2 = vecints[idx1].size() / 2;
            vecints[idx1][idx2] = val;
            if (vecints[idx1][idx2] == val)
                std::cout << "correct assignment in vecints: " << idx1
                          << std::endl;
            else
                std::cout << "incorrect assignment in vecints: " << idx1
                          << std::endl;
        }
        {
            Point2D val(11, 15);
            int idx1 = dis(gen) - 1;
            int idx2 = vecpts[idx1].size() / 2;
            vecpts[idx1][idx2] = val;
            if (vecpts[idx1][idx2] == val)
                std::cout << "correct assignment in vecpts: " << idx1
                          << std::endl;
            else
                std::cout << "incorrect assignment in vecpts: " << idx1
                          << std::endl;
        }
    }
};

int main() {
    clock_t start;

    tester<Nallocator> tester1;
    start = clock();
    tester1.main();
    std::cout << "Naive allocator without memory pool cost: "
              << (clock() - start) * 1.0 / CLOCKS_PER_SEC << " seconds"
              << std::endl
              << std::endl;

    tester<Mallocator> tester2;
    start = clock();
    tester2.main();
    std::cout << "Allocator with memory pool cost: "
              << (clock() - start) * 1.0 / CLOCKS_PER_SEC << " seconds"
              << std::endl
              << std::endl;

    tester<std::allocator> tester3;
    start = clock();
    tester3.main();
    std::cout << "std::allocator cost: "
              << (clock() - start) * 1.0 / CLOCKS_PER_SEC << " seconds"
              << std::endl
              << std::endl;
    return 0;
}