#include "Display.h"

#include <Arduino.h>

/*------------------------------------------------------------------------------
 * DISPLAY CLASS
 *----------------------------------------------------------------------------*/
volatile uint8_t Display::dmaBuffer = 0;
volatile boolean Display::displayAvailable = true;
DMAMEM uint32_t Display::pulseBuffer[2][FRAME_WORDS][4] = {};
DMAMEM uint32_t Display::dmaBufferData[2][BITCOUNT * LEDCOUNT] = {};
DMAMEM uint32_t Display::dmaBufferDataLow[BITCOUNT * LEDCOUNT] = {};
DMAMEM uint32_t Display::dmaBufferDataSecond[2][BITCOUNT * LEDCOUNT] = {};
DMAMEM uint32_t Display::dmaBufferHigh[1] = {0xFFFFFFFF};
DMAMEM uint32_t Display::dmaBufferLow[50] = {};
DMAChannel Display::dmaChannel[2];
DMASetting Display::dmaSetting[6];
uint8_t Display::motionBlur = 240;
uint8_t Display::brightness = 255;
uint32_t Display::cubeBuffer = 0;
DMAMEM Color Display::cube[2][width][height][depth];

static uint8_t mapPhysicalXToCubeX(uint8_t x) {
  return (3 - (x >> 2)) * 4 + (x & 0x03);
}

static uint8_t mapPhysicalYToCubeY(uint8_t y) { return y; }

static uint8_t mapPhysicalZToCubeZ(uint8_t x, uint8_t z) {
  // Physical x 4..7 map to the third logical four-column group.
  if (x >= 4 && x < 8) {
    static constexpr uint8_t THIRD_Z_MAP[16] = {
        5, 4, 7, 6, 1, 0, 3, 2, 13, 12, 15, 14, 11, 10, 9, 8};
    return THIRD_Z_MAP[z];
  }

  // Physical x 8..15 map to the first two logical four-column groups.
  if (x >= 4) {
    static constexpr uint8_t FIRST_TWO_Z_MAP[16] = {
        7, 6, 5, 4, 1, 0, 3, 2, 15, 14, 13, 12, 11, 10, 9, 8};
    return FIRST_TWO_Z_MAP[z];
  }

  uint8_t mapped_z = z;
  if (x >= 4 && x < 8 && (z < 4 || (z >= 8 && z < 12))) {
    mapped_z = (z & 0x0C) | ((z + 2) & 0x03);
  } else if (x >= 4 && z >= 12) {
    mapped_z = (z & 0x0C) | ((z + 2) & 0x03);
  }

  // Full-cube wiring correction measured with the rear-to-front plane test.
  // Physical x 0..3 are the four rightmost columns after the X mapping.
  const uint8_t depth_offset = x < 4 ? 7 : 5;
  const uint8_t corrected_z = (mapped_z + 16 - depth_offset) & 0x0F;
  const uint8_t twelve_column_offset = x < 4 ? 0 : 12;
  return (16 - corrected_z + twelve_column_offset) & 0x0F;
}

void Display::begin() {
  setupRAM();
  setupPLL();
  setupFIO();
  setupDMA();
}

void Display::beginPolledTest() {
  dmaChannel[0].disable();
  dmaChannel[1].disable();
  setupPLL();
  setupFIO(false);
}

static bool writePolledPulse(uint32_t high, uint32_t data) {
  const uint32_t start = ARM_DWT_CYCCNT;
  while ((IMXRT_FLEXIO2_S.SHIFTSTAT & 0x0F) != 0x0F) {
    if (ARM_DWT_CYCCNT - start > F_CPU_ACTUAL / 1000) return false;
  }
  IMXRT_FLEXIO2_S.SHIFTBUFBIS[0] = high;
  IMXRT_FLEXIO2_S.SHIFTBUFBIS[1] = data;
  IMXRT_FLEXIO2_S.SHIFTBUFBIS[2] = data;
  IMXRT_FLEXIO2_S.SHIFTBUFBIS[3] = 0;
  return true;
}

