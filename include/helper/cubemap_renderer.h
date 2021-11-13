#ifndef ASSIGNMENT_CUBEMAP_RENDERER_H
#define ASSIGNMENT_CUBEMAP_RENDERER_H

#include <cg_common.h>
#include <cg_fwd.h>

namespace cg {
void renderToCubemap(Scene &scene, Camera &camera, CubeTexture &targetTexture, Object3D *skipObject);
}

#endif //ASSIGNMENT_CUBEMAP_RENDERER_H
