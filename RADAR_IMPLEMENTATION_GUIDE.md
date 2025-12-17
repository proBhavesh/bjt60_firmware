# Avian Radar Driver Implementation Guide

## Overview

The Avian BGT60TR13C radar requires extensive configuration across 100+ SPI registers. This guide shows how to extract and implement the full initialization.

## Register Map Summary

From `registers_BGT60TRxxC.h`:
- **MAIN (0x00)**: Frame control, reset
- **ADC0-ADC5 (0x01, 0x50-0x54)**: ADC configuration
- **PACR1/PACR2 (0x04-0x05)**: Power and analog control
- **CSP_I/CSP_D (0x08-0x0F)**: Chirp shape pulses
- **CS1-CS4 (0x10-0x2B)**: Chirp sequences (4 independent sequences)
- **PLL1-PLL4 (0x30-0x4F)**: 4 PLLs, 8 registers each (32 total)
- **RFT, DFT, etc. (0x55-0x5F)**: Timing and status

## Required Configuration for Presence Detection

### Target Parameters
```
Frequency: 58-61 GHz (3 GHz bandwidth)
Samples: 64 per chirp
Chirps: 32 per frame
Sample rate: 1 MHz
Chirp time: ~64 µs
Frame rate: 5 Hz (200 ms)
TX power: 31 (maximum)
IF gain: 33 dB
```

### Step-by-Step Implementation

#### 1. Extract Configuration from Python Example

File: `../radar_sdk/examples/py/FMCW/presence_detection.py` (lines 144-161)

```python
config = FmcwSimpleSequenceConfig(
    frame_repetition_time_s=1 / 5,  # 5 Hz
    chirp_repetition_time_s=0.000411238,  # 411.238 µs
    num_chirps=32,
    tdm_mimo=False,
    chirp=FmcwSequenceChirp(
        start_frequency_Hz=58_000_000_000,  # 58 GHz
        end_frequency_Hz=61_232_137_439,    # 61.232 GHz
        sample_rate_Hz=1e6,                 # 1 MHz
        num_samples=64,
        rx_mask=1,  # RX antenna 1
        tx_mask=1,  # TX antenna 1
        tx_power_level=31,
        lp_cutoff_Hz=500000,  # 500 kHz low-pass
        hp_cutoff_Hz=80000,   # 80 kHz high-pass
        if_gain_dB=33,
    )
)
```

#### 2. Use Infineon's High-Level Driver

**Recommended Approach**: Instead of manual register programming, use the driver from the reference firmware.

Extract these files from `../firmware_extracted/Firmware/sources/`:
```
stratula/library/components/radar/
├── Avian.c/h              - Main driver
├── DataAvian.c/h          - Data acquisition
└── Driver/ (from radar_sdk/external/lib_avian/)
    ├── registers_BGT60TRxxC.h
    ├── _configuration.c/h  - Configuration helper
    └── ifxRadar_*.c/h      - Low-level functions
```

#### 3. Key Driver Functions to Port

From `Avian.c`:
```c
// Initialize device
ifx_Error_t Avian_Constructor(Avian_t* handle);

// Configure frequency and chirp
ifx_Error_t Avian_configure_chirp_settings(
    Avian_t* handle,
    const ifx_Avian_Config_t* config
);

// Start data acquisition
ifx_Error_t Avian_startData(Avian_t* handle);

// Read frame from FIFO
ifx_Error_t Avian_readFifo(
    Avian_t* handle,
    uint16_t* data,
    uint32_t num_samples
);
```

#### 4. Simplified Configuration Structure

Create a minimal config structure:

```c
typedef struct {
    uint64_t start_frequency_Hz;
    uint64_t end_frequency_Hz;
    uint32_t sample_rate_Hz;
    uint16_t num_samples_per_chirp;
    uint16_t num_chirps_per_frame;
    uint8_t  tx_power;
    uint8_t  if_gain_dB;
    uint8_t  rx_mask;
    uint8_t  tx_mask;
} avian_simple_config_t;

const avian_simple_config_t presence_config = {
    .start_frequency_Hz = 58000000000ULL,
    .end_frequency_Hz = 61232137439ULL,
    .sample_rate_Hz = 1000000,
    .num_samples_per_chirp = 64,
    .num_chirps_per_frame = 32,
    .tx_power = 31,
    .if_gain_dB = 33,
    .rx_mask = 0x01,  // RX1
    .tx_mask = 0x01,  // TX1
};
```