bool Display::testAllChannelsPolled(uint32_t color_bits, uint32_t &shift_errors) {
  uint32_t interrupt_mask;
  asm volatile("mrs %0, primask" : "=r" (interrupt_mask) :: "memory");
  __disable_irq();
  bool sent = true;
  // Idle underruns are expected with no DMA. Prime with low words, then
  // measure only the continuous data/reset transfer for missed refills.
  for (uint16_t i = 0; i < 200 && sent; ++i) sent = writePolledPulse(0, 0);
  IMXRT_FLEXIO2_S.SHIFTERR = 0x0F;
  for (uint16_t led = 0; led < LEDCOUNT && sent; ++led) {
    for (uint8_t bit = 0; bit < BITCOUNT && sent; ++bit) {
      const uint32_t data = (color_bits & (0x80000000u >> bit)) ? 0xFFFFFFFFu : 0;
      sent = writePolledPulse(0xFFFFFFFFu, data);
    }
  }
  for (uint16_t i = 0; i < 200 && sent; ++i) sent = writePolledPulse(0, 0);
  shift_errors = IMXRT_FLEXIO2_S.SHIFTERR & 0x0F;
  if (!interrupt_mask) __enable_irq();
  return sent;
}

// DMAMEM can't be (static) initialized, need to do this in code
void Display::setupRAM(void) {
  memset(dmaBufferData, 0, sizeof(dmaBufferData));
  memset(pulseBuffer, 0, sizeof(pulseBuffer));
  arm_dcache_flush(pulseBuffer, sizeof(pulseBuffer));
  memset(dmaBufferDataLow, 0, sizeof(dmaBufferDataLow));
  memset(dmaBufferDataSecond, 0, sizeof(dmaBufferDataSecond));
  memset(dmaBufferHigh, -1, sizeof(dmaBufferHigh));
  memset(dmaBufferLow, 0, sizeof(dmaBufferLow));
  memset(cube, 0, sizeof(cube));
  arm_dcache_flush(dmaBufferData, sizeof(dmaBufferData));
  arm_dcache_flush(dmaBufferDataLow, sizeof(dmaBufferDataLow));
  arm_dcache_flush(dmaBufferDataSecond, sizeof(dmaBufferDataSecond));
  arm_dcache_flush(dmaBufferHigh, sizeof(dmaBufferHigh));
  arm_dcache_flush(dmaBufferLow, sizeof(dmaBufferLow));
}
// The channel stops after the trailing low words, before buffer ownership changes.
void Display::displayReady(void) {
  // First clear the interrupt flag to avoid retriggering
  dmaChannel[0].clearInterrupt();
  // Swap dma buffer if a new one is available
  if (!displayAvailable) {
    // The prep buffer becomes the dma buffer and visa versa
    dmaBuffer = 1 - dmaBuffer;
  }
  dmaChannel[0].TCD->SADDR = pulseBuffer[dmaBuffer];
  dmaChannel[0].clearComplete();
  dmaChannel[0].enable();
  // The display is available to accept cube data
  displayAvailable = true;
}

