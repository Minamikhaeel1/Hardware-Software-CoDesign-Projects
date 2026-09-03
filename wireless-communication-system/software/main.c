#include "platform.h"
#include "xparameters.h"
#include "xaxidma.h"
#include "xaxidma_hw.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xil_types.h"
#include "xstatus.h"
#include "xtrafgen.h"
#include "sleep.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Generated macro names can differ slightly between Vitis versions. */
#if defined(XPAR_AXIDMA_0_DEVICE_ID)
#define DMA_DEVICE_ID XPAR_AXIDMA_0_DEVICE_ID
#elif defined(XPAR_AXI_DMA_0_DEVICE_ID)
#define DMA_DEVICE_ID XPAR_AXI_DMA_0_DEVICE_ID
#else
#error "No AXI DMA device ID was found in xparameters.h"
#endif

#if defined(XPAR_AXI_TRAFFIC_GEN_0_DEVICE_ID)
#define TRAFFIC_GEN_DEVICE_ID XPAR_AXI_TRAFFIC_GEN_0_DEVICE_ID
#elif defined(XPAR_XTRAFGEN_0_DEVICE_ID)
#define TRAFFIC_GEN_DEVICE_ID XPAR_XTRAFGEN_0_DEVICE_ID
#else
#error "No AXI Traffic Generator device ID was found in xparameters.h"
#endif

/*
 * One source bit enters the rate-1/2 convolutional encoder per AXI beat.
 * Its two encoded bits are returned in bits [1:0] of one byte.
 */
#define SOURCE_SYMBOLS       128U
#define ENCODED_BYTES        SOURCE_SYMBOLS
#define DECODER_INPUT_BYTES  SOURCE_SYMBOLS
#define DMA_TIMEOUT_US       1000000U
#define NOISE_SIGMA          0.50

/*
 * This program targets the corrected Vivado design described in
 * HARDWARE_CHANGES.md: an 8-bit hard-decision Viterbi input and a TLAST
 * generator after the convolutional encoder.
 */
static u8 EncodedBuffer[ENCODED_BYTES] __attribute__((aligned(64)));
static u8 DecoderInput[DECODER_INPUT_BYTES] __attribute__((aligned(64)));

static XAxiDma AxiDma;
static XTrafGen TrafficGenerator;

static void PrintDmaStatus(void)
{
    const u32 TxStatus = XAxiDma_ReadReg(
        AxiDma.RegBase,
        XAXIDMA_TX_OFFSET + XAXIDMA_SR_OFFSET
    );
    const u32 RxStatus = XAxiDma_ReadReg(
        AxiDma.RegBase,
        XAXIDMA_RX_OFFSET + XAXIDMA_SR_OFFSET
    );

    xil_printf("MM2S status = 0x%08x\r\n", (unsigned int)TxStatus);
    xil_printf("S2MM status = 0x%08x\r\n", (unsigned int)RxStatus);
}

static int InitializeDma(void)
{
    XAxiDma_Config *Config;
    int Status;

    Config = XAxiDma_LookupConfig(DMA_DEVICE_ID);
    if (Config == NULL) {
        xil_printf("ERROR: AXI DMA configuration was not found.\r\n");
        return XST_FAILURE;
    }

    Status = XAxiDma_CfgInitialize(&AxiDma, Config);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: AXI DMA initialization failed.\r\n");
        return XST_FAILURE;
    }

    if (XAxiDma_HasSg(&AxiDma) != 0) {
        xil_printf("ERROR: AXI DMA must be configured in simple mode.\r\n");
        return XST_FAILURE;
    }

    XAxiDma_IntrDisable(
        &AxiDma,
        XAXIDMA_IRQ_ALL_MASK,
        XAXIDMA_DMA_TO_DEVICE
    );
    XAxiDma_IntrDisable(
        &AxiDma,
        XAXIDMA_IRQ_ALL_MASK,
        XAXIDMA_DEVICE_TO_DMA
    );

    return XST_SUCCESS;
}

static int InitializeTrafficGenerator(void)
{
    XTrafGen_Config *Config;
    int Status;

    Config = XTrafGen_LookupConfig(TRAFFIC_GEN_DEVICE_ID);
    if (Config == NULL) {
        xil_printf("ERROR: AXI Traffic Generator configuration was not found.\r\n");
        return XST_FAILURE;
    }

    Status = XTrafGen_CfgInitialize(
        &TrafficGenerator,
        Config,
        Config->BaseAddress
    );
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: AXI Traffic Generator initialization failed.\r\n");
        return XST_FAILURE;
    }

    if (TrafficGenerator.OperatingMode != XTG_MODE_STREAMING) {
        xil_printf("ERROR: AXI Traffic Generator is not in streaming mode.\r\n");
        return XST_FAILURE;
    }

    XTrafGen_ResetStreamingRandomLen(&TrafficGenerator);
    XTrafGen_SetStreamingTransLen(&TrafficGenerator, SOURCE_SYMBOLS - 1U);
    XTrafGen_SetStreamingTransCnt(&TrafficGenerator, 1U);

    return XST_SUCCESS;
}

