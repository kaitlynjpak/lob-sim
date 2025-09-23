#pragma once
#include <cstdint>
#include <string>

// Basic Order struct for LOB simulator
enum class Side { Buy, Sell };

struct Order {
    uint64_t id;
    Side side;
    int64_t price;  // in ticks
    int64_t qty;    // quantity
};
