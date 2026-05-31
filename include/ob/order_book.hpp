#pragma once
#include <vector>
#include <map>
#include <queue>
#include "decoder.hpp"

struct __attribute__((packed)) Order{
    OrderId id;
    Side side;
    Price price;
    Quantity qty;
};

struct PriceLevel{
    Price price;
    std::queue<Order> orders;
    Quantity qty;
};

class OrderBook{
    public:
        std::unordered_map<OrderId, Order> orders;
        bool add(Event u);
        bool cancel(OrderId oid);
        bool lookup(OrderId oid);
        int peek_bid();
        int peek_ask();
        std::map<Price, PriceLevel> bids;
        std::map<Price, PriceLevel> asks;

};