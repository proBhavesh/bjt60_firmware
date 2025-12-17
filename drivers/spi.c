/*
 * SPI driver implementation
 * Pin mappings from RadarBaseboardMCU7 reference firmware
 *
 * SPI0 on SAMS70Q21:
 *   MISO = PD20 (Peripheral B)
 *   MOSI = PD21 (Peripheral B)
 *   SPCK = PD22 (Peripheral B)
 *   CS   = PA11 (GPIO - manual control)
 */

#include "spi.h"
#include "sams70.h"

/* SPI0 pin definitions - from SAMS70Q21 datasheet */
#define SPI0_MISO_PIN   (1 << 20)  /* PD20 - MISO (Peripheral B) */
#define SPI0_MOSI_PIN   (1 << 21)  /* PD21 - MOSI (Peripheral B) */
#define SPI0_SPCK_PIN   (1 << 22)  /* PD22 - SPCK (Peripheral B) */

/* CS pin on PIOA - from connector.h: csn0 = PA11 */
#define SPI0_CS_PIN     (1 << 11)  /* PA11 - CSN0 (GPIO for manual control) */

/* Track CS state */
static volatile int cs_active = 0;

void spi_init(void)
{
    /* Enable peripheral clocks */
    PMC_PCER0 = (1 << ID_SPI0) | (1 << ID_PIOA) | (1 << ID_PIOD);

    /*
     * Configure SPI pins on PIOD - assign to Peripheral B
     * ABCDSR[0]=1, ABCDSR[1]=0 selects Peripheral B
     */
    PIOD->PIO_PDR = SPI0_MISO_PIN | SPI0_MOSI_PIN | SPI0_SPCK_PIN;
    PIOD->PIO_ABCDSR[0] |= (SPI0_MISO_PIN | SPI0_MOSI_PIN | SPI0_SPCK_PIN);
    PIOD->PIO_ABCDSR[1] &= ~(SPI0_MISO_PIN | SPI0_MOSI_PIN | SPI0_SPCK_PIN);

    /* Configure CS pin (PA11) as GPIO for manual control */
    PIOA->PIO_PER = SPI0_CS_PIN;      /* Enable PIO control */
    PIOA->PIO_OER = SPI0_CS_PIN;      /* Output enable */
    PIOA->PIO_SODR = SPI0_CS_PIN;     /* Set high (deselected) */

    /* Reset SPI */
    SPI0->SPI_CR = SPI_CR_SWRST;
    SPI0->SPI_CR = SPI_CR_SWRST;  /* Reset twice as per datasheet */

    /* Configure SPI mode register
     * - Master mode
     * - Local loopback disabled
     * - Mode fault detection disabled
     * - No peripheral select (using GPIO for CS)
     */
    SPI0->SPI_MR = SPI_MR_MSTR | SPI_MR_MODFDIS;

    /* Configure chip select 0 settings (used as default timing)
     * SCBR: SPI clock = MCK / SCBR
     * MCK = 150 MHz, target ~10 MHz -> SCBR = 15
     * CPOL = 0, NCPHA = 1 (CPHA = 0) -> SPI Mode 0
     * 8 bits per transfer
     * CSAAT: CS stays low between transfers
     */
    SPI0->SPI_CSR[0] = SPI_CSR_SCBR(15) |
                       SPI_CSR_BITS_8 |
                       SPI_CSR_NCPHA |
                       SPI_CSR_CSAAT |
                       SPI_CSR_DLYBCT(0);

    /* Enable SPI */
    SPI0->SPI_CR = SPI_CR_SPIEN;

    cs_active = 0;
}

/*
 * Assert chip select (active low)
 */
void spi_select(void)
{
    PIOA->PIO_CODR = SPI0_CS_PIN;  /* Clear = low = selected */
    cs_active = 1;
}

/*
 * Deassert chip select
 */
void spi_deselect(void)
{
    /* Wait for any pending transfer to complete */
    while (!(SPI0->SPI_SR & SPI_SR_TXEMPTY));

    PIOA->PIO_SODR = SPI0_CS_PIN;  /* Set = high = deselected */
    cs_active = 0;
}

uint8_t spi_transfer(uint8_t data)
{
    /* Wait for TX ready */
    while (!(SPI0->SPI_SR & SPI_SR_TDRE));

    /* Send data */
    SPI0->SPI_TDR = data;

    /* Wait for RX ready */
    while (!(SPI0->SPI_SR & SPI_SR_RDRF));

    /* Return received data */
    return (uint8_t)SPI0->SPI_RDR;
}

void spi_transfer_buffer(const uint8_t *tx_buf, uint8_t *rx_buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        uint8_t tx_data = tx_buf ? tx_buf[i] : 0xFF;
        uint8_t rx_data = spi_transfer(tx_data);
        if (rx_buf) {
            rx_buf[i] = rx_data;
        }
    }
}