static int WaitForDma(int Direction)
{
    u32 Timeout = DMA_TIMEOUT_US;
    u32 StatusRegister;
    u32 StatusOffset;

    while (XAxiDma_Busy(&AxiDma, Direction) != 0) {
        if (Timeout == 0U) {
            xil_printf("ERROR: AXI DMA transfer timed out.\r\n");
            PrintDmaStatus();
            return XST_FAILURE;
        }

        Timeout--;
        usleep(1U);
    }

    StatusOffset = (Direction == XAXIDMA_DMA_TO_DEVICE)
        ? XAXIDMA_TX_OFFSET
        : XAXIDMA_RX_OFFSET;

    StatusRegister = XAxiDma_ReadReg(
        AxiDma.RegBase,
        StatusOffset + XAXIDMA_SR_OFFSET
    );

    if ((StatusRegister & XAXIDMA_ERR_ALL_MASK) != 0U) {
        xil_printf("ERROR: AXI DMA reported a transfer error.\r\n");
        PrintDmaStatus();
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

/* Exact zero-mean Gaussian sample using the Marsaglia polar method. */
static double GenerateGaussianNoise(double Sigma)
{
    static int SpareValid = 0;
    static double Spare = 0.0;
    double U;
    double V;
    double RadiusSquared;
    double Scale;

    if (SpareValid != 0) {
        SpareValid = 0;
        return Sigma * Spare;
    }

    do {
        U = (2.0 * (double)rand() / (double)RAND_MAX) - 1.0;
        V = (2.0 * (double)rand() / (double)RAND_MAX) - 1.0;
        RadiusSquared = (U * U) + (V * V);
    } while ((RadiusSquared >= 1.0) || (RadiusSquared == 0.0));

    Scale = sqrt((-2.0 * log(RadiusSquared)) / RadiusSquared);
    Spare = V * Scale;
    SpareValid = 1;

    return Sigma * U * Scale;
}

static void ApplyAwgnAndMakeHardDecisions(void)
{
    u32 Symbol;
    u32 Bit;
    u32 ChangedBits = 0U;

    for (Symbol = 0U; Symbol < SOURCE_SYMBOLS; Symbol++) {
        const u8 EncodedPair = EncodedBuffer[Symbol] & 0x03U;
        u8 ReceivedPair = 0U;

        for (Bit = 0U; Bit < 2U; Bit++) {
            const u8 TransmittedBit = (EncodedPair >> Bit) & 0x01U;
            const double Bpsk = (TransmittedBit != 0U) ? 1.0 : -1.0;
            const double Received = Bpsk + GenerateGaussianNoise(NOISE_SIGMA);
            const u8 HardDecision = (Received >= 0.0) ? 1U : 0U;

            ReceivedPair |= (u8)(HardDecision << Bit);
            if (HardDecision != TransmittedBit) {
                ChangedBits++;
            }
        }

        DecoderInput[Symbol] = ReceivedPair;
    }

    xil_printf("AWGN channel produced %u hard-decision bit changes.\r\n",
               (unsigned int)ChangedBits);
}

static int CaptureEncodedData(void)
{
    int Status;

    memset(EncodedBuffer, 0, sizeof(EncodedBuffer));
    Xil_DCacheFlushRange((UINTPTR)EncodedBuffer, sizeof(EncodedBuffer));

    /* Arm S2MM before starting the stream source. */
    Status = XAxiDma_SimpleTransfer(
        &AxiDma,
        (UINTPTR)EncodedBuffer,
        ENCODED_BYTES,
        XAXIDMA_DEVICE_TO_DMA
    );
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: Could not start the S2MM transfer.\r\n");
        PrintDmaStatus();
        return XST_FAILURE;
    }

    XTrafGen_StreamEnable(&TrafficGenerator);

    Status = WaitForDma(XAXIDMA_DEVICE_TO_DMA);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    Xil_DCacheInvalidateRange((UINTPTR)EncodedBuffer, sizeof(EncodedBuffer));
    xil_printf("Captured %d encoded symbols in DDR.\r\n", (int)SOURCE_SYMBOLS);
    return XST_SUCCESS;
}

static int SendDataToViterbi(void)
{
    int Status;

    Xil_DCacheFlushRange((UINTPTR)DecoderInput, sizeof(DecoderInput));

    Status = XAxiDma_SimpleTransfer(
        &AxiDma,
        (UINTPTR)DecoderInput,
        DECODER_INPUT_BYTES,
        XAXIDMA_DMA_TO_DEVICE
    );
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: Could not start the MM2S transfer.\r\n");
        PrintDmaStatus();
        return XST_FAILURE;
    }

    return WaitForDma(XAXIDMA_DMA_TO_DEVICE);
}

int main(void)
{
    int Status;
    int Result = XST_FAILURE;

    init_platform();
    srand(1U); /* A repeatable noise sequence is useful during debugging. */

    xil_printf("\r\n========================================\r\n");
    xil_printf(" PYNQ-Z2 Encoded Communication Test\r\n");
    xil_printf("========================================\r\n");

    Status = InitializeDma();
    if (Status != XST_SUCCESS) {
        goto Done;
    }

    Status = InitializeTrafficGenerator();
    if (Status != XST_SUCCESS) {
        goto Done;
    }

    Status = CaptureEncodedData();
    if (Status != XST_SUCCESS) {
        goto Done;
    }

    ApplyAwgnAndMakeHardDecisions();

    Status = SendDataToViterbi();
    if (Status != XST_SUCCESS) {
        goto Done;
    }

    xil_printf("Decoder input transfer completed.\r\n");
    xil_printf("Capture the Viterbi output with the connected ILA.\r\n");
    xil_printf("COMMUNICATION DATA-PATH TEST COMPLETED\r\n");
    Result = XST_SUCCESS;

Done:
    cleanup_platform();
    return Result;
}
