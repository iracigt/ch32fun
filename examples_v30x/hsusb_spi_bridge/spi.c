#include "ch32v003fun.h"
#include "buf.h"

static uint8_t tx_data[] = { 0xF0, 0x70, 0x30, 0x10 };
static buffer_t tx_buf = {
    .buf = &tx_data,
    .len = sizeof(tx_data),
};

void spi_init(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_SPI1;

	SPI1->CTLR1 = 0;
	SPI1->CTLR1 |= SPI_CTLR1_BR & (0<<3);
    SPI1->CTLR1 |= (SPI_CPOL_Low | SPI_CPHA_1Edge);
	SPI1->CTLR1 |= SPI_Mode_Slave | SPI_NSS_Hard;
    SPI1->CTLR1 |= SPI_DataSize_8b | SPI_Direction_2Lines_FullDuplex;

    SPI1->HSCR = 1;

    RCC->APB2PCENR |= RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA;

    // CS on PA4, input, floating
    GPIOA->CFGLR &= ~(0xf<<(4*4));
    GPIOA->CFGLR |= GPIO_CNF_IN_FLOATING<<(4*4);

    // SCK on PA5, input, floating
    GPIOA->CFGLR &= ~(0xf<<(4*5));
    GPIOA->CFGLR |= GPIO_CNF_IN_FLOATING<<(4*5);

    // MOSI on PA7, input, floating
    GPIOA->CFGLR &= ~(0xf<<(4*7));
    GPIOA->CFGLR |= GPIO_CNF_IN_FLOATING<<(4*7);
    
    // MISO on PA6, Output, alt func, push-pull
    GPIOA->CFGLR &= ~(0xf<<(4*6));
    GPIOA->CFGLR |= (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*6);


    // Route PA4 to EXTI 4
    AFIO->EXTICR[1] &= ~AFIO_EXTICR2_EXTI4;
    AFIO->EXTICR[1] |= AFIO_EXTICR2_EXTI4_PA;

    // Enable EXTI4 on rising edge only
    EXTI->RTENR |= EXTI_RTENR_TR4;
    EXTI->FTENR &= ~EXTI_FTENR_TR4;

    NVIC_EnableIRQ(EXTI4_IRQn);


    // Configure DMA
    RCC->APB2PCENR |= RCC_AHBPeriph_DMA1;
    
    // RX DMA into buffers
    DMA1_Channel2->CFGR &= ~DMA_CFGR1_EN;
    DMA1_Channel2->CFGR  = DMA_M2M_Disable
                | DMA_Priority_VeryHigh
                | DMA_MemoryDataSize_Byte
                | DMA_PeripheralDataSize_Byte
                | DMA_MemoryInc_Enable
                | DMA_Mode_Normal
                | DMA_DIR_PeripheralSRC
                | DMA_IT_TC;

    // TX DMA repeats tx buffer 
    DMA1_Channel3->CFGR &= ~DMA_CFGR1_EN;
    DMA1_Channel3->CFGR  = DMA_M2M_Disable
                  | DMA_Priority_VeryHigh
                  | DMA_MemoryDataSize_Byte
                  | DMA_PeripheralDataSize_Byte
                  | DMA_MemoryInc_Enable
                  | DMA_Mode_Circular
                  | DMA_DIR_PeripheralDST
                  | DMA_IT_TC;
}

void spi_put_byte(uint8_t x) {
    SPI1->DATAR = x;
}

uint8_t spi_get_byte(void) {
    return SPI1->DATAR;
}

bool spi_rx_avail(void) {
    return SPI1->STATR & SPI_STATR_RXNE;
}

