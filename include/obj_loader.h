#ifndef ASSIGNMENT_OBJ_LOADER_H
#define ASSIGNMENT_OBJ_LOADER_H

#include <cg_common.h>
#include <cg_fwd.h>

#include <filesystem>

namespace cg {

Object3D *loadObj(const char* path);

}

#endif //ASSIGNMENT_OBJ_LOADER_H
