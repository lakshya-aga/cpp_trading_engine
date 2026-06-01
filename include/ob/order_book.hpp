#pragma once
#include <tuple>
#include <vector>
#include <unordered_map>
#include <map>
#include <list>
#include "decoder.hpp"

struct Order{ // Not using packed because mis aligned pointers create potential hazards flagged by compiler which I do not know
    OrderId id;
    Side side;
    Price price;
    Quantity qty;
};

struct PriceLevel{
    Price price;
    std::list<Order> orders;
    Quantity qty;
};
struct IdxEntry { Side side; Price price; };

class OrderBook{
    public:
        std::unordered_map<OrderId, IdxEntry> orders;
        bool add(const Order& order);
        bool cancel(OrderId oid);
        bool lookup(OrderId oid);
        int64_t peek_bid();
        int64_t peek_ask();
        std::map<Price, PriceLevel> bids;
        std::map<Price, PriceLevel> asks;

};