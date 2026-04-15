# Qualifiers

The language uses a system of annotqualifiers) to explicitly define the memory hierarchy and movement of data across different hardware components. This provides a complete mental model for data management, from CPU RAM to GPU SRAM.

## `@host`
- **Hardware:** CPU RAM
- **Typical Usage:** Dataset loading and heavy preprocessing tasks (Pandas-style). This is standard, pageable system memory.

## `@pinned`
- **Hardware:** CPU RAM (Page-locked / Pinned)
- **Typical Usage:** Transit buffers used to stream data to GPUs at high bandwidth (e.g., 32GB/s+). Since it cannot be paged out to disk, it allows for faster DMA transfers to the device.

## `@global`
- **Hardware:** GPU VRAM
- **Typical Usage:** Storage of model weights, biases, and gradients on a single GPU board. This is the main high-capacity memory on the device.

## `@sharded`
- **Hardware:** Cluster VRAM (Multi-GPU)
- **Typical Usage:** Distribution of giant models (e.g., Llama 405B) across 8+ GPUs. It abstracts the partitioned memory across a cluster of devices.

## `@shared`
- **Hardware:** GPU SRAM (Shared Memory)
- **Typical Usage:** Ultra high-speed computation inside a kernel (e.g., Tiling). This is the small, fast, on-chip memory shared by threads within the same block.
