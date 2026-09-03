# Image Processing Implementation Summary

The archived Vivado 2022.2 run completed implementation and bitstream generation for the PYNQ-Z2 (`xc7z020clg400-1`).

## Timing

- Worst negative slack (WNS): **2.083 ns**
- Worst hold slack (WHS): **0.019 ns**
- Setup and hold timing were met for the implemented clocked paths.

## Utilization

- Slice LUTs: **5,043 / 53,200 (9.48%)**
- Slice registers: **7,044 / 106,400 (6.62%)**
- Block RAM tiles: **4**
- DSP slices: **0**

## Dependency

The original project selected the PYNQ-Z2 board part `tul.com.tw:pynq-z2:part0:1.0`. Install compatible PYNQ-Z2 board files before rebuilding, or target the device directly and recreate the board-level processing-system configuration.

