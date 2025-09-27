#include "order_book.hpp"
#include <stdexcept>
#include <cstddef>
#include <iterator>

void OrderBook::add_limit(const Order& o) {
  if (o.type != OrdType::Limit) {
    throw std::invalid_argument("add_limit expects OrdType::Limit");
  }
  if (index.find(o.id) != index.end()) {
    throw std::invalid_argument("Duplicate OrderId");
  }

  // Choose the side explicitly; avoid mixing comparator types
  if (o.side == Side::Buy) {
    auto& q = bids[o.limit_price];   // creates level if missing
    q.push_back(o);
    const std::size_t pos = q.size() - 1;
    index.emplace(o.id, IndexEntry{ Side::Buy, o.limit_price, pos });
  } else {
    auto& q = asks[o.limit_price];
    q.push_back(o);
    const std::size_t pos = q.size() - 1;
    index.emplace(o.id, IndexEntry{ Side::Sell, o.limit_price, pos });
  }
}

void OrderBook::cancel(OrderId id) {
  auto it = index.find(id);
  if (it == index.end()) return; // not found

  const Side side = it->second.side;
  const Price px  = it->second.px;
  std::size_t pos = it->second.pos;

  // Get the right side’s map
  if (side == Side::Buy) {
    auto lvl_it = bids.find(px);
    if (lvl_it == bids.end()) { index.erase(it); return; }

    LevelQueue& q = lvl_it->second;
    if (pos >= q.size()) { index.erase(it); return; }

    auto erase_it = q.begin() + static_cast<std::ptrdiff_t>(pos);
    q.erase(erase_it);

    // Re-index subsequent orders at the same price level
    for (std::size_t p = pos; p < q.size(); ++p) {
      auto idx_it = index.find(q[p].id);
      if (idx_it != index.end()) idx_it->second.pos = p;
    }

    if (q.empty()) bids.erase(lvl_it);
  } else {
    auto lvl_it = asks.find(px);
    if (lvl_it == asks.end()) { index.erase(it); return; }

    LevelQueue& q = lvl_it->second;
    if (pos >= q.size()) { index.erase(it); return; }

    auto erase_it = q.begin() + static_cast<std::ptrdiff_t>(pos);
    q.erase(erase_it);

    for (std::size_t p = pos; p < q.size(); ++p) {
      auto idx_it = index.find(q[p].id);
      if (idx_it != index.end()) idx_it->second.pos = p;
    }

    if (q.empty()) asks.erase(lvl_it);
  }

  index.erase(it);
}