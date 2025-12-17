/*
 * Wave Detection - Pure C Implementation
 *
 * Neural network: Input(16) -> Dense(8) -> Dense(4) -> Dense(2)
 * Total parameters: 182
 * No external dependencies
 */

#include "wave_detector.h"
#include <math.h>

/* Layer 1: Dense 16 -> 8 (ReLU) */
static const float w1[16][8] = {
    {0.32980254f, -0.02496379f, 0.30209440f, -0.11644694f, 0.04519681f, -0.13354440f, -0.32077715f, -0.36338332f},
    {-0.40206444f, -0.04301091f, -0.22268654f, 0.10808137f, -0.41211712f, -0.15678507f, 0.20804110f, -0.21199650f},
    {0.18801580f, -0.22358067f, -0.36946794f, -0.48220518f, 0.33878481f, 0.04329069f, -0.34362292f, 0.50366127f},
    {0.26452217f, 0.42581150f, 0.22167429f, 0.42777508f, 0.43240607f, -0.08492296f, 0.14961132f, -0.18232718f},
    {0.21364534f, 0.46156669f, -0.40729040f, 0.04020086f, 0.48080018f, 0.41614047f, 0.48359418f, -0.24080200f},
    {-0.31132922f, 0.48520765f, 0.01171832f, 0.35814717f, 0.13328038f, -0.25391904f, -0.40312019f, -0.51732713f},
    {-0.46675530f, 0.37558398f, 0.22086556f, -0.24868231f, 0.19133236f, -0.02355710f, -0.04469455f, -0.00291827f},
    {-0.38197386f, -0.07204188f, 0.05611242f, -0.38166788f, -0.44408664f, 0.35872978f, -0.19372870f, -0.06949453f},
    {0.26647729f, -0.37269884f, 0.23692526f, 0.24892583f, -0.05767173f, -0.39432275f, -0.11378434f, -0.51935881f},
    {-0.13895679f, 0.31915131f, 0.06958958f, 0.07057528f, 0.29120213f, 0.28267917f, 0.00337918f, -0.36695108f},
    {0.05359723f, -0.08583080f, 0.25963172f, 0.04194283f, 0.55628926f, 0.22609018f, 0.43387657f, -0.06110585f},
    {-0.13567379f, -0.22794667f, 0.17715798f, -0.40897074f, 0.45271522f, -0.19161229f, 0.43179619f, 0.46201310f},
    {-0.17622799f, 0.20866704f, -0.09948226f, 0.36645931f, 0.49236283f, 0.31856999f, -0.18077825f, 0.29256123f},
    {0.32659808f, -0.03708697f, 0.45204884f, 0.19596906f, -0.22153094f, 0.51280475f, 0.48965842f, -0.18314792f},
    {-0.14821732f, 0.06111924f, 0.19644912f, -0.09462006f, 0.01200948f, -0.08236312f, 0.35399389f, 0.33500469f},
    {-0.51819670f, -0.12815793f, 0.49369168f, 0.22890970f, -0.18535389f, 0.30364490f, -0.21325979f, 0.29175428f}
};
static const float b1[8] = {0.10328110f, -0.05915626f, -0.04145275f, 0.10210554f, -0.05003232f, -0.03028097f, -0.01067997f, 0.00124598f};

/* Layer 2: Dense 8 -> 4 (ReLU) */
static const float w2[8][4] = {
    {0.58832401f, 0.70374918f, 0.54729235f, 0.01879263f},
    {0.14009967f, -0.13044180f, -0.63124835f, -0.42119914f},
    {0.05174207f, -0.34547144f, -0.10903412f, 0.30865416f},
    {-0.13833724f, 0.63029885f, -0.59256184f, -0.62043798f},
    {-0.73342377f, -0.45763695f, -0.02046448f, -0.35686213f},
    {0.32662791f, -0.40250662f, 0.38903245f, 0.23656723f},
    {-0.63624889f, -0.21843451f, -0.10156664f, 0.49817804f},
    {0.14059293f, -0.03793889f, 0.14701013f, -0.10218775f}
};
static const float b2[4] = {-0.07235415f, 0.10248745f, -0.01646657f, 0.05570351f};

/* Layer 3: Dense 4 -> 2 (Softmax) */
static const float w3[4][2] = {
    {0.85200727f, -0.08993543f},
    {0.89764124f, -0.85179788f},
    {0.28849286f, 0.52609241f},
    {-0.82824796f, -0.11782224f}
};
static const float b3[2] = {-0.00058696f, 0.00058696f};

/* Class names */
static const char* class_names[2] = {"no_presence", "waving"};


/* ReLU activation */
static inline float relu(float x) {
    return x > 0.0f ? x : 0.0f;
}


bool wave_detect(const float* input, wave_result_t* result)
{
    if (!input || !result) {
        return false;
    }

    float layer1[8];
    float layer2[4];
    float layer3[2];

    /* Layer 1: Dense(16->8) + ReLU */
    for (int j = 0; j < 8; j++) {
        float sum = b1[j];
        for (int i = 0; i < 16; i++) {
            sum += input[i] * w1[i][j];
        }
        layer1[j] = relu(sum);
    }

    /* Layer 2: Dense(8->4) + ReLU */
    for (int j = 0; j < 4; j++) {
        float sum = b2[j];
        for (int i = 0; i < 8; i++) {
            sum += layer1[i] * w2[i][j];
        }
        layer2[j] = relu(sum);
    }

    /* Layer 3: Dense(4->2) */
    for (int j = 0; j < 2; j++) {
        float sum = b3[j];
        for (int i = 0; i < 4; i++) {
            sum += layer2[i] * w3[i][j];
        }
        layer3[j] = sum;
    }

    /* Softmax */
    float max_val = layer3[0] > layer3[1] ? layer3[0] : layer3[1];
    float exp0 = expf(layer3[0] - max_val);
    float exp1 = expf(layer3[1] - max_val);
    float sum_exp = exp0 + exp1;

    result->scores[0] = exp0 / sum_exp;
    result->scores[1] = exp1 / sum_exp;

    /* Find prediction */
    if (result->scores[1] > result->scores[0]) {
        result->predicted_class = WAVE_CLASS_WAVING;
        result->confidence = result->scores[1];
    } else {
        result->predicted_class = WAVE_CLASS_NO_PRESENCE;
        result->confidence = result->scores[0];
    }

    result->valid = true;
    return true;
}


const char* wave_get_class_name(wave_class_t class_id)
{
    if (class_id < WAVE_NUM_CLASSES) {
        return class_names[class_id];
    }
    return "unknown";
}
