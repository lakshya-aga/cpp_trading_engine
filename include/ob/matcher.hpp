#pragma once
#include "order_book.hpp"
#include "message.hpp"
class Matcher{
    public:
        OrderBook ob;
        Trade match(Event order);
};