// Notifies the display that a new frame is ready for displaying. Transfer
// the cube data to the prep buffer and enable the interrupt to swap the
// buffers.
//
// Note : In principle every 24 bit of led data (3 bytes) is rotated and
// the 3 bytes become bits in 24 bytes with offset chn. Maybe this can be
// made non-blocking using the pixel pipeline (pxp)?
//
// Maybe the rotate left can be implemented in assembly? Not sure if the
// compiler is smart enough to optimize the shifts as a rotation.
void Display::update() {
  if (displayAvailable) {
    uint32_t *prepBuffer = dmaBufferData[1 - dmaBuffer];
    memset(prepBuffer, 0, sizeof(dmaBufferData[0]));
    for (uint8_t x = 0; x < width; x++) {
      for (uint8_t y = 0; y < height; y++) {
        const uint8_t cube_x = mapPhysicalXToCubeX(x);
        const uint8_t cube_y = mapPhysicalYToCubeY(y);
        uint8_t led = 0x7F - (x << 4 & 0x30) - (((0x10 - (x & 1)) ^ y) & 0x0F);
        for (uint8_t z = 0; z < depth; z++) {
          const uint8_t cube_z = mapPhysicalZToCubeZ(x, z);
          led = 0x7F - led;
          uint32_t *offset = prepBuffer + led * BITCOUNT;
          uint8_t chn = (x >> 1 & 0x0E) + (z << 1 & 0xF8) + (z >> 1 & 1);
          uint32_t value = cube[cubeBuffer][cube_x][cube_y][cube_z]
                               .blend(motionBlur,
                                      cube[1 - cubeBuffer][cube_x][cube_y][cube_z])
                               .scale(brightness)
                               .bits();
          const uint8_t rotation = chn + 1;
          if (rotation < 32) {
            value = (value << rotation) | (value >> (32 - rotation));
          }
          uint32_t mask = 1 << chn;
          for (uint8_t i = 0; i < BITCOUNT; i++) {
            *offset++ |= (value & mask);
            value = (value << 1) | (value >> 31);
          }
        }
      }
    }
    cubeBuffer = 1 - cubeBuffer;
    prepareFrame();
    // Writing to the cube data is not allowed to prevent frame tearing
    displayAvailable = false;
  }
}

void Display::testAllChannels(uint32_t color_bits) {
  if (displayAvailable) {
    uint32_t *prepBuffer = dmaBufferData[1 - dmaBuffer];
    uint32_t *prepBufferSecond = dmaBufferDataSecond[1 - dmaBuffer];
    for (uint16_t led = 0; led < LEDCOUNT; led++) {
      for (uint8_t bit = 0; bit < BITCOUNT; bit++) {
        uint32_t value =
            (color_bits & (0x80000000 >> bit)) ? 0xFFFFFFFF : 0x00000000;
        prepBuffer[led * BITCOUNT + bit] = value;
        prepBufferSecond[led * BITCOUNT + bit] = value;
      }
    }
    prepareFrame();
    displayAvailable = false;
  }
}

void Display::testChannel(uint8_t channel, uint32_t color_bits) {
  if (channel >= 32 || !displayAvailable) return;

  uint32_t *prepBuffer = dmaBufferData[1 - dmaBuffer];
  const uint32_t channelMask = 1u << channel;
  for (uint16_t led = 0; led < LEDCOUNT; ++led) {
    for (uint8_t bit = 0; bit < BITCOUNT; ++bit) {
      prepBuffer[led * BITCOUNT + bit] =
          (color_bits & (0x80000000u >> bit)) ? channelMask : 0;
    }
  }
  prepareFrame();
  displayAvailable = false;
}

void Display::prepareFrame() {
  const uint8_t next = 1 - dmaBuffer;
  for (uint16_t bit = 0; bit < BITCOUNT * LEDCOUNT; ++bit) {
    // Narrow-pulse WCK starts its 32-channel cycle 17 positions later than
    // the former square wave. Rotate the serialized channel word so logical
    // and physical channel numbering remains unchanged.
    const uint32_t channelData = dmaBufferData[next][bit];
    const uint32_t alignedData = (channelData << 17) | (channelData >> 15);
    pulseBuffer[next][bit][0] = 0xFFFFFFFF;
    pulseBuffer[next][bit][1] = alignedData;
#if defined PL9823
    pulseBuffer[next][bit][2] = alignedData;
#else
    pulseBuffer[next][bit][2] = 0;
#endif
    pulseBuffer[next][bit][3] = 0;
  }
  arm_dcache_flush(pulseBuffer[next], sizeof(pulseBuffer[0]));
  asm volatile("dsb" ::: "memory");
}
// Check if the display is available to accept new cube data
bool Display::available() { return displayAvailable; }
// Clear the cube so a new frame can be freshly created.
void Display::clear() { memset(cube[cubeBuffer], 0, sizeof(cube[0])); }
// Set the master display brightness value
void Display::setBrightness(const uint8_t value) { brightness = value; }
uint8_t Display::getBrightness() { return brightness; }
// Set the motion blur value
void Display::setMotionBlur(const uint8_t value) { motionBlur = value; }
uint8_t Display::getMotionBlur() { return motionBlur; }
/****************************************************************************
 * Set up PLL5 (also known as "VIDEO PLL")
 * This configures the Clock Controller Module (CCM)
 *
 * The internal pll clock is set to 24MHz, the frequency generated is
 * 24 * (DIV_SELECT + NUM/DENOM) -> 24 * (42 + 2/3) = 1024 MHz
 * PL9823 diagnostic: 24 * 40 = 960 MHz, matching the measured GPIO test.
 ***************************************************************************/
