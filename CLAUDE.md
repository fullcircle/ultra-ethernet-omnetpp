# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an OMNeT++ simulation of Ultra Ethernet technology for AI/HPC workloads. The project simulates large-scale ethernet clusters (1K-10K nodes) with specialized features like In-Network Computing (INC), packet spraying, and AI-optimized transport protocols.

## Build System

### Core Commands
- `make` - Build the simulation executable (`ultraethernet_sim`)
- `make clean` - Clean build artifacts
- `make install` - Copy executable and config to results directory

### Running Simulations
- `make run` - Run basic 1K node simulation
- `make benchmark` - Run performance comparison tests
- `make sweep` - Run parameter sensitivity analysis
- `make run-parallel` - Run 10K node simulation with MPI parallelization

### Configuration
- Main config file: `omnetpp.ini`
- Key configurations:
  - `UltraEthernet_1K`: 1024-node cluster simulation
  - `UltraEthernet_10K`: 10,000-node cluster with partitioning
  - `Performance_Comparison`: Baseline comparison with RoCE/InfiniBand
  - `Parameter_Sweep`: Automated parameter sensitivity analysis

## Architecture

### Protocol Stack Layers (in UltraEthernetHost)
1. **Application Layer** (`AIHPCApplication`): Generates AI/HPC workloads (AllReduce, AllGather, etc.)
2. **Transport Layer** (`UETTransport`): Handles RDMA operations, congestion control, packet spraying
3. **Network Layer** (`UltraEthernetIP`): IP-level routing and addressing
4. **Link Layer** (`UltraEthernetLink`): Link-level reliability (LLR) and error correction
5. **Physical Layer** (`UltraEthernetPhy`): 800G/1600G links with FEC

### Key Components
- **UltraEthernetSwitch**: Network switches with INC processing capability
- **INCProcessor**: In-Network Computing for collective operations (AllReduce, Broadcast)
- **UltraEthernetCluster**: Network topology generator (supports Dragonfly topology)

### Message Types
- **UETPacket**: Base transport packet with multi-layer headers
- **INCPacket**: Extended packet for collective operations
- **LLRAck**: Link-layer acknowledgment packets

## AI/HPC Features

### Transport Profiles
- `AI_BASE`: Basic AI workload support
- `AI_FULL`: Advanced features (deferrable sends, enhanced congestion control)
- `HPC`: Optimized for HPC simulation workloads

### Workload Types
- `AI_TRAINING`: Deep learning training patterns
- `AI_INFERENCE`: Inference workloads
- `HPC_SIMULATION`: Scientific computing patterns
- `DISTRIBUTED_DATABASE`: Database workloads

### Communication Patterns
- AllReduce, AllGather, Broadcast operations
- Point-to-point and parameter server patterns
- Atomic operations (fetch-and-add, compare-and-swap)

## Performance Analysis

Use `performance_analysis.py` to process simulation results:
- Parses `.sca` (scalar) and `.vec` (vector) result files
- Generates performance comparison reports
- Analyzes throughput, latency, and error metrics

## Development Notes

### Dependencies
- Requires OMNeT++ framework with OMNETPP_ROOT set
- INET framework for network simulation components
- MPI support for parallel simulation (10K+ nodes)

### Key Parameters
- `linkSpeed`: 800Gbps default, configurable per link
- `fecEnabled`: Forward Error Correction for reliability
- `incProcessingEnabled`: Enable In-Network Computing
- `packetSprayingEnabled`: Multi-path packet distribution
- `profileType`: AI_BASE/AI_FULL/HPC transport profiles

### Simulation Scaling
- 1K nodes: Single-threaded simulation
- 10K nodes: Requires MPI parallelization with partitioning
- Use `parallelSimulation=true` and `numPartitions` for large scales