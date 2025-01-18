# Cache Simulator

A configurable N-way set-associative cache simulator featuring configurable replacement policies, write-back mechanism, and optional victim caches for both L1 and L2 caches. Designed for performance analysis and experimentation with benchmarks like SPEC 2000.

## Features

- **Configurable Replacement Policies**:
  - LRU (Least Recently Used)
  - PseudoLRU
  - Random
  - FIFO (First In, First Out)
  - Round Robin
- **Write-back Mechanism**
- **Optional Victim Caches** for L1 and L2

## How to Run

### Compilation
To compile the code, use the following command:
```bash
gcc -o cache_sim.exe cache_sim.cpp
```

### Execution
After compiling, run the simulator using the following command:
```bash
./cache_sim.exe <l1_size> <l1_assoc> <l1_block_size> <vc_num_blocks> <l2_size> <l2_assoc> <trace_file> <policy>
```

### Command-line Arguments
- `<l1_size>`: Size of the L1 cache in bytes.
- `<l1_assoc>`: Associativity of the L1 cache (e.g., 1 for direct-mapped, 2 for 2-way).
- `<l1_block_size>`: Block size for the L1 cache in bytes.
- `<vc_num_blocks>`: Number of blocks in the victim cache (0 for no victim cache).
- `<l2_size>`: Size of the L2 cache in bytes (0 for no L2 cache).
- `<l2_assoc>`: Associativity of the L2 cache.
- `<trace_file>`: Path to the trace file to use for simulation.
- `<policy>`: Replacement policy to use (options: `LRU`, `PseudoLRU`, `Random`, `FIFO`, `RoundRobin`).

### Example
To run the simulator with an L1 cache of 32 KB, 4-way associativity, 64-byte blocks, a victim cache with 8 blocks, an L2 cache of 256 KB, 8-way associativity, and using the `LRU` replacement policy:
```bash
./cache_sim.exe 32768 4 64 8 262144 8 trace.txt LRU
```
