#pragma once

#include <iostream>
#include <chrono>

namespace structuredb::cli {

template <typename Callback>
void MeasureTime(Callback cb) {
    const auto start = std::chrono::high_resolution_clock::now();
    cb();
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cerr << "Execution time: " << duration.count() << " ms" << std::endl;
}

}