void spi_start(void) {
    buffer_t rx_buf = buf_next_chunk(BUF_OVERWRITE);

    // RX DMA
    DMA1_Channel2->CFGR &= ~DMA_CFGR1_EN;
    DMA1_Channel2->MADDR = (uint32_t) rx_buf.buf;
    DMA1_Channel2->CNTR = rx_buf.len;
    DMA1_Channel2->CFGR |= DMA_CFGR1_EN;

    // TX DMA
    DMA1_Channel3->CFGR &= ~DMA_CFGR1_EN;
    DMA1_Channel3->MADDR = (uint32_t) tx_buf.buf;
    DMA1_Channel3->CNTR = tx_buf.len;
    DMA1_Channel3->CFGR |= DMA_CFGR1_EN;

    // flush the SPI RX FIFO
    while (spi_rx_avail()) {
        (void) spi_get_byte();
    }

    // Enable SPI as a DMA trigger for TX and RX
    SPI1->CTLR2 |= SPI_CTLR2_TXDMAEN | SPI_CTLR2_RXDMAEN;
    
    // Clear any pending EXTI4 interrupt then enable
    EXTI->INTFR = EXTI_INTF_INTF4;
    EXTI->INTENR |= EXTI_INTENR_MR4;

    // EXTI4 now armed
    // wait for chip-select rising interrupt to begin
}

static void init_dma_tx( DMA_Channel_TypeDef* dma_ch, volatile void *periph, void *buf, u16 bufsize )
{
    RCC->APB2PCENR |= RCC_AHBPeriph_DMA1;

    dma_ch->CFGR &= ~DMA_CFGR1_EN;
    dma_ch->MADDR = (uint32_t) buf;
    dma_ch->PADDR = (uint32_t) periph;
    dma_ch->CNTR  = bufsize;
    dma_ch->CFGR  = DMA_M2M_Disable
                  | DMA_Priority_VeryHigh
                  | DMA_MemoryDataSize_Byte
                  | DMA_PeripheralDataSize_Byte
                  | DMA_MemoryInc_Enable
                  | DMA_Mode_Circular
                  | DMA_DIR_PeripheralDST
                  | DMA_IT_TC;
}

static void init_dma_rx( DMA_Channel_TypeDef* dma_ch, volatile void *periph, void *buf, u16 bufsize )
{
    RCC->APB2PCENR |= RCC_AHBPeriph_DMA1;

    dma_ch->CFGR &= ~DMA_CFGR1_EN;
    dma_ch->MADDR = (uint32_t) buf;
    dma_ch->PADDR = (uint32_t) periph;
    dma_ch->CNTR  = bufsize;
    dma_ch->CFGR  = DMA_M2M_Disable
                  | DMA_Priority_VeryHigh
                  | DMA_MemoryDataSize_Byte
                  | DMA_PeripheralDataSize_Byte
                  | DMA_MemoryInc_Enable
                  | DMA_Mode_Normal
                  | DMA_DIR_PeripheralSRC
                  | DMA_IT_TC;
}

void EXTI4_IRQHandler_impl( void ) __attribute__((interrupt));
void EXTI4_IRQHandler_impl( void )
{
  if(EXTI->INTFR & EXTI_INTF_INTF4)
  {
    EXTI->INTFR = EXTI_INTF_INTF4;
    SPI1->CTLR1 |= SPI_CTLR1_SPE;
    EXTI->INTENR &= ~EXTI_INTENR_MR4;
  }
}

void DMA1_Channel2_IRQHandler( void ) __attribute__((interrupt));
void DMA1_Channel2_IRQHandler( void ) 
{
    if (DMA1->INTFR & DMA1_IT_TC2) {
        DMA1->INTFR = DMA1_IT_TC2;

        buffer_t buf = buf_next_chunk(BUF_OVERWRITE);
        DMA1_Channel2->CFGR &= ~DMA_CFGR1_EN;
        if (buf.len) {
            DMA1_Channel2->MADDR = (uint32_t) buf.buf;
            DMA1_Channel2->CNTR = buf.len;
            DMA1_Channel2->CFGR |= DMA_CFGR1_EN;
        }
    }
}

void DMA1_Channel3_IRQHandler( void ) __attribute__((interrupt));
void DMA1_Channel3_IRQHandler( void ) 
{
    if (DMA1->INTFR & DMA1_IT_TC3) {
        DMA1->INTFR = DMA1_IT_TC3;
    }
}