#include "lib/shaders/rt_structure.h"

__kernel void render_kernel(__global float3 *output, int width, int height) {
    const int work_item_id = get_global_id(0);
    int x = work_item_id % width;
    int y = work_item_id / width;
    float fx = (float) x / (float) width;
    float fy = (float) y / (float) height;
    output[work_item_id] =
#ifdef __cplusplus
        float3{fx, fy, 0};
#else
        (float3)(fx, fy, 0);
#endif
}
