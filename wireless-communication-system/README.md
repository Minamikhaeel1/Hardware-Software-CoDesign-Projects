# Wireless Communication System

This project explores a hardware/software communication chain on the PYNQ-Z2 using a convolutional encoder, a software noise model and a Viterbi decoder.

![Vivado block design](docs/block-design.png)

## Intended data flow

1. An AXI Traffic Generator provides the source stream.
2. A convolutional encoder produces the coded data.
3. AXI DMA writes the coded stream to DDR.
4. ARM software applies an additive-noise model and prepares decoder input.
5. AXI DMA streams the data to the Viterbi decoder.
6. An ILA captures the Viterbi output for inspection.

## Implementation result

| Metric | Result |
|---|---:|
| Target | PYNQ-Z2 (`xc7z020clg400-1`) |
| Vivado version | 2022.2 |
| WNS | 0.855 ns |
| WHS | 0.014 ns |
| LUTs | 6,805 (12.79%) |
| Flip-flops | 8,361 (7.86%) |
| BRAM tiles | 4.5 |
| DSPs | 0 |
| Bitstream | Generated successfully |

## Corrected software and required hardware update

`software/main.c` replaces the unsafe buffer-content polling with AXI DMA status polling and a timeout, initializes and starts the streaming AXI Traffic Generator, performs the required cache maintenance, and applies Gaussian noise only to the two valid encoder output bits.

The archived bitstream cannot run that application correctly without two hardware changes:

1. Insert `rtl/axis_tlast_generator.sv` after the convolutional encoder so the DMA receives a packet-ending `TLAST`.
2. Configure the Viterbi Decoder for hard decisions and change the DMA MM2S stream width to 8 bits.

Follow [HARDWARE_CHANGES.md](HARDWARE_CHANGES.md) exactly, then regenerate the bitstream and XSA before rebuilding the Vitis platform.

The Viterbi output currently goes only to the ILA. Therefore, successful DMA completion proves data movement but not decoded-bit correctness. Capture the decoder output in the ILA, or add a second S2MM path for automatic comparison in software.

The archived implementation also contains evaluation-licensed communication IP; deployment restrictions imposed by the generated license must be observed.

## Contents

- `hardware/design_1.bd`: Vivado block-design source.
- `software/main.c`: corrected bare-metal Vitis application for the revised hard-decision design.
- `rtl/axis_tlast_generator.sv`: AXI4-Stream packet-boundary helper.
- `rtl/axis_tlast_generator_tb.sv`: self-checking testbench for the helper.
- `HARDWARE_CHANGES.md`: exact Vivado integration requirements.
- `docs/implementation-summary.md`: implementation and reproducibility notes.
