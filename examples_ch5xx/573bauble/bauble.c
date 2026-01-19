#include "ch32fun.h"
#include "bauble.h"
#include <stdio.h>

#ifdef CH570_CH572
#define LED PA9
#else
#define LED PA8
#endif

#define SLEEPTIME_MS 1000

// The advertisement to be sent. The MAC address should be in the first 6 bytes in reversed byte order,
// after that any BLE flag can be used.
uint8_t adv[] = {0x66, 0x55, 0x44, 0x33, 0x22, 0x11, // MAC (reversed)
				 0x03, 0x19, 0x00, 0x00, // 0x19: "Appearance", 0x00, 0x00: "Unknown"
				 0x08, 0x09, 'c', 'h', '3', '2', 'f', 'u', 'n'}; // 0x09: "Complete Local Name"
// uint8_t adv_channels[] = {37,38,39};
uint8_t adv_channels[] = {38};

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

static void rx_loop() {

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

static void tx_loop() {

    printf("Starting TX....\n");

    while(1) {
		// BLE advertisements are sent on channels 37, 38 and 39, over the 1M PHY
		for(int c = 0; c < sizeof(adv_channels); c++) {
			printf("Advertising on channel %d\n", adv_channels[c]);
            DevSetChannel(adv_channels[c]);
			Frame_TX(adv, sizeof(adv));
		}

		// go to sleep between advertisements
		printf("Sleeping...\n");
        LL->LL25 = 0x1000;
		Delay_Ms(SLEEPTIME_MS);

		blink(1);
	}
}


int main() {
    SystemInit();
    DCDCEnable(); // Enable the internal DCDC
    funGpioInitAll();
    funPinMode( LED, GPIO_CFGLR_OUT_2Mhz_PP );

    printf(".~ CH573 bauBLE ~.\n");
    printf("Build time: %s\n", __TIME__);

    sweep_regs(RF, 54, "RF");

    uint8_t txPower = LL_TX_POWER_MINUS_10_DBM;
    RFCoreInit(txPower);
    SetAccessAddress(0xdeadbeef);
    // SetAccessAddress(0x00b1d00f);
    blink(5);

    DevSetChannel(39);
    DevSetFrequency(2473000);

    memset(LLE_BUF_TX, 0, 0x110);
    memset(LLE_BUF_RX, 0xAA, 0x110);

    // uint32_t rssi_reg = BB->RSSI_ST;
    // printf("RSSI %02x %03x %04x %d\n", (rssi_reg >> 24) & 0xFF, (rssi_reg >> 15) & 0x1FF, (rssi_reg >> 0) & 0x7FFF, (int16_t)(rssi_reg << 1)/2);

    tx_loop();
    // rx_loop();
}