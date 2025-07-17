# Ultra Ethernet OMNeT++ Simulation

An OMNeT++ simulation framework for Ultra Ethernet networking technology optimized for AI/HPC workloads, featuring In-Network Computing (INC) capabilities and large-scale cluster support.

## Overview

This project simulates Ultra Ethernet protocol stacks designed for high-performance AI and HPC applications. It models complete networking clusters from 1,000 to 10,000+ nodes with advanced features like packet spraying, in-network computing, and AI-optimized transport protocols.

## Features

### Core Protocol Stack
- **Physical Layer**: 800G/1600G links with Forward Error Correction (FEC)
- **Link Layer**: Link-Level Reliability (LLR) with retransmission
- **Network Layer**: IP routing with Ultra Ethernet extensions
- **Transport Layer**: RDMA operations with congestion control and multipath
- **Application Layer**: AI/HPC workload generators and communication patterns

### Advanced Capabilities
- **In-Network Computing (INC)**: Hardware-accelerated collective operations
- **Packet Spraying**: Multi-path packet distribution for load balancing
- **AI Transport Profiles**: Optimized for training and inference workloads
- **Parallel Simulation**: MPI-based scaling for large clusters
- **Performance Analysis**: Comprehensive metrics and baseline comparisons

## Quick Start

### Prerequisites
- OMNeT++ 6.0+ with INET framework
- GCC with C++17 support
- MPI (for parallel simulations)

### Build
```bash
# Set environment variables
export OMNETPP_ROOT=/path/to/omnetpp
export INET_ROOT=/path/to/inet

# Build simulation
make

# Run basic simulation
make run
```

### Configuration

The simulation includes several pre-configured scenarios:

- **1K Node Cluster**: `make run` (basic configuration)
- **Performance Comparison**: `make benchmark` (includes baseline comparisons)
- **Parameter Sweep**: `make sweep` (sensitivity analysis)
- **10K Node Cluster**: `make run-parallel` (requires MPI)

## Simulation Configurations

### UltraEthernet_1K
- 1,024 nodes with Dragonfly topology
- 800 Gbps links with FEC enabled
- AI training workload with AllReduce patterns
- Single-threaded simulation

### UltraEthernet_10K
- 10,000 nodes with partitioned simulation
- MPI-based parallel execution
- Optimized for large-scale performance studies

### Performance_Comparison
- Statistical comparison with RoCE and InfiniBand baselines
- Multiple simulation runs for confidence intervals
- Detailed performance measurement

## Architecture

### Network Topology
```
Application Layer (AIHPCApplication)
    ↓
Transport Layer (UETTransport)
    ↓
Network Layer (UltraEthernetIP)
    ↓
Link Layer (UltraEthernetLink)
    ↓
Physical Layer (UltraEthernetPhy)
```

### Key Components

- **UltraEthernetHost**: Complete host implementation with full protocol stack
- **UltraEthernetSwitch**: Network switch with INC processing capabilities
- **INCProcessor**: In-network computing engine for collective operations
- **AIHPCApplication**: Workload generator for AI/HPC communication patterns

## Workload Types

### AI Workloads
- **AI_TRAINING**: Deep learning training with AllReduce operations
- **AI_INFERENCE**: Inference workloads with broadcast patterns
- **Parameter Server**: Distributed parameter server communication

### HPC Workloads
- **HPC_SIMULATION**: Scientific computing communication patterns
- **Point-to-Point**: Direct node-to-node communication
- **Collective Operations**: Optimized group communication

## Performance Analysis

Use the included Python analysis tool:

```bash
python3 performance_analysis.py results/
```

The analyzer processes simulation results and generates:
- Latency and throughput comparisons
- Error rate analysis
- Scaling performance metrics
- Baseline technology comparisons

## Configuration Parameters

### Key Parameters
- `linkSpeed`: Link bandwidth (default: 800Gbps)
- `profileType`: AI_BASE, AI_FULL, or HPC transport profile
- `incProcessingEnabled`: Enable in-network computing
- `packetSprayingEnabled`: Enable multipath packet distribution
- `workloadType`: AI_TRAINING, AI_INFERENCE, or HPC_SIMULATION

### Simulation Scale
- `numNodes`: Number of compute nodes
- `numPartitions`: MPI partitions for parallel simulation
- `parallelSimulation`: Enable MPI-based scaling

## Results and Metrics

The simulation collects comprehensive performance metrics:

- **Latency**: End-to-end message delivery times
- **Throughput**: Aggregate network bandwidth utilization
- **Scalability**: Performance across different cluster sizes
- **Reliability**: Error rates and correction statistics
- **Efficiency**: Resource utilization and overhead analysis

## Development

### File Structure
- `*.h/*.cc`: C++ implementation files
- `*.ned`: Network description files
- `*.msg`: Message definition files
- `omnetpp.ini`: Simulation configuration
- `Makefile`: Build configuration

### Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test with existing configurations
5. Submit a pull request

## License

This project is available under standard academic and research use terms.

## References

- OMNeT++ Simulation Framework: https://omnetpp.org/
- INET Framework: https://inet.omnetpp.org/
- Ultra Ethernet Consortium: https://ultraethernet.org/