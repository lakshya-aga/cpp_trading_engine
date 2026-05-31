#include "decoder.hpp"
#include "message.hpp"

Decoder::Decoder(std::span<const std::byte> buf){
    data_ = buf.data();
    size_ = buf.size();
}
void Decoder::reset() noexcept{
    pos_ = 0;
}

bool Decoder::next(Event& out){
    if (pos_ >= size_) return false;
    auto n = decode_one(data_+pos_, size_-pos_, out);
    if(n==0) return false;
    pos_ += n;
    return true;
}