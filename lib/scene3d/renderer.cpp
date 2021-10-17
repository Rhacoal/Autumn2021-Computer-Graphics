#include <object3d.h>
#include <renderer.h>
#include <material.h>
#include <geometry.h>
#include <scene.h>
#include <lighting.h>
#include <camera.h>

#include <optional>

void cg::Renderer::render(Scene &sc, Camera &cam) {
    ProgramArguments pargs;

    // collect lighting
    sc.traverse([&](Object3D &obj) {
        obj.updateWorldMatrix();
        if (obj.isLight()) {
            pargs.lights.push_back(obj.isLight());
        }
    });

    sc.traverse([&](Object3D &obj) {
        obj.render(*this, sc, cam);
    });

    // arranges draw order
    sort(begin(draw_calls), end(draw_calls),
         [](const auto &l, const auto &r) {
             auto[l_mat, l_geo] = l;
             auto[r_mat, r_geo] = r;
             // draw same materials together
             if (l_mat == r_mat) {
                 return false;
             }
//             // draw transparent objects last
//             if (l_mat->isTransparent() != r_mat->isTransparent()) {
//                 return r_mat->isTransparent();
//             }
             return l_mat < r_mat;
         }
    );

    // render objects in order
    std::optional<Material *> current_material;
    GLuint sp = 0;
    for (auto[mat, geo] : draw_calls) {
        if (current_material != mat) {
            // switch material
            sp = mat->shaderProgram(sc, cam, pargs);
            if (sp) {
                glUseProgram(sp);
            }
            current_material = mat;
        }
        if (!sp) { continue; }
        geo->bindVAO(sp);

        glDrawElements(GL_TRIANGLES, geo->vertexCount(), GL_UNSIGNED_INT, nullptr);
    }
    glUseProgram(0);
}
