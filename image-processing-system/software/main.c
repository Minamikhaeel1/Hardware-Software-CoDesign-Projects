#include "platform.h"
#include "xparameters.h"
#include "xaxivdma.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xil_types.h"
#include "xstatus.h"
#include "xv_tpg.h"
#include "sleep.h"

#include <string.h>

/* Generated macro names can differ slightly between Vitis versions. */
#if defined(XPAR_AXIVDMA_0_DEVICE_ID)
#define VDMA_DEVICE_ID XPAR_AXIVDMA_0_DEVICE_ID
#elif defined(XPAR_AXI_VDMA_0_DEVICE_ID)
#define VDMA_DEVICE_ID XPAR_AXI_VDMA_0_DEVICE_ID
#else
#error "No AXI VDMA device ID was found in xparameters.h"
#endif

#if defined(XPAR_V_TPG_0_DEVICE_ID)
#define TPG_DEVICE_ID XPAR_V_TPG_0_DEVICE_ID
#else
#error "No Video Test Pattern Generator device ID was found in xparameters.h"
#endif

#define FRAME_WIDTH          64U
#define FRAME_HEIGHT         64U
#define BYTES_PER_PIXEL      3U
#define FRAME_BYTES          (FRAME_WIDTH * FRAME_HEIGHT * BYTES_PER_PIXEL)
#define PIXEL_COUNT          (FRAME_WIDTH * FRAME_HEIGHT)

/* The supplied Vivado design configures the VDMA with three frame stores. */
#define VDMA_FRAME_STORES    3U
#define CAPTURE_TIMEOUT_US   1000000U

/*
 * Global aligned arrays are placed in DDR by the normal standalone linker
 * script. Each frame starts on a 64-byte boundary, which is safe when the
 * VDMA Data Realignment Engine is disabled.
 */
static u8 FrameBuffers[VDMA_FRAME_STORES][FRAME_BYTES]
    __attribute__((aligned(64)));
static u8 GrayFrame[PIXEL_COUNT] __attribute__((aligned(64)));

static XAxiVdma Vdma;
static XV_tpg Tpg;

static int InitializeVdma(void)
{
    XAxiVdma_Config *Config;
    XAxiVdma_DmaSetup Setup;
    XAxiVdma_FrameCounter FrameCounter;
    u32 Index;
    int Status;

    Config = XAxiVdma_LookupConfig(VDMA_DEVICE_ID);
    if (Config == NULL) {
        xil_printf("ERROR: AXI VDMA configuration was not found.\r\n");
        return XST_FAILURE;
    }

    if ((Config->HasS2Mm == 0) ||
        (Config->MaxFrameStoreNum < VDMA_FRAME_STORES)) {
        xil_printf("ERROR: The VDMA hardware configuration does not match "
                   "this application.\r\n");
        return XST_FAILURE;
    }

    Status = XAxiVdma_CfgInitialize(&Vdma, Config, Config->BaseAddress);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: AXI VDMA initialization failed.\r\n");
        return XST_FAILURE;
    }

    memset(&Setup, 0, sizeof(Setup));
    Setup.VertSizeInput = (int)FRAME_HEIGHT;
    Setup.HoriSizeInput = (int)(FRAME_WIDTH * BYTES_PER_PIXEL);
    Setup.Stride = (int)(FRAME_WIDTH * BYTES_PER_PIXEL);
    Setup.FrameDelay = 0;
    Setup.EnableCircularBuf = 0;
    Setup.EnableSync = 0;
    Setup.PointNum = 0;
    Setup.EnableFrameCounter = 1;
    Setup.FixedFrameStoreAddr = 0;

    for (Index = 0U; Index < VDMA_FRAME_STORES; Index++) {
        Setup.FrameStoreStartAddr[Index] = (UINTPTR)&FrameBuffers[Index][0];
    }

    Status = XAxiVdma_DmaConfig(&Vdma, XAXIVDMA_WRITE, &Setup);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: AXI VDMA write-channel configuration failed.\r\n");
        return XST_FAILURE;
    }

    Status = XAxiVdma_DmaSetBufferAddr(
        &Vdma,
        XAXIVDMA_WRITE,
        Setup.FrameStoreStartAddr
    );
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: AXI VDMA frame-buffer configuration failed.\r\n");
        return XST_FAILURE;
    }

    /* Both counts must be non-zero in the 2022.2 VDMA driver. */
    memset(&FrameCounter, 0, sizeof(FrameCounter));
    FrameCounter.ReadFrameCount = 1;
    FrameCounter.ReadDelayTimerCount = 1;
    FrameCounter.WriteFrameCount = 1;
    FrameCounter.WriteDelayTimerCount = 1;

    Status = XAxiVdma_SetFrameCounter(&Vdma, &FrameCounter);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: AXI VDMA frame counter could not be set.\r\n");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

