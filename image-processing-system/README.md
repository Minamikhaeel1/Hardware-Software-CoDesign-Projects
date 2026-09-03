# Image Processing System

This project implements a basic hardware/software image-processing pipeline on the PYNQ-Z2.

![Vivado block design](docs/block-design.png)

## Architecture

1. The Video Test Pattern Generator produces a 64 × 64 RGB color-bar frame.
2. AXI VDMA receives the AXI4-Stream pixels and writes the frame to DDR through the Zynq high-performance port.
3. Software running on the ARM processor reads the frame and calculates an 8-bit grayscale value for every pixel.
4. An ILA monitors the video stream during hardware debugging.

## Hardware interfaces

- TPG output: AXI4-Stream video, 24-bit RGB pixels.
- VDMA direction used: S2MM (stream to memory).
- Processor control: AXI4-Lite through the PS general-purpose master port.
- Frame memory: DDR through a PS high-performance slave port.

## Implementation result

| Metric | Result |
|---|---:|
| Target | PYNQ-Z2 (`xc7z020clg400-1`) |
| Vivado version | 2022.2 |
| WNS | 2.083 ns |
| WHS | 0.019 ns |
| LUTs | 5,043 (9.48%) |
| Flip-flops | 7,044 (6.62%) |
| BRAM tiles | 4 |
| DSPs | 0 |
| Bitstream | Generated successfully |

## Important software notes

Before using the software as a reusable reference:

- Configure one VDMA frame store or provide a valid address for every configured frame store.
- Zero-initialize `XAxiVdma_DmaSetup` before assigning its fields.
- Check the return value of `XAxiVdma_DmaSetBufferAddr`.
- Call `XV_tpg_Start()` after configuring and enabling auto-restart.
- Store the computed grayscale values in a real output buffer.
- Use a frame counter, interrupt or timeout instead of an unlimited busy loop.
- Flush or invalidate the data cache at the DMA ownership boundaries.

## Contents

- `hardware/c2g.bd`: Vivado block-design source.
- `docs/implementation-summary.md`: implementation and reproducibility notes.

