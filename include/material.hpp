#ifndef ASSIGNMENT_MATERIAL_HPP
#define ASSIGNMENT_MATERIAL_HPP

#include <string>

namespace cgpa {

struct texture {

};

struct shader {
    std::string header;
    std::string uniforms;
    std::string main;
};

template<typename M>
struct material_traits {
    typedef M material_type;
};

struct phong_material {
    texture diffuse;
    glm::vec4 color;
    glm::float32 shininess;
};

/**
 * Material of an object, including color, textures, shaders and so on.
 */
class material {
public:

};
}

#endif //ASSIGNMENT_MATERIAL_HPP
