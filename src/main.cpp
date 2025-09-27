#include <iostream>
#include <stdexcept>
#include "order_book.hpp"
#include "matching_engine.hpp"

// Simple pretty-printers for quick sanity checks
static void dump_side(const char* name,
                      const std::map<Price, LevelQueue, std::less<Price>>& asks) {
  std::cout << name << " (low→high):\n";
  for (const auto& [px, q] : asks) {
    std::cout << "  " << px << " : [";
    for (size_t i = 0; i < q.size(); ++i) {
      std::cout << q[i].id << ":" << q[i].qty << (i + 1 < q.size() ? ", " : "");
    }
    std::cout << "]\n";
  }
}

static void dump_side(const char* name,
                      const std::map<Price, LevelQueue, std::greater<Price>>& bids) {
  std::cout << name << " (high→low):\n";
  for (const auto& [px, q] : bids) {
    std::cout << "  " << px << " : [";
    for (size_t i = 0; i < q.size(); ++i) {
      std::cout << q[i].id << ":" << q[i].qty << (i + 1 < q.size() ? ", " : "");
    }
    std::cout << "]\n";
  }
}

static void dump_book(const OrderBook& ob) {
  std::cout << "================ BOOK ================\n";
  dump_side("ASKS", ob.asks);
  dump_side("BIDS", ob.bids);
  std::cout << "best_bid=" << ob.best_bid()
            << " best_ask=" << ob.best_ask()
            << " mid="      << ob.mid() << "\n";
  std::cout << "======================================\n";
}

static void dump_fills(const std::vector<Fill>& fills) {
  for (const auto& f : fills) {
    std::cout << "TRADE taker=" << f.taker_id
              << " maker=" << f.maker_id
              << " side="  << (f.taker_side == Side::Buy ? "B" : "S")
              << " px="    << f.price
              << " qty="   << f.qty
              << " t="     << f.ts << "\n";
  }
  if (fills.empty()) std::cout << "(no trades)\n";
}

int main() {
  OrderBook ob;

  auto make_order = [](OrderId id, Side side, Price px, Qty qty, TimePoint ts) {
    Order o;
    o.id = id;
    o.side = side;
    o.type = OrdType::Limit;
    o.limit_price = px;
    o.qty = qty;
    o.ts = ts;
    return o;
  };

  // Add some bids/asks
  ob.add_limit(make_order(101, Side::Buy, 100, 5, 0.10));
  ob.add_limit(make_order(102, Side::Buy, 100, 3, 0.20));
  ob.add_limit(make_order(103, Side::Buy,  99, 7, 0.30));
  ob.add_limit(make_order(201, Side::Sell, 102, 4, 0.15));
  ob.add_limit(make_order(202, Side::Sell, 103, 6, 0.25));
  ob.add_limit(make_order(203, Side::Sell, 102, 2, 0.35));

  if (!ob.self_check()) { std::cerr << "self_check failed after adds!\n"; return 1; } // <---

  std::cout << "After adds:\n";
  dump_book(ob);

  // Cancels
  ob.cancel(102);
  ob.cancel(201);

  if (!ob.self_check()) { std::cerr << "self_check failed after cancels!\n"; return 1; } // <---

  std::cout << "\nAfter cancels (102, 201):\n";
  dump_book(ob);

  // Cancel non-existent
  ob.cancel(999);

  if (!ob.self_check()) { std::cerr << "self_check failed after cancel(999)!\n"; return 1; } // <---

  std::cout << "\nAfter cancel(999) (no-op):\n";
  dump_book(ob);

  // Duplicate ID (should throw)
try {
  ob.add_limit(make_order(101, Side::Buy, 100, 1, 0.5)); // duplicate id
  std::cerr << "Expected duplicate ID exception!\n";
} catch (const std::invalid_argument&) { /* ok */ }

// Cancel non-existent (no-op)
ob.cancel(424242); 
if (!ob.self_check()) return 1;

// Cancel the last remaining order at a level (level should disappear)
ob.add_limit(make_order(300, Side::Sell, 105, 2, 1.0));
ob.cancel(300);
if (ob.asks.count(105) != 0) { std::cerr << "level not erased\n"; return 1; }

std::cout << "\n===== M2: Matching Engine Demo =====\n";
MatchingEngine me(ob);

  // Seed book with some resting orders
  OrderId id1 = 1;
  ob.add_limit({id1++, Side::Sell, OrdType::Limit, 101, 5, 0.1});
  ob.add_limit({id1++, Side::Sell, OrdType::Limit, 102, 3, 0.2});
  ob.add_limit({id1++, Side::Buy,  OrdType::Limit,  99, 4, 0.3});
  ob.add_limit({id1++, Side::Buy,  OrdType::Limit, 100, 6, 0.4});

  std::cout << "Initial book:\n";
  dump_book(ob);

  // Crossing BUY limit @ 102 for 8 units
  std::vector<Fill> fills1;
  me.submit_limit(Side::Buy, 102, 8, 1.0, fills1);

  std::cout << "\nAfter BUY limit @102 x8:\n";
  dump_fills(fills1);
  dump_book(ob);

  // Market SELL for 7 units
  std::vector<Fill> fills2;
  me.submit_market(Side::Sell, 7, 2.0, fills2);

  std::cout << "\nAfter MARKET SELL x7:\n";
  dump_fills(fills2);
  dump_book(ob);

  return 0;
}