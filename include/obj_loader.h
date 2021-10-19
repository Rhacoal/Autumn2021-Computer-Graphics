#ifndef ASSIGNMENT_OBJ_LOADER_H
#define ASSIGNMENT_OBJ_LOADER_H

#include <cg_common.h>
#include <cg_fwd.h>

#include <filesystem>

namespace cg {

Mesh *read(const std::filesystem::path &path);

}

#endif //ASSIGNMENT_OBJ_LOADER_H