void Display::setupPLL() {
  // Before disabeling the PLL set the bypass source to the internal
  // 24MHz reference clock. See 14.6.1.6 page 1039.
  CCM_ANALOG_PLL_VIDEO_CLR = CCM_ANALOG_PLL_VIDEO_BYPASS_CLK_SRC(3) |
                             // Clear power down bit to power up the PLL
                             CCM_ANALOG_PLL_VIDEO_POWERDOWN;
  // Bypass the PLL also see 13.3.2.2.1 page 987
  CCM_ANALOG_PLL_VIDEO_SET = CCM_ANALOG_PLL_VIDEO_BYPASS;
  // Disable the Video PLL output before configurating
  CCM_ANALOG_PLL_VIDEO_CLR = CCM_ANALOG_PLL_VIDEO_ENABLE;
  // Clear dividers before setting the values
  CCM_ANALOG_PLL_VIDEO_CLR = CCM_ANALOG_PLL_VIDEO_DIV_SELECT(0x7f) |
                             CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(3);
#if defined PL9823
  // NUM = 30 bits signed number, abs(NUM) must be less than DENOM
  CCM_ANALOG_PLL_VIDEO_NUM = 0;
  // DENOM = 30 bits unsigned number. NUM/DENOM -> fracional loop diver
  CCM_ANALOG_PLL_VIDEO_DENOM = 1;
#else
  // NUM = 30 bits signed number, abs(NUM) must be less than DENOM
  CCM_ANALOG_PLL_VIDEO_NUM = 2;
  // DENOM = 30 bits unsigned number. NUM/DENOM -> fracional loop diver
  CCM_ANALOG_PLL_VIDEO_DENOM = 3;
#endif
  // Clear dividers before setting the values
  CCM_ANALOG_MISC2_CLR = CCM_ANALOG_MISC2_VIDEO_DIV(3);
  // Post-divider for video values (0=/1, 1=/2, 2=/1, 3=/4) (2 is also
  // /1)
  CCM_ANALOG_MISC2_SET = CCM_ANALOG_MISC2_VIDEO_DIV(0);

  CCM_ANALOG_PLL_VIDEO_SET =
  // DIV_SELECT set the loop divider (27-54)
#if defined PL9823
      CCM_ANALOG_PLL_VIDEO_DIV_SELECT(40) |
#else
      CCM_ANALOG_PLL_VIDEO_DIV_SELECT(42) |
#endif
      // Divider after PLL (0=/4, 1=/2, 2=/1, 3=reserved)
      CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(2) |
      // Enable PLL output (still bypassed)
      CCM_ANALOG_PLL_VIDEO_ENABLE;

  // Wait for the PLL to lock. Prevents random initial edges 13.3.2.2.1
  while ((CCM_ANALOG_PLL_VIDEO & CCM_ANALOG_PLL_VIDEO_LOCK) == 0) {
  }
  // Disable bypass for Video PLL once it's locked.
  CCM_ANALOG_PLL_VIDEO_CLR = CCM_ANALOG_PLL_VIDEO_BYPASS;
}
/*******************************************************************************
 * Configure flexio
 *
 * Pad settings:
 *   HYS Hysteresis enable = 0 (0=disabled, 1=enabled)
 *   PUS pull up/down config select = 0
 *     (0=100K pull down, 1=47K pull up, 2=100K pull up, 3=22K pull up)
 *   PUE Keeper select = 0 (0=keeper, 1=pull)
 *   PKE Pull keeper enable = 0 (0=disabled, 1=enabled)
 *   ODE Open drain enable = 0 (0=disabled, 1=enabled)
 *   SPEED speed = 1
 *     (0=50MHz, 1=100MHz, 2=150MHz, 3=200MHz)
 *     (Increases output driver current or reduce switching noise)
 *   DSE drive strength = x (should be impedance matched)
 *     (0 = off, 1=150/1 Ohm, 2=150/2 Ohm, 3=150/3 Ohm, ... 7=150/7 Ohm)
 *     (tr -> 5=1.70ns, 3=2.35ns, 2=3.13ns, 1=5.14ns @ SRE=0)
 *     (tr -> 5=1.06ns, 3=1.74ns, 2=2.46ns, 1=4.77ns @ SRE=1)
 *     (Transition rise and fall time are DSE & SRE dependent)
 *   SRE slew rate = 0 (0=slow, 1=fast)
 ******************************************************************************/
