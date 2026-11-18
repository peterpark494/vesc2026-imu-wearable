#include "forest_runtime.h"

bool forest_predict_proba(
    const forest_model_t *model,
    const float *features,
    size_t feature_count,
    float *probability,
    size_t probability_count) {
    if (model == NULL || features == NULL || probability == NULL ||
        model->tree_count == 0u || model->class_count == 0u ||
        probability_count < model->class_count) {
        return false;
    }
    for (uint32_t c = 0u; c < model->class_count; ++c) {
        probability[c] = 0.0f;
    }
    for (uint32_t tree = 0u; tree < model->tree_count; ++tree) {
        int32_t node = model->tree_offsets[tree];
        const int32_t end = model->tree_offsets[tree + 1u];
        while (node >= 0 && node < end) {
            const int32_t left = model->children_left[node];
            const int32_t right = model->children_right[node];
            if (left == right) {
                const size_t base = (size_t)node * model->class_count;
                for (uint32_t c = 0u; c < model->class_count; ++c) {
                    probability[c] += model->leaf_probability[base + c];
                }
                break;
            }
            const int16_t feature = model->feature[node];
            if (feature < 0 || (size_t)feature >= feature_count ||
                left < 0 || right < 0 ||
                (uint32_t)left >= model->node_count ||
                (uint32_t)right >= model->node_count) {
                return false;
            }
            node = features[feature] <= model->threshold[node] ? left : right;
        }
        if (node < 0 || node >= end) {
            return false;
        }
    }
    const float scale = 1.0f / (float)model->tree_count;
    for (uint32_t c = 0u; c < model->class_count; ++c) {
        probability[c] *= scale;
    }
    return true;
}
