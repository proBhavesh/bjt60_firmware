/*
 * Host-side test of presence detection algorithm
 * Compile and run on PC to test without hardware
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#define RADAR_NUM_SAMPLES 64
#define RADAR_NUM_CHIRPS 32
#define RADAR_FRAME_SIZE (RADAR_NUM_SAMPLES * RADAR_NUM_CHIRPS)

typedef struct {
    int16_t samples[RADAR_FRAME_SIZE];
    bool valid;
} radar_frame_t;

/* Include the algorithm (simplified for PC testing) */
#define DETECT_START_SAMPLE (RADAR_NUM_SAMPLES / 8)
#define DETECT_END_SAMPLE (RADAR_NUM_SAMPLES / 2)
#define THRESHOLD_PRESENCE 0.0007f
#define ALPHA_SLOW 0.001f
#define ALPHA_MED 0.05f
#define ALPHA_FAST 0.6f

typedef struct {
    float slow_avg[RADAR_NUM_SAMPLES];
    float fast_avg[RADAR_NUM_SAMPLES];
    bool first_run;
    bool presence_detected;
} presence_ctx_t;

void presence_init(presence_ctx_t *ctx) {
    memset(ctx, 0, sizeof(presence_ctx_t));
    ctx->first_run = true;
}

bool presence_detect_simple(presence_ctx_t *ctx, const radar_frame_t *frame) {
    if (!frame || !frame->valid) return false;

    float energy[RADAR_NUM_SAMPLES];

    /* Calculate energy per sample */
    for (int s = 0; s < RADAR_NUM_SAMPLES; s++) {
        float sum = 0.0f;
        for (int c = 0; c < RADAR_NUM_CHIRPS; c++) {
            int16_t sample = frame->samples[c * RADAR_NUM_SAMPLES + s];
            float val = (float)sample / 32768.0f;
            sum += val * val;
        }
        energy[s] = sum / RADAR_NUM_CHIRPS;
    }

    /* Initialize on first run */
    if (ctx->first_run) {
        for (int i = 0; i < RADAR_NUM_SAMPLES; i++) {
            ctx->slow_avg[i] = energy[i];
            ctx->fast_avg[i] = energy[i];
        }
        ctx->first_run = false;
        return false;
    }

    /* Update averages */
    float alpha_slow_used = ctx->presence_detected ? ALPHA_SLOW : ALPHA_MED;
    for (int i = 0; i < RADAR_NUM_SAMPLES; i++) {
        ctx->slow_avg[i] = ctx->slow_avg[i] * (1.0f - alpha_slow_used) + energy[i] * alpha_slow_used;
        ctx->fast_avg[i] = ctx->fast_avg[i] * (1.0f - ALPHA_FAST) + energy[i] * ALPHA_FAST;
    }

    /* Find max difference */
    float max_diff = 0.0f;
    for (int i = DETECT_START_SAMPLE; i < DETECT_END_SAMPLE; i++) {
        float diff = ctx->fast_avg[i] - ctx->slow_avg[i];
        if (diff > max_diff) max_diff = diff;
    }

    ctx->presence_detected = (max_diff > THRESHOLD_PRESENCE);
    return ctx->presence_detected;
}

/* Generate synthetic radar frame */
void generate_test_frame(radar_frame_t *frame, bool add_target) {
    frame->valid = true;

    for (int i = 0; i < RADAR_FRAME_SIZE; i++) {
        /* Background noise */
        frame->samples[i] = (rand() % 200) - 100;

        /* Add target at range bin 20 if requested */
        if (add_target && (i % RADAR_NUM_SAMPLES) == 20) {
            frame->samples[i] += (rand() % 1000) + 500;
        }
    }
}

int main() {
    printf("Testing Presence Detection Algorithm\n");
    printf("=====================================\n\n");

    presence_ctx_t ctx;
    presence_init(&ctx);

    radar_frame_t frame;

    printf("Testing 100 frames...\n\n");

    int detections = 0;

    for (int f = 0; f < 100; f++) {
        /* Simulate target appearing at frame 30-70 */
        bool target_present = (f >= 30 && f < 70);

        generate_test_frame(&frame, target_present);
        bool detected = presence_detect_simple(&ctx, &frame);

        if (detected) detections++;

        if (f % 10 == 0 || detected != target_present) {
            printf("Frame %3d: Target=%s Detected=%s %s\n",
                   f,
                   target_present ? "YES" : "NO ",
                   detected ? "YES" : "NO ",
                   (detected == target_present) ? "✓" : "✗");
        }
    }

    printf("\n");
    printf("Results:\n");
    printf("--------\n");
    printf("Total detections: %d/100 frames\n", detections);
    printf("Expected: ~40 frames (30-70)\n");
    printf("\n");

    if (detections >= 30 && detections <= 50) {
        printf("✓ Algorithm working correctly!\n");
        return 0;
    } else {
        printf("✗ Algorithm may need tuning\n");
        return 1;
    }
}