void Display::setupFIO(bool useDMA) {
  *portModeRegister(DIN) |= digitalPinToBitMask(DIN);
  // Match the drive strength used by pinMode(OUTPUT) in the working GPIO test.
  *portControlRegister(DIN) = IOMUXC_PAD_DSE(7) | IOMUXC_PAD_SPEED(1);
  // SION + ALT4 (FLEXIO2_FLEXIO16) (IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_00)
  *portConfigRegister(DIN) = 0x14;

  *portModeRegister(WCK) |= digitalPinToBitMask(WCK);
  *portControlRegister(WCK) = IOMUXC_PAD_DSE(7) | IOMUXC_PAD_SPEED(1);
  // SION + ALT4 (FLEXIO2_FLEXIO11) (IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_11)
  *portConfigRegister(WCK) = 0x14;

  *portModeRegister(BCK) |= digitalPinToBitMask(BCK);
  *portControlRegister(BCK) = IOMUXC_PAD_DSE(7) | IOMUXC_PAD_SPEED(1);
  // SION + ALT4 (FLEXIO2_FLEXIO00) (IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_00)
  *portConfigRegister(BCK) = 0x14;

  // Disable flexio2 clock gate see 14.7.24 page 1087
  CCM_CCGR3 &= ~CCM_CCGR3_FLEXIO2(CCM_CCGR_ON);
  // Serial Clock Multiplexer Register 2
  CCM_CSCMR2 &= ~CCM_CSCMR2_FLEXIO2_CLK_SEL(3);
  // Derive clock from PLL5 clock 14.7.8 page 1059
  CCM_CSCMR2 |= CCM_CSCMR2_FLEXIO2_CLK_SEL(2);
  // Clock divider register pre divider
  CCM_CS1CDR &= ~CCM_CS1CDR_FLEXIO2_CLK_PRED(7);
  // Divide by (4 + 1 = 5)
  CCM_CS1CDR |= CCM_CS1CDR_FLEXIO2_CLK_PRED(4);
  // Clock divider register post divider
  CCM_CS1CDR &= ~CCM_CS1CDR_FLEXIO2_CLK_PODF(7);
  // Divide by (0 + 1 = 1)
  CCM_CS1CDR |= CCM_CS1CDR_FLEXIO2_CLK_PODF(0);
  // Enable flexio2 clock gate see 14.7.24 page 1087
  CCM_CCGR3 |= CCM_CCGR3_FLEXIO2(CCM_CCGR_ON);

  // Shifter control 50.5.1.14 page 2925
  // SHIFTER0 is configured to output on DIN = pin 8
  IMXRT_FLEXIO2_S.SHIFTCTL[0] =
      // Timer select -> TIMER0 controls logic and shift clock
      FLEXIO_SHIFTCTL_TIMSEL(0) |
      // Shift on the internal positive edge. BCK is inverted below so DIN
      // still changes half a BCK cycle before the external rising edge.
      // Shifter pin configuration -> shifter pin output
      FLEXIO_SHIFTCTL_PINCFG(3) |
      // Shifter pin select FLEXIO16 pin (DIN pin)
      FLEXIO_SHIFTCTL_PINSEL(16) |
      // Shifter pin polarity -> pin is active high
      (FLEXIO_SHIFTCTL_PINPOL & 0) |
      // Shifter mode -> Transmit mode. Load SHIFTBUF contents into
      // the shifter on expiration of the Timer
      FLEXIO_SHIFTCTL_SMOD(2);
  // SHIFTERS 1-3 do not output to a pin
  IMXRT_FLEXIO2_S.SHIFTCTL[1] = FLEXIO_SHIFTCTL_SMOD(2);
  IMXRT_FLEXIO2_S.SHIFTCTL[2] = FLEXIO_SHIFTCTL_SMOD(2);
  IMXRT_FLEXIO2_S.SHIFTCTL[3] = FLEXIO_SHIFTCTL_SMOD(2);

  // Shifter configuration 50.5.1.15 page 2927
  // SHIFTER0 shifts 1 bit on each clock and has SHIFTER(0+1) as source
  IMXRT_FLEXIO2_S.SHIFTCFG[0] =
      // 1-bit shift on each shift clock
      FLEXIO_SHIFTCFG_PWIDTH(0) |
      // Input source for shifter is output of Shifter N+1
      FLEXIO_SHIFTCFG_INSRC;
  // Same for shifters 1 - 3
  IMXRT_FLEXIO2_S.SHIFTCFG[1] = FLEXIO_SHIFTCFG_INSRC;
  IMXRT_FLEXIO2_S.SHIFTCFG[2] = FLEXIO_SHIFTCFG_INSRC;
  // Illegal input source for shifter 3 ??????
  IMXRT_FLEXIO2_S.SHIFTCFG[3] = FLEXIO_SHIFTCFG_INSRC;

  // Timer configuration 50.5.1.21.4 page 2935
  IMXRT_FLEXIO2_S.TIMCFG[0] =
      // Timer output is logic 1 when enabled, not affected by reset
      FLEXIO_TIMCFG_TIMOUT(0) |
      // Decrement counter on flexio clock, shift clock = timer output
      FLEXIO_TIMCFG_TIMDEC(0) |
      // Timer never resets
      FLEXIO_TIMCFG_TIMRST(0) |
      // Timer never disabled
      FLEXIO_TIMCFG_TIMDIS(0) |
      // Timer enabled on trigger high (shifter 0 status flag)
      FLEXIO_TIMCFG_TIMENA(2) |
      // Stop bit disabled
      FLEXIO_TIMCFG_TSTOP(0) |
      // Start bit disabled
      (FLEXIO_TIMCFG_TSTART & 0);
  IMXRT_FLEXIO2_S.TIMCFG[1] =
      // Timer enabled on timer N-1 enable
      FLEXIO_TIMCFG_TIMENA(1);

  // Timer control 50.5.1.20 page 2932
  // Triggered after loading SHIFTER0 from SHIFTBUF0 50.5.1.16.3 page
  // 2929 TIMER0 is configured to output on BCK = pin 10
  IMXRT_FLEXIO2_S.TIMCTL[0] =
      // Trigger select -> triggers on SHIFTER[N=0] status flag (4*N+1)
      FLEXIO_TIMCTL_TRGSEL(1) |
      // Trigger polarity -> trigger active low
      FLEXIO_TIMCTL_TRGPOL |
      // Trigger source -> internal trigger selected
      FLEXIO_TIMCTL_TRGSRC |
      // Timer pin configuration -> timer pin output
      FLEXIO_TIMCTL_PINCFG(3) |
      // Timer pin select -> FLEXIO00 (BCK on pin 10)
      FLEXIO_TIMCTL_PINSEL(0) |
      // Invert external BCK to move its rising edge away from the WCK latch.
      FLEXIO_TIMCTL_PINPOL |
      // Timer mode -> dual 8 bit counters baud mode
      FLEXIO_TIMCTL_TIMOD(1);
  // No trigger is used, timer is enabled on TIMER0.
  // TIMER1 outputs WCK on pin 9.
  IMXRT_FLEXIO2_S.TIMCTL[1] =
      // Timer pin configuration -> timer pin output
      FLEXIO_TIMCTL_PINCFG(3) |
      // Timer pin select -> FLEXIO11 (WCK on pin 9)
      FLEXIO_TIMCTL_PINSEL(11) |
      // Latch phase that keeps all 32 channel positions correctly aligned.
      FLEXIO_TIMCTL_PINPOL |
      // Timer mode -> dual 8-bit PWM mode. A narrow WCK pulse avoids an
      // unnecessary edge halfway through the 32-channel shift cycle.
      FLEXIO_TIMCTL_TIMOD(2);

  // Timer compare 50.5.1.22 page 2937
  // The upper 8 bits configure the number of bits = (cmp[15:8] + 1) / 2
  // Upper 8 bits -> 4 x 32 bits -> 128 * 2 -> 256 - 1 = 0xFF voor 128
  // bits The lower 8 bits configure baud rate divider = (cmp[ 7:0] + 1)
  // * 2 Lower 8 bits -> (0 + 1) * 2 -> divide frequency by 2
  IMXRT_FLEXIO2_S.TIMCMP[0] = 0x0000FF00;
  // 63 FlexIO clocks low plus one high clock is 64 FlexIO clocks, exactly
  // one WCK period per 32 BCK pulses.
  IMXRT_FLEXIO2_S.TIMCMP[1] = 0x0000003E;

  // Shiftbuffers 1 & 2 get filled by DMA later. Start with all zero's
  // to reset/latch the led's. See 50.5.1.6.3 page 2918 for DMA
  // triggering.
  IMXRT_FLEXIO2_S.SHIFTBUFBIS[0] = 0x00000000;
  IMXRT_FLEXIO2_S.SHIFTBUFBIS[1] = 0x00000000;
  IMXRT_FLEXIO2_S.SHIFTBUFBIS[2] = 0x00000000;
  IMXRT_FLEXIO2_S.SHIFTBUFBIS[3] = 0x00000000;
  // Enable DMA trigger when SHIFTBUF[1 or 2] is loaded onto the shifter
  IMXRT_FLEXIO2_S.SHIFTSDEN = useDMA ? 0x02 : 0;
  // Enable flexio, SHIFTBUF[0] has been written, so TIMER0 will start
  // and other buffers are also ready so no errors in shifting data
  IMXRT_FLEXIO2_S.CTRL = FLEXIO_CTRL_FLEXEN;
}

