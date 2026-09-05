# Image Processing System

This project demonstrates a simple hardware/software image-processing pipeline on the PYNQ-Z2 board.

## Block design

![Vivado block design](images/block-design.png)

## What the project does

1. The Video Test Pattern Generator creates a 64 × 64 RGB color-bar image.
2. AXI VDMA transfers the image stream to DDR memory.
3. The ARM processor reads the RGB pixels from memory.
4. Software converts every RGB pixel into an 8-bit grayscale value.
5. The grayscale image is stored in memory, while the ILA monitors the hardware video stream.

## Main components

| Component | Purpose |
|---|---|
| Zynq-7000 Processing System | Runs the image-processing software and controls the system. |
| Video Test Pattern Generator | Produces the RGB color-bar image. |
| AXI VDMA | Transfers the video stream into DDR memory. |
| DDR memory | Stores the captured RGB frame and grayscale result. |
| ILA | Captures the video stream inside the FPGA for observation. |

## Project purpose

The project shows how FPGA hardware and processor software can share image-processing work. The hardware generates and transfers the image efficiently, while the ARM processor performs the grayscale conversion.
