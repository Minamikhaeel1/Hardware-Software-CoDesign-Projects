# Hardware/Software Co-Design Projects

Coursework projects developed for **Hardware/Software Co-design with AI Integration** using the AMD/Xilinx flow on the **PYNQ-Z2** board.

The repository focuses on two Zynq-7000 systems that combine programmable-logic accelerators, AXI interconnects and DMA-based data movement with software running on the ARM processing system.

## Projects

| Project | Main data path | Purpose |
|---|---|---|
| [Image Processing System](image-processing-system/) | Video Test Pattern Generator → AXI VDMA → DDR → ARM processor | Capture an RGB test image and process it into grayscale in software. |
| [Wireless Communication System](wireless-communication-system/) | AXI Traffic Generator → Convolutional Encoder → AXI DMA → software noise model → Viterbi Decoder → ILA | Demonstrate a simplified encoded communication chain and hardware/software partitioning. |

## Platform and tools

- PYNQ-Z2 (`xc7z020clg400-1`)
- AMD/Xilinx Vivado 2022.2
- AMD/Xilinx Vitis 2022.2
- Zynq-7000 Processing System
- AXI4, AXI4-Lite and AXI4-Stream
- AXI DMA / AXI VDMA
- Integrated Logic Analyzer (ILA)

## Implementation results

Both archived Vivado implementations target the PYNQ-Z2 and successfully generated bitstreams.

| Project | WNS | WHS | LUTs | Flip-flops | BRAM tiles | DSPs |
|---|---:|---:|---:|---:|---:|---:|
| Image processing | 2.083 ns | 0.019 ns | 5,043 | 7,044 | 4 | 0 |
| Wireless communication | 0.855 ns | 0.014 ns | 6,805 | 8,361 | 4.5 | 0 |

Positive WNS and WHS indicate that the implemented clocked paths met their timing requirements in these runs.

## Repository scope and validation status

The repository contains the block-design sources, software and project documentation. Large generated Vivado directories are intentionally excluded.

- Hardware implementation and bitstream generation: complete for the archived designs.
- Vitis reference applications are included for both projects.
- The image-processing application matches the archived three-frame-store VDMA configuration.
- The communication design uses the ILA to observe the Viterbi decoder output.
- Some communication IP may operate under an AMD/Xilinx evaluation license.

## Rebuilding

1. Install the PYNQ-Z2 board files.
2. Open Vivado 2022.2 and create a project for `xc7z020clg400-1`.
3. Add the appropriate `.bd` file from the project `hardware/` directory.
4. Validate the block design and regenerate output products.
5. Create an HDL wrapper, run synthesis and implementation, then generate the bitstream.

Exact results can vary with tool version, IP revisions and implementation directives.

## Author

**Mina Mikhaeel Fathy**

Communication and Electronics Engineering, Helwan University

Course instructor: **Mohamed Khaled**