void Display::setupDMA() {
  dmaChannel[0].disable();
  dmaChannel[1].disable();
  dmaChannel[0].sourceBuffer(&pulseBuffer[dmaBuffer][0][0],
                             sizeof(pulseBuffer[0]));
  dmaChannel[0].destination(IMXRT_FLEXIO2_S.SHIFTBUFBIS[0]);
  // One request loads High, Data, Data, Low. Wrap the destination every
  // 16 bytes so the next request starts at shifter 0 again.
  dmaChannel[0].TCD->DOFF = 4;
  dmaChannel[0].TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) |
      DMA_TCD_ATTR_DSIZE(2) | DMA_TCD_ATTR_DMOD(4);
  dmaChannel[0].TCD->NBYTES_MLNO = 16;
  dmaChannel[0].TCD->CITER_ELINKNO = FRAME_WORDS;
  dmaChannel[0].TCD->BITER_ELINKNO = FRAME_WORDS;
  dmaChannel[0].disableOnCompletion();
  dmaChannel[0].interruptAtCompletion();
  dmaChannel[0].triggerAtHardwareEvent(DMAMUX_SOURCE_FLEXIO2_REQUEST1);
  dmaChannel[0].attachInterrupt(&displayReady);
  dmaChannel[0].enable();
}
#if 0
// Previous two-channel diagnostic retained for comparison during this test.
void Display::setupDMAOld() {
  // TCD 0 transfers the active dma buffer data to SHIFTBUF[1].
  dmaSetting[0].sourceBuffer(dmaBufferData[dmaBuffer],
                             sizeof(dmaBufferData[0]));
  dmaSetting[0].destination(IMXRT_FLEXIO2_S.SHIFTBUFBIS[1]);
  dmaSetting[0].replaceSettingsOnCompletion(dmaSetting[1]);
  // Synchronize buffer swapping after the reset/latch cycle, when both DMA
  // channels can be restarted from a clean frame boundary.
  // TCD 1 sets SHIFTBUF[0] Low. The DMA channel is triggered by FlexIO
  // shifter 1. A write to SHIFTBUF[0] doesn't clear this trigger. The
  // DMA channel is triggered again upon completion of TCD1
  dmaSetting[1].sourceBuffer(dmaBufferLow, 4);
  dmaSetting[1].destination(IMXRT_FLEXIO2_S.SHIFTBUFBIS[0]);
  dmaSetting[1].replaceSettingsOnCompletion(dmaSetting[2]);
  // TCD 2 sets SHIFTBUF[1] to 0. This is to reset the leds.
  // We need >50us of zeros so adjust buffer size accordingly.
  dmaSetting[2].sourceBuffer(dmaBufferLow, sizeof(dmaBufferLow));
  dmaSetting[2].destination(IMXRT_FLEXIO2_S.SHIFTBUFBIS[1]);
  dmaSetting[2].replaceSettingsOnCompletion(dmaSetting[3]);
  // TCD 3 sets SHIFTBUF[0] High. Triggers again.
  dmaSetting[3].sourceBuffer(dmaBufferHigh, 4);
  dmaSetting[3].destination(IMXRT_FLEXIO2_S.SHIFTBUFBIS[0]);
  dmaSetting[3].replaceSettingsOnCompletion(dmaSetting[0]);
  dmaSetting[3].interruptAtCompletion();

  // TCD 4 = TCD 0 for SHIFBUF[2]
  dmaSetting[4].sourceBuffer(dmaBufferDataSecond[dmaBuffer],
                             sizeof(dmaBufferDataSecond[0]));
  dmaSetting[4].destination(IMXRT_FLEXIO2_S.SHIFTBUFBIS[2]);
  dmaSetting[4].replaceSettingsOnCompletion(dmaSetting[5]);
  // TCD 5 = TCD 0 for SHIFBUF[2]
  dmaSetting[5].sourceBuffer(dmaBufferLow, sizeof(dmaBufferLow));
  dmaSetting[5].destination(IMXRT_FLEXIO2_S.SHIFTBUFBIS[2]);
  dmaSetting[5].replaceSettingsOnCompletion(dmaSetting[4]);

  // Initialize both DMA channels
  dmaChannel[0].disable();
  dmaChannel[1].disable();
  // Start with the reset/latch signal, so we can begin "fresh"
  dmaChannel[0] = dmaSetting[2];
  dmaChannel[1] = dmaSetting[4];
  dmaChannel[0].triggerAtHardwareEvent(DMAMUX_SOURCE_FLEXIO2_REQUEST1);
  // Attach interrupt to dma channel 0. The interrupt is enabled in
  // update and disabled again in the ISR.
  dmaChannel[0].attachInterrupt(&displayReady);
  // Bit pattern for PL9823 leds: High, Data, Data, Low
  // Bit pattern for WS2812 leds: High, Data, Low, Low
  dmaChannel[1].triggerAtTransfersOf(dmaSetting[0]);
  dmaChannel[0].enable();
  dmaChannel[1].enable();
}
#endif
