#pragma once
#include <cstdint> 
#include <type_traits>
using OrderId = uint64_t;
using Price = int64_t;
using Quantity = int64_t;
using Timestamp = uint64_t;
using SeqNum = uint64_t;


enum class Side: uint8_t{
    Buy, Sell
};

enum class MsgType: uint8_t{
    Add, Cancel, Modify, Trade
};

struct __attribute__((packed)) Header
{
    MsgType type;
    SeqNum seqNum;
    Timestamp ts;
};
struct __attribute__((packed)) Add
{
    Header header;
    OrderId  id;
    Side side;
    Price price;
    Quantity qty;
};

struct __attribute__((packed)) Cancel
{
    Header header;
    OrderId  id;
};

struct __attribute__((packed)) Modify
{
    Header header;
    OrderId  id;
    Price new_price;
    Quantity new_quantity;
};

struct __attribute__((packed)) Trade
{
    Header header;
    OrderId taker; 
    OrderId maker; 
    Price price; 
    Quantity qty;
};

static_assert(std::is_trivially_copyable_v<Add>);
static_assert(sizeof(Add)==42);

static_assert(std::is_trivially_copyable_v<Cancel>);
static_assert(sizeof(Cancel)==25);

static_assert(std::is_trivially_copyable_v<Modify>);
static_assert(sizeof(Modify)==41);

static_assert(std::is_trivially_copyable_v<Trade>);
static_assert(sizeof(Trade)==49);

static_assert(std::is_trivially_copyable_v<Header>);
static_assert(sizeof(Header)==17);

// #include <iostream>
// int main(){
//     std::cout<<sizeof(Add)<<std::endl;
//     std::cout<<sizeof(Cancel)<<std::endl;
//     std::cout<<sizeof(Modify)<<std::endl;
//     std::cout<<sizeof(Trade)<<std::endl;
//     std::cout<<sizeof(Header)<<std::endl;
// }