#### 5. Critical Register Sequences

**Power-On Sequence**:
```c
// 1. Hardware reset
gpio_radar_reset_low();
delay_ms(10);
gpio_radar_reset_high();
delay_ms(50);

// 2. Software reset
avian_write_reg(BGT60TRxxC_REG_MAIN, 0x10);  // SW_RESET
delay_ms(10);

// 3. Configure analog power (MUST be done first)
avian_write_reg(BGT60TRxxC_REG_PACR1, 0xEF67FD);
avian_write_reg(BGT60TRxxC_REG_PACR2, 0x080074);
```

**PLL Configuration** (most complex part):
```c
// Frequency calculation:
// f = f_ref * (N + k/2^20)
// For 58-61 GHz with 80 MHz reference:
// N = 725-765, k = fractional part

// This requires extracting PLL calculation from:
// radar_sdk/external/lib_avian/src/Driver/_configuration.c
// Functions: calculate_pll_params(), write_pll_config()
```

#### 6. FIFO Reading

After frame acquisition starts:
```c
// Wait for frame ready (IRQ or poll STAT1)
while (!(avian_read_reg(BGT60TRxxC_REG_STAT1) & FRAME_READY_BIT));

// Read FIFO burst (4096 samples for 64x32x2 I/Q)
uint8_t fifo_cmd[2] = {0x80, 0x00};  // FIFO read command
spi_cs_low();
spi_transfer_buffer(fifo_cmd, NULL, 2);
for (int i = 0; i < num_samples; i++) {
    frame->samples[i] = spi_read_word();  // 16-bit read
}
spi_cs_high();
```

## Implementation Options

### Option A: Port Full Driver (3-5 days)
**Effort**: High
**Reliability**: High
**Steps**:
1. Copy entire Avian driver from reference code
2. Port dependencies (types, error handling)
3. Adapt for bare-metal (remove OS calls)
4. Test and debug

**Files to copy**: ~10 files, ~5000 lines of code

### Option B: Use Infineon SDK Directly (1-2 days)
**Effort**: Medium
**Reliability**: High
**Steps**:
1. Link against precompiled SDK library
2. Implement platform callbacks (SPI, GPIO, delay)
3. Call SDK configuration functions

**Problem**: SDK libraries are x86-64, not ARM. Need ARM build.

### Option C: Minimal Register Programming (2-3 days)
**Effort**: Medium
**Reliability**: Medium
**Steps**:
1. Extract exact register values from working Python example
2. Capture SPI trace from PC+board setup
3. Replay register sequence in firmware
4. Tune for presence detection use case

**Tools needed**: Logic analyzer or SPI sniffer

## Recommended Path Forward

**For this project**, I recommend **Option A** (port full driver):

1. Start with these specific files:
   ```
   radar_sdk/external/lib_avian/src/Driver/
   ├── registers_BGT60TRxxC.h  → Copy to drivers/
   ├── Driver.c/h              → Port to avian_radar.c
   └── _configuration.c/h      → Port PLL calc functions
   ```

2. Simplify by removing features not needed:
   - Multiple shape support (use only one chirp shape)
   - TDM MIMO (use single TX/RX)
   - Advanced features (CW mode, SADC, etc.)

3. Focus on single use case:
   - Fixed configuration (58-61 GHz, 64 samples, 32 chirps)
   - Hardcode PLL values if needed
   - Test with presence detection only

## Validation

After implementation:
1. Read back all configured registers
2. Verify chip ID and status registers
3. Check FIFO has expected data count
4. Validate frame timing (should be 200ms)
5. Test with Python SDK side-by-side for comparison

## Time Estimate

- **Reading documentation**: 4-6 hours
- **Extracting code**: 8-12 hours
- **Porting to bare-metal**: 8-12 hours
- **Testing and debugging**: 8-16 hours
- **Total**: 28-46 hours (3.5-6 days)

## Reference Files

All needed information is in:
```
../radar_sdk/external/lib_avian/src/Driver/
../firmware_extracted/Firmware/sources/stratula/library/components/radar/
../radar_sdk/examples/py/FMCW/presence_detection.py
```

The register header file has complete bit field definitions.
The configuration functions in the SDK handle all the complex math.
