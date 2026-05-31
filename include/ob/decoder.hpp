#pragma once
#include <cstddef>
#include <cstdint>
#include "message.hpp"
#include <span>
#include <cstring>

class Event{
    public: 
        MsgType type;
        SeqNum seq;
        Timestamp ts;
        OrderId oid;
        Side side;
        Price price;
        Quantity qty;
};

constexpr std::size_t wire_size(MsgType type) noexcept{
    switch(type) {
        case MsgType::Add: return 42;
        case MsgType::Cancel: return 25;
        case MsgType::Modify: return 41;
        case MsgType::Trade: return 49;
        default: return 0;
    }
}
inline std::size_t decode_one(const std::byte* p, std::size_t remaining,
                              Event& out) noexcept {
    if (remaining < 1) return 0;
    MsgType t = static_cast<MsgType>(p[0]);
    std::size_t n = wire_size(t);
    if (n == 0 || remaining < n) return 0;   

    switch (t) {
    case MsgType::Add: {
        Add a;
        std::memcpy(&a, p, sizeof(a));       
        out.type  = MsgType::Add;
        out.seq   = a.header.seqNum;         
        out.ts    = a.header.ts;
        out.oid   = a.id;
        out.side  = a.side;
        out.price = a.price;
        out.qty   = a.qty;
        break;
    }
    case MsgType::Cancel: {
        Cancel c;
        std::memcpy(&c, p, sizeof(c));
        out.type = MsgType::Cancel;
        out.seq  = c.header.seqNum;
        out.ts   = c.header.ts;
        out.oid  = c.id;
        break;
    }
    case MsgType::Modify: {
        Modify m;
        std::memcpy(&m, p, sizeof(m));
        out.type  = MsgType::Modify;
        out.seq   = m.header.seqNum;
        out.ts    = m.header.ts;
        out.oid   = m.id;
        out.price = m.new_price;
        out.qty   = m.new_quantity;
        break;
    }
    default:
        return 0;
    }
    return n;
}


static_assert(std::is_trivially_copyable_v<Event>);


class Decoder{
    private:
        const std::byte* data_;
        size_t size_;
        size_t pos_ = 0;
    public:
        explicit Decoder(std::span<const std::byte> buf);
        bool next(Event& out);
        void reset() noexcept;
        
};
