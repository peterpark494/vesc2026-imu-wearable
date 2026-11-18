#ifndef VESC_FOREST_RUNTIME_H
#define VESC_FOREST_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t tree_count;
    uint32_t class_count;
    uint32_t node_count;
    const int32_t *tree_offsets;
    const int32_t *children_left;
    const int32_t *children_right;
    const int16_t *feature;
    const float *threshold;
    const float *leaf_probability;
} forest_model_t;

bool forest_predict_proba(
    const forest_model_t *model,
    const float *features,
    size_t feature_count,
    float *probability,
    size_t probability_count);

#endif
