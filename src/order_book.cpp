// #pragma once
#include <tuple>
#include <vector>
#include <unordered_map>
#include <map>
#include <list>
#include "order_book.hpp"

bool OrderBook::add(const Order& order){
    orders[order.id] = {order.side, order.price};
    if(order.side==Side::Sell)
    {
        asks[order.price].orders.push_back(order);
        orders[order.id].it = std::prev(asks[order.price].orders.end());
        return true;
    }
    else if(order.side==Side::Buy){
        bids[order.price].orders.push_back(order);
        orders[order.id].it = std::prev(bids[order.price].orders.end());
        return true;
    }
    return false;
}

bool OrderBook::cancel(OrderId oid){
    auto idx = orders.find(oid);
    if (idx == orders.end()) return false;
    auto order = idx->second;
    auto& book = order.side == Side::Buy ? bids : asks; // & to reference is crucial, otherwise end up with copies. (sweet java memories)
    auto level = book.find(order.price);

    if(level!=book.end())
    level->second.orders.erase(orders[oid].it);

    if (level->second.orders.empty()) book.erase(level);
    orders.erase(idx);
    return true;
}

bool OrderBook::lookup(OrderId oid){
    return orders.contains(oid);
}

int64_t OrderBook::peek_bid(){
    if(bids.size()==0)
    return INT64_MIN;
    return (--bids.end())->first;
}

int64_t OrderBook::peek_ask(){
    if(asks.size()==0)
    return INT64_MAX;
    return (asks.begin())->first;

}

