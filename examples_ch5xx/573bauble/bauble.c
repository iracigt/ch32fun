#include "ch32fun.h"
#include "bauble.h"
#include <stdio.h>

#ifdef CH570_CH572
#define LED PA9
#else
#define LED PA8
#endif

static void blink(int n) {
    for(int i = n-1; i >= 0; i--) {
        funDigitalWrite( LED, FUN_LOW ); // Turn on LED
        Delay_Ms(250);
        funDigitalWrite( LED, FUN_HIGH ); // Turn off LED
        Delay_Ms(250);
    }
}

static void incoming_frame_handler(void) {


    // printf("LLE_BUF1[0..3] (0x%08lx) = %02x %02x %02x %02x\n", LLE_BUF, LLE_BUF[0], LLE_BUF[1], LLE_BUF[2], LLE_BUF[3]);
    // printf("LLE_BUF2[0..3] (0x%08lx) = %02x %02x %02x %02x\n", LLE_BUF2, LLE_BUF2[0], LLE_BUF2[1], LLE_BUF2[2], LLE_BUF2[3]);

    uint8_t *frame = LLE_BUF_RX;
    uint8_t rssi = frame[0];
    uint8_t len = frame[1];
    size_t extra_offset = (2+len+3)/4*4;
    int16_t cfo = (*(uint16_t *)(&frame[extra_offset]) + 0x400) % 0x800 - 0x400;


    // // The first two bytes of the frame are metadata with RSSI and length
    printf("RSSI: %d len: %d cfo : %d\n", rssi, len, cfo);
    printf("MAC: ");
    for(int i = 7; i > 2; i--) {
        printf("%02x:", frame[i]);
    }
    printf("%02x data:", frame[2]);
    for(int i = 8; i < len + 2; i++) {
        printf("%02x ", frame[i]);
    }

    printf("\npost: ");
    
    for(int i = len + 2; i < len + 2 + 32; i++) {
        printf("%02x ", frame[i]);
    }

    printf("\nraw: ");

        for(int i = 0; i < extra_offset+4; i++) {
        printf("%02x ", frame[i]);
    }

    printf("\n");
}

static void sweep_regs(void *reg, int count, const char *name) {
    volatile uint32_t *base = reg;
    for(int i = 0; i < count; i++) {
        uint32_t val = base[i];
        base[i] = 0xFFFFFFFF;
        printf("MASK_%s%02d = %08lx\n", name, i, base[i]);
        base[i] = val;
    }
}

static void dump_reg(uint32_t addr, uint8_t len, char *name)
{
    uint8_t i;
    printf("---- %s reg ----\n", name);
    for (i = 0; i < len; i++) {
        printf("%s->%s%d = 0x%08lx\n", name, name, i, ((volatile uint32_t *)addr)[i]);
    }
    printf("\n");
}

static void dump_all(void)
{
    dump_reg(DMA_BASE, 8, "DMA");
    dump_reg(LL_BASE, 31, "LL");
    dump_reg(BB_BASE, 31, "BB");
    // dump_reg(RF, 31, "RF");
}

int main() {
    SystemInit();
    DCDCEnable(); // Enable the internal DCDC
    funGpioInitAll();
    funPinMode( LED, GPIO_CFGLR_OUT_2Mhz_PP );

    printf(".~ ch32fun BLE RX ~.\n");

    uint8_t txPower = 0x14;
    RFCoreInit(txPower);
    SetAccessAddress(0xdeadbeef);
    // SetAccessAddress(0x00b1d00f);
    blink(5);

    DevSetChannel(39);
    DevSetFrequency(2473000);

    memset(LLE_BUF_TX, 0, 0x110);
    memset(LLE_BUF_RX, 0xAA, 0x110);


    uint32_t rssi_reg = BB->RSSI_ST;
    printf("RSSI %02x %03x %04x %d\n", (rssi_reg >> 24) & 0xFF, (rssi_reg >> 15) & 0x1FF, (rssi_reg >> 0) & 0x7FFF, (int16_t)(rssi_reg << 1)/2);

    printf("Starting RX....\n");
    Frame_RX(NULL);


    while(1) {
        // now listen for frames on channel 37. When the RF subsystem
        // detects and finalizes one, "rx_ready" in iSLER.h is set true
        printf("Listening...\n");
        while(!rx_ready) {
            Frame_RX(NULL);
            uint32_t rssi_reg = BB->RSSI_ST;
            // if (rssi_reg & 0xFF000000) 
            {
                printf("RSSI %02x %d %02x %04x %+5d\n", (rssi_reg >> 24) & 0xFF, (rssi_reg >> 23)&1, (rssi_reg >> 15) & 0xFF, (rssi_reg >> 0) & 0x7FFF, (int16_t)(rssi_reg << 1)/2);
            }
            Delay_Ms(5);

        };
        printf("\nFrame received! RSSI = 0x%02x LEN = 0x%02x DMACFG = 0x%04x\n", LLE_BUF_RX[0], LLE_BUF_RX[1], DMA->DMA0_CTRL_CFG & 0xFFFF);
        // we got a frame!
        incoming_frame_handler();
        blink(1);
        // Delay_Ms(500);
        Frame_RX(NULL);
        memset(LLE_BUF_RX, 0xAA, 0x110);
    }
}