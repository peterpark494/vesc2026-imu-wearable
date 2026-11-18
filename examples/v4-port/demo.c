#include "forest_runtime.h"

#include <stdio.h>

int main(void)
{
    static const int32_t offsets[] = {0, 3, 6};
    static const int32_t left[] = {1, -1, -1, 4, -1, -1};
    static const int32_t right[] = {2, -1, -1, 5, -1, -1};
    static const int16_t features[] = {0, -1, -1, 0, -1, -1};
    static const float thresholds[] = {0.5f, 0, 0, 1.5f, 0, 0};
    static const float leaves[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};
    const forest_model_t model = {2, 2, 6, offsets, left, right, features, thresholds, leaves};
    const float inputs[] = {0.5f, 1.0f, 2.0f};
    const float expected[] = {1.0f, 0.5f, 0.0f};
    float probabilities[2];

    for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; ++i) {
        if (!forest_predict_proba(&model, &inputs[i], 1, probabilities, 2) ||
            probabilities[0] != expected[i] || probabilities[1] != 1.0f - expected[i]) {
            fputs("Unexpected inference result\n", stderr);
            return 1;
        }
        printf("feature=%.1f class probabilities=[%.2f, %.2f]\n",
               inputs[i], probabilities[0], probabilities[1]);
    }
    if (forest_predict_proba(&model, inputs, 0, probabilities, 2)) {
        fputs("Accepted a missing feature\n", stderr);
        return 1;
    }
    return 0;
}
