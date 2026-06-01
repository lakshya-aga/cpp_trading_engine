#pragma once
#include "order_book.hpp"
#include "message.hpp"
#include <vector>
class Matcher{
    public:
        explicit Matcher(OrderBook& book);
        std::vector<Trade> apply(const Event& e);
        OrderBook& ob;
        std::vector<Trade> match(Event order);
        std::vector<Trade> cancel(Event order);
        std::vector<Trade> modify(Event order);
};

