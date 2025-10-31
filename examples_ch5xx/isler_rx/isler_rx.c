#include "ch32fun.h"
#include "iSLER.h"
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


    printf("LLE_BUF1[0..3] (0x%08lx) = %02x %02x %02x %02x\n", LLE_BUF, LLE_BUF[0], LLE_BUF[1], LLE_BUF[2], LLE_BUF[3]);
    printf("LLE_BUF2[0..3] (0x%08lx) = %02x %02x %02x %02x\n", LLE_BUF2, LLE_BUF2[0], LLE_BUF2[1], LLE_BUF2[2], LLE_BUF2[3]);

    // The chip stores the incoming frame in LLE_BUF, defined in extralibs/iSLER.h
    uint8_t *frame = (uint8_t *)(0x20000000 + DMA->DMA6);

    // // The first two bytes of the frame are metadata with RSSI and length
    printf("RSSI: %d len: %d\n", frame[0], frame[1]);
    // printf("MAC: ");
    // for(int i = 7; i > 2; i--) {
    //     printf("%02x:", frame[i]);
    // }
    // printf("%02x data:", frame[2]);
    // for(int i = 8; i < frame[1] +2; i++) {
    //     printf("%02x ", frame[i]);
    // }
    // printf("\n");
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

#define PRINT(...) printf(__VA_ARGS__)

void dump_reg(uint32_t addr, uint8_t len, char *name)
{
    uint8_t i;
    PRINT("---- %s reg ----\n", name);
    for (i = 0; i < len; i++) {
        PRINT("%s->%s%d = 0x%08lx\n", name, name, i, ((volatile uint32_t *)addr)[i]);
    }
    PRINT("\n");
}

static void dump_all(void)
{
    dump_reg(DMA_BASE, 8, "DMA");
    dump_reg(LL_BASE, 31, "LL");
    dump_reg(BB_BASE, 31, "BB");
    // dump_reg(RF, 31, "RF");
}

int old_main() {
    SystemInit();
    DCDCEnable(); // Enable the internal DCDC
    funGpioInitAll();
    funPinMode( LED, GPIO_CFGLR_OUT_2Mhz_PP );

    printf(".~ ch32fun BLE RX ~.\n");

    uint8_t txPower = 0x14;
    RFCoreInit(txPower);
    SetAccessAddress(0xdeadbeef);
    blink(5);

    memset(LLE_BUF, 0, 0x110);
    memset(LLE_BUF2, 0, 0x110);

    // dump_regs((void *)LL, 32, "LL");

    // volatile uint32_t *ll_reg = (void *)LL;
    // for(int i = 0; i < 32; i++) {
    //     uint32_t val = ll_reg[i];
    //     uint32_t mask = 0xFFFFFFFF;
    //     // Locks up if we set LL3 bit 4
    //     // if (i == 3) mask = 0xFFFFFFEF;
    //     ll_reg[i] = mask;
    //     printf("MASK_LL%02d = %08lx\n", i, ll_reg[i] | ~mask);
    //     ll_reg[i] = val;
    // }

    // LL->INT_EN = 0xFFFF;

    printf("Starting RX....\n");
    Frame_RX(NULL, 38, 0);

    // *(volatile uint32_t *) 0xe000e100 = 0x100000;
    // *(volatile uint32_t *) 0xe000e100 = 0x200000;
    // dump_all();

    while(1) {
        // now listen for frames on channel 37. When the RF subsystem
        // detects and finalizes one, "rx_ready" in iSLER.h is set true
        printf("Listening...");
        while(!rx_ready) {
            Delay_Ms(10);
            printf(".");
        };
        printf("\nFrame received!\n");
        // we got a frame!
        incoming_frame_handler();
        blink(1);
        Delay_Ms(500);
        Frame_RX(NULL, 38, 0);
    }
}

int main() {
    SystemInit();

    printf("start.\n");
   	DevInit(0x14);
	RegInit();

	SetAccessAddress(0xdeadbeef);

	LL->INT_EN = 0x00000000; // disable interrupts
    Frame_RX(NULL, 38, 0);

    while(1)
    {
        uint32_t status = LL->STATUS & 0xFFFF;

        if (status & (1<<0)) {
            LL->STATUS &= ~(1<<0);
            printf("MANUAL RX IRQ 1 %08lx\n", status);
            printf("LLEBUF (0x%08lx) :", (void *)LLE_BUF2);
            for (int i = 0; i < 16; i++) {
                printf(" %x", ((uint8_t *)LLE_BUF2)[i]);
            }
            printf("\n");
            Frame_RX(NULL, 38, 0);
        }

        if (status) {
            LL->STATUS = 0;
        }
    }
}
