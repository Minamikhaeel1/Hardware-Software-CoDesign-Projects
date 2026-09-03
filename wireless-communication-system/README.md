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

## Validation items still required

The bitstream result proves that the design was implemented, but it does not by itself prove correct end-to-end packet processing. Before treating the project as fully verified:

- Provide an AXI4-Stream `TLAST` boundary to the DMA S2MM input after the convolutional encoder.
- Configure and start the AXI Traffic Generator from software, or replace it with a controlled stream source.
- Make the Viterbi hard/soft-decision setting match the software sample representation.
- Derive separate byte counts for the source, encoded data and decoder input.
- Poll DMA status with a timeout and perform the required cache maintenance.
- Capture valid Viterbi output in the ILA or return it to memory through another S2MM channel for automatic comparison.

The archived implementation also contains evaluation-licensed communication IP; deployment restrictions imposed by the generated license must be observed.

## Contents

- `hardware/design_1.bd`: Vivado block-design source.
- `docs/implementation-summary.md`: implementation and reproducibility notes.

