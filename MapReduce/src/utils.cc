// utils.cc
#include "utils.h"

#include <chrono>
#include <random>

uint32_t SetUid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis;
    return dis(gen);
}
