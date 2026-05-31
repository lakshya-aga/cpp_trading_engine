
import random
import struct


random.Random(0)

def pack_add(seq, ts, id, side, price, qty) -> bytes:
    return struct.pack("<BQQQBqq", 0, seq, ts, id, side, price, qty)

def pack_cancel(seq, ts, id):
    return struct.pack("<BQQQ", 1, seq, ts, id)

def pack_modify(seq, ts, id, new_price, new_qty):
    return struct.pack("<BQQQqq", 2, seq, ts, id, new_price, new_qty)

def generate_feed(count, seed, mid, band, weights) -> list[bytes]:
    random.seed(seed)
    feed = []
    live = []
    next_id = 1
    for i in range(count):
        op = random.choices(
            population=["add", "cancel", "modify"],
            weights=weights,
            k=1
        )[0]
        ts = i * 1000
        if not live:
            op = "add"
        if op == "add":
            oid = next_id
            next_id += 1
            side_code = random.choice([0, 1])
            price = mid + random.randint(-band, band)
            qty = random.randint(1, 100)
            feed.append(pack_add(seq=i, ts=ts, id=oid, side=side_code, price=price, qty=qty))
            live.append(oid)           # <-- the missing line

        elif op == "cancel":
            j = random.randrange(len(live))           # pick a live id
            oid = live[j]
            live[j] = live[-1]                     # swap-with-last...
            live.pop()                             # ...and pop -> O(1) remove
            feed.append(pack_cancel(i, ts, oid))
        elif op == "modify":
            oid = random.choice(live)                 # modify keeps it live
            new_price = mid + random.randint(-band, band)
            new_qty = random.randint(1, 100)
            feed.append(pack_modify(i, ts, oid, new_price, new_qty))

    
    return feed

if __name__ == "__main__":
    feed = generate_feed(count=1000, seed=0, mid=10000, band=100, weights=[0.5, 0.2, 0.3])
    with open("feed.bin", "wb") as f:
        for entry in feed:
            f.write(entry)