/*
 * Wave Detection - Pure C Implementation
 * No external dependencies (no TFLite Micro needed)
 *
 * Model: 3-layer dense network trained on radar energy data
 * Input: 16 energy values (normalized 0-1)
 * Output: 0 = no_presence, 1 = waving
 */

#ifndef WAVE_DETECTOR_H
#define WAVE_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>

#define WAVE_WINDOW_SIZE 16
#define WAVE_NUM_CLASSES 2

/* Normalization constants (from training data) */
#define WAVE_NORM_MIN  261.28f
#define WAVE_NORM_MAX  2864.34f
#define WAVE_NORM_RANGE (WAVE_NORM_MAX - WAVE_NORM_MIN)

/* Normalize raw energy value to 0-1 range */
#define WAVE_NORMALIZE(val) (((val) - WAVE_NORM_MIN) / WAVE_NORM_RANGE)

typedef enum {
    WAVE_CLASS_NO_PRESENCE = 0,
    WAVE_CLASS_WAVING = 1
} wave_class_t;

typedef struct {
    wave_class_t predicted_class;
    float confidence;
    float scores[WAVE_NUM_CLASSES];
    bool valid;
} wave_result_t;

/*
 * Run wave detection inference
 * Input: array of 16 normalized energy values (0-1 range)
 * Output: result with predicted class and confidence
 * Returns: true on success
 */
bool wave_detect(const float* energy_window, wave_result_t* result);

/*
 * Get class name
 */
const char* wave_get_class_name(wave_class_t class_id);

#endif /* WAVE_DETECTOR_H */
