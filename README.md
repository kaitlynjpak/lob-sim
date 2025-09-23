# LOB Simulator (C++)

Event-driven limit order book simulator with:
- Matching engine (price-time priority)
- Strategies (market maker, momentum, pairs)
- Metrics (PnL, inventory, Sharpe, drawdown)

## Build
```bash
mkdir -p build && cd build
cmake ..
cmake --build .
./lob_sim
