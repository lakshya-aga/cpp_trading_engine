#include "matcher.hpp"
#include "message.hpp"

Matcher::Matcher(OrderBook& book) : ob(book) {}

std::vector<Trade> Matcher::apply(const Event& e) {
    switch(e.type){
        case MsgType::Add:    return match(e);
        case MsgType::Cancel: return cancel(e);
        case MsgType::Modify: return modify(e);
        default:              return {};
    }
}

Trade make_trade(OrderId maker, OrderId taker, Price price, Quantity qty){
    Trade t;
    t.header.type=MsgType::Trade;
    t.qty=qty;
    t.maker=maker;
    t.taker=taker;
    t.price=price;
    return t;
}
std::vector<Trade> Matcher::match(Event order){
    Quantity remaining = order.qty;
    std::vector<Trade> trades;
    switch(order.side){
        case Side::Buy: 
        while(remaining > 0 && !ob.asks.empty()){
            auto other_side = ob.asks.begin();
            if(other_side->first > order.price){
                break;   
            }
            while(remaining > 0 && !other_side->second.orders.empty()){
                auto& existing_order = other_side->second.orders.front();
                Quantity traded = std::min(remaining, existing_order.qty);
                trades.push_back(make_trade(
                        existing_order.id, 
                        order.oid, 
                        existing_order.price, 
                        traded));
                remaining-=traded;
                existing_order.qty-=traded;
                if(existing_order.qty==0){
                    ob.orders.erase(existing_order.id);
                    other_side->second.orders.pop_front();
                }
                
            }
            if(other_side->second.orders.empty()) ob.asks.erase(other_side);
                
        }
        break;
        // return trades;
        
        case Side::Sell: 
        while(remaining > 0 && !ob.bids.empty()){
            auto other_side = std::prev(ob.bids.end());
            if(other_side->first < order.price){
                break;   
            }
            while(remaining>0 && !other_side->second.orders.empty()){
                auto& existing_order = other_side->second.orders.front();
                Quantity traded = std::min(remaining, existing_order.qty);
                trades.push_back(make_trade(
                        existing_order.id, 
                        order.oid, 
                        existing_order.price, 
                        traded));
                remaining-=traded;
                existing_order.qty-=traded;
                if(existing_order.qty==0){
                    ob.orders.erase(existing_order.id);
                    other_side->second.orders.pop_front();
                }
                
            }
            if(other_side->second.orders.empty()) ob.bids.erase(other_side);
        }
        break;
        // return trades;

    }
    if(remaining>0){
        ob.add(Order{order.oid, order.side, order.price, remaining});
    }
    return trades;
}
std::vector<Trade> Matcher::cancel(Event order){
    ob.cancel(order.oid);
    return {};                 
}
std::vector<Trade> Matcher::modify(Event order){
    ob.cancel(order.oid);      // remove the old resting order so it loses time priority
    return match(order);       // re-submit through the matching path; may trade or rest
}