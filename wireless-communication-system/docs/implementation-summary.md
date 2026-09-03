# Wireless Communication Implementation Summary

The archived Vivado 2022.2 run completed implementation and bitstream generation for the PYNQ-Z2 (`xc7z020clg400-1`).

## Timing

- Worst negative slack (WNS): **0.855 ns**
- Worst hold slack (WHS): **0.014 ns**
- Setup and hold timing were met for the implemented clocked paths.

## Utilization

- Slice LUTs: **6,805 / 53,200 (12.79%)**
- Slice registers: **8,361 / 106,400 (7.86%)**
- Block RAM tiles: **4.5**
- DSP slices: **0**

## Reproducibility notes

- The original project selected `tul.com.tw:pynq-z2:part0:1.0`, so compatible board files are required.
- The communication encoder/decoder IP was generated under an evaluation license. A regenerated licensed build may be required for unrestricted hardware operation.
- Review packet boundaries and Viterbi input packing before board-level validation.