static int InitializeTpg(void)
{
    int Status;

    Status = XV_tpg_Initialize(&Tpg, TPG_DEVICE_ID);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: Video TPG initialization failed.\r\n");
        return XST_FAILURE;
    }

    XV_tpg_Set_height(&Tpg, FRAME_HEIGHT);
    XV_tpg_Set_width(&Tpg, FRAME_WIDTH);
    XV_tpg_Set_colorFormat(&Tpg, 0U); /* RGB in this generated TPG design. */
    XV_tpg_Set_bckgndId(&Tpg, XTPG_BKGND_COLOR_BARS);
    XV_tpg_EnableAutoRestart(&Tpg);

    return XST_SUCCESS;
}

static int CaptureOneFrame(void)
{
    u32 Timeout = CAPTURE_TIMEOUT_US;
    u32 Errors;
    int Status;

    memset(FrameBuffers, 0, sizeof(FrameBuffers));

    /* Prevent dirty CPU cache lines from later overwriting DMA output. */
    Xil_DCacheFlushRange((UINTPTR)FrameBuffers, sizeof(FrameBuffers));

    /* Start the receiver before allowing the TPG to produce video data. */
    Status = XAxiVdma_DmaStart(&Vdma, XAXIVDMA_WRITE);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: AXI VDMA write channel did not start.\r\n");
        return XST_FAILURE;
    }

    XV_tpg_Start(&Tpg);

    while (XAxiVdma_IsBusy(&Vdma, XAXIVDMA_WRITE) != 0) {
        if (Timeout == 0U) {
            xil_printf("ERROR: Frame capture timed out.\r\n");
            XAxiVdma_DmaStop(&Vdma, XAXIVDMA_WRITE);
            return XST_FAILURE;
        }

        Timeout--;
        usleep(1U);
    }

    Errors = (u32)XAxiVdma_GetDmaChannelErrors(&Vdma, XAXIVDMA_WRITE);
    if (Errors != 0U) {
        xil_printf("ERROR: AXI VDMA reported 0x%08x.\r\n",
                   (unsigned int)Errors);
        return XST_FAILURE;
    }

    Xil_DCacheInvalidateRange(
        (UINTPTR)&FrameBuffers[0][0],
        FRAME_BYTES
    );

    return XST_SUCCESS;
}

static void ConvertToGrayscale(void)
{
    u32 Pixel;
    u32 Checksum = 0U;
    u8 Minimum = 255U;
    u8 Maximum = 0U;

    for (Pixel = 0U; Pixel < PIXEL_COUNT; Pixel++) {
        const u32 Offset = Pixel * BYTES_PER_PIXEL;
        const u32 Red = FrameBuffers[0][Offset + 0U];
        const u32 Green = FrameBuffers[0][Offset + 1U];
        const u32 Blue = FrameBuffers[0][Offset + 2U];
        const u8 Gray = (u8)((77U * Red + 150U * Green + 29U * Blue) >> 8U);

        GrayFrame[Pixel] = Gray;
        Checksum += Gray;

        if (Gray < Minimum) {
            Minimum = Gray;
        }
        if (Gray > Maximum) {
            Maximum = Gray;
        }
    }

    xil_printf("Processed %d pixels into GrayFrame.\r\n", (int)PIXEL_COUNT);
    xil_printf("Grayscale range: %d to %d, checksum: %u\r\n",
               (int)Minimum,
               (int)Maximum,
               (unsigned int)Checksum);
}

int main(void)
{
    int Status;
    int Result = XST_FAILURE;

    init_platform();

    xil_printf("\r\n========================================\r\n");
    xil_printf(" PYNQ-Z2 RGB-to-Grayscale Demonstration\r\n");
    xil_printf("========================================\r\n");

    Status = InitializeVdma();
    if (Status != XST_SUCCESS) {
        goto Done;
    }

    Status = InitializeTpg();
    if (Status != XST_SUCCESS) {
        goto Done;
    }

    Status = CaptureOneFrame();
    if (Status != XST_SUCCESS) {
        goto Done;
    }

    xil_printf("One 64 x 64 RGB frame was captured in DDR.\r\n");
    ConvertToGrayscale();
    xil_printf("IMAGE PROCESSING TEST PASSED\r\n");
    Result = XST_SUCCESS;

Done:
    cleanup_platform();
    return Result;
}
