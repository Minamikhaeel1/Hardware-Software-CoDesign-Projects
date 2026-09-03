# Required Vivado changes for the communication software

The archived block design implements successfully, but its original data path cannot complete an AXI DMA S2MM transfer. The convolutional encoder does not generate `TLAST`, while AXI DMA uses `TLAST` to recognize the end of an incoming packet. The original Viterbi Decoder is also configured for 3-bit soft decisions, whereas the corrected software produces 2-bit hard-decision symbol pairs.

Apply these changes before running `software/main.c`.

## 1. Insert the TLAST generator

1. Add `rtl/axis_tlast_generator.sv` to the Vivado project as a design source.
2. In the block design, choose **Add Module** and add `axis_tlast_generator`.
3. Set `DATA_WIDTH` to `8` and `PACKET_BEATS` to `128`.
4. Disconnect `convolution_0/M_AXIS_DATA` from `axi_dma_0/S_AXIS_S2MM`.
5. Connect `convolution_0/M_AXIS_DATA` to `axis_tlast_generator_0/S_AXIS`.
6. Connect `axis_tlast_generator_0/M_AXIS` to `axi_dma_0/S_AXIS_S2MM`.
7. Connect `aclk` to the same 100 MHz fabric clock as the encoder and DMA.
8. Connect `aresetn` to the same active-low peripheral reset.

The module passes `TDATA`, `TVALID` and `TREADY` without changing them, drives `TKEEP` high, and asserts `TLAST` on every 128th accepted output beat. If `SOURCE_SYMBOLS` changes in the C program, change `PACKET_BEATS` to the same value.

## 2. Match the Viterbi input to the software

1. Re-customize `viterbi_0`.
2. Select **Hard Coding** instead of 3-bit soft coding.
3. Keep constraint length `7`, rate `1/2`, and traceback length `42`.
4. Keep the decoder polynomials matched to the convolutional encoder. The IP GUIs may display the decoder polynomials in the reversed-bit convention; do not change the known matched pair only because the displayed decimal values look different.
5. Re-customize `axi_dma_0` and set the MM2S stream width to `8` bits so one byte carries one hard-decision pair in bits `[1:0]`.
6. Connect `axi_dma_0/M_AXIS_MM2S` to the Viterbi data input and keep the ILA on the Viterbi output.

## 3. Rebuild the platform

1. Validate the block design and regenerate output products.
2. Regenerate the HDL wrapper.
3. Run synthesis, implementation and bitstream generation.
4. Export a new XSA that includes the bitstream.
5. In Vitis, update or recreate the platform from the new XSA, then rebuild the BSP and application.

The software uses `log()` and `sqrt()` from the C math library. If Vitis reports an undefined reference to either function, open the application project's linker settings and add `m` to the libraries list (Vitis passes this to the linker as `-lm`).

The C program initializes the AXI Traffic Generator in streaming mode, requests one 128-beat transaction, starts S2MM before the source, waits with a timeout, adds Gaussian noise to the two valid encoded bits, and sends the hard decisions to the Viterbi Decoder.

## 4. What is and is not automatically verified

The program verifies DMA completion and reports DMA errors. Because the current architecture sends the Viterbi output only to an ILA, the software cannot compare decoded bits automatically. Use the ILA to capture `TDATA`, `TVALID` and `TREADY` at the decoder output. For a self-checking end-to-end test, add a second S2MM path from the Viterbi output to DDR and compare the recovered source bits in software.
