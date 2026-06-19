#include "analog.h"
#include "main.h"

/* -------------------------------------------------------------------------
 * MUX hardware — SN74LV4051APWR 8:1, x9 units
 * PA0=S2 (MSB), PA1=S1, PA2=S0 (LSB)
 * ------------------------------------------------------------------------- */
#define MUX_SEL_PORT  GPIOA
#define MUX_S0_PIN    GPIO_PIN_2
#define MUX_S1_PIN    GPIO_PIN_1
#define MUX_S2_PIN    GPIO_PIN_0

/* 2µs settle: tPD_max=35ns, so this is 57× margin.
 * Extra headroom for PCB parasitics and VDDA noise from USB to settle
 * before the ADC sampling capacitor charges. */
#define MUX_SETTLE_US 2U

/* ADC — OH49E: Rout=20Ω, MUX Ron=70Ω, STM32 Rin≈100Ω, Cadc=4pF.
 * 12-bit settle time: ln(2^13) × 190Ω × 4pF = 6.8ns.
 * 15-cycle sample (625ns at 24MHz) = 90× margin over settle requirement.
 * Chosen to integrate over USB 12MHz noise: 625ns ≈ 7.5 bit periods. */
#define ADC_SAMPLE_TIME ADC_SAMPLETIME_15CYCLES

#define NUM_MUX      9
#define MUX_CHANNELS 8

/* ADC1 channels for each MUX output AM1–AM9 */
static const uint32_t mux_adc_channels[NUM_MUX] = {
    ADC_CHANNEL_10,  /* AM1  PC0 */
    ADC_CHANNEL_11,  /* AM2  PC1 */
    ADC_CHANNEL_12,  /* AM3  PC2 */
    ADC_CHANNEL_6,   /* AM4  PA6 */
    ADC_CHANNEL_7,   /* AM5  PA7 */
    ADC_CHANNEL_14,  /* AM6  PC4 */
    ADC_CHANNEL_15,  /* AM7  PC5 */
    ADC_CHANNEL_8,   /* AM8  PB0 */
    ADC_CHANNEL_9,   /* AM9  PB1 */
};

/* sensor_map[mux][channel] = key index 0–60, or NO_KEY.
 * NC? entries (AM2 Y1/Y4/Y6, AM3 Y4, AM4 Y1/Y4–Y7, AM5 Y2,
 *              AM6 Y2, AM7 Y4, AM9 Y4) — verify against schematic. */
static const uint8_t sensor_map[NUM_MUX][MUX_CHANNELS] = {
    /* AM1   Y0  Y1  Y2  Y3  Y4  Y5  Y6  Y7 */
    {         1,  0, 14,  2,  3,  6,  4,  5 },
    /* AM2 */
    {        16, NO_KEY, 15, 17, NO_KEY, 20, NO_KEY, 19 },
    /* AM3 */
    {        43, 42, 41, 28, NO_KEY, 31, 29, 30 },
    /* AM4 */
    {        55, NO_KEY, 54, 56, NO_KEY, NO_KEY, NO_KEY, NO_KEY },
    /* AM5 */
    {        12, 11, NO_KEY, 13,  7, 10,  8,  9 },
    /* AM6 */
    {        26, 25, NO_KEY, 27, 21, 24, 22, 23 },
    /* AM7 */
    {        46, 45, 44, 47, NO_KEY, 34, 32, 33 },
    /* AM8 */
    {        40, 49, 48, 39, 35, 38, 36, 37 },
    /* AM9 */
    {        59, 58, 57, 60, NO_KEY, 52, 50, 51 },
};

static uint16_t adc_values[NUM_KEYS];
static uint32_t settle_ticks;

volatile uint8_t scan_ready;

static void delay_ticks(uint32_t ticks)
{
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < ticks) {}
}

static void mux_select(uint8_t ch)
{
    static const uint32_t all = MUX_S0_PIN | MUX_S1_PIN | MUX_S2_PIN;
    uint32_t set = ((ch & 0x01) ? MUX_S0_PIN : 0U)
                 | ((ch & 0x02) ? MUX_S1_PIN : 0U)
                 | ((ch & 0x04) ? MUX_S2_PIN : 0U);
    MUX_SEL_PORT->BSRR = set | ((all & ~set) << 16);
    delay_ticks(settle_ticks);
}

static uint16_t adc_read(uint32_t channel)
{
    static ADC_ChannelConfTypeDef cfg = {
        .Rank         = 1,
        .SamplingTime = ADC_SAMPLE_TIME,
    };
    cfg.Channel = channel;
    HAL_ADC_ConfigChannel(&hadc1, &cfg);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 1);
    return (uint16_t)HAL_ADC_GetValue(&hadc1);
}

void analog_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL        |= DWT_CTRL_CYCCNTENA_Msk;
    settle_ticks = MUX_SETTLE_US * (SystemCoreClock / 1000000U);
}

bool analog_task(void)
{
    if (!scan_ready) return false;
    scan_ready = 0;

    for (uint8_t ch = 0; ch < MUX_CHANNELS; ch++) {
        mux_select(ch);
        for (uint8_t m = 0; m < NUM_MUX; m++) {
            uint8_t key = sensor_map[m][ch];
            if (key == NO_KEY) continue;
            adc_values[key] = adc_read(mux_adc_channels[m]);
        }
    }
    return true;
}

uint16_t analog_read(uint8_t key)
{
    return adc_values[key];
}
