#include <object3d.h>
#include <renderer.h>
#include <material.h>
#include <geometry.h>
#include <scene.h>
#include <lighting.h>
#include <camera.h>

#include <optional>
#include <algorithm>

void cg::Renderer::render(Scene &sc, Camera &cam) {
    ProgramArguments pargs;
    draw_calls.clear();

    // collect lighting
    sc.traverse([&](Object3D &obj) {
        if (obj.isLight()) {
            pargs.lights.push_back(obj.isLight());
        }
    });

    sc.traverse([&](Object3D &obj) {
        obj.render(*this, sc, cam);
    });

    // arranges draw order
    std::sort(begin(draw_calls), end(draw_calls), [](const auto &l, const auto &r) {
        const auto&[l_mat, l_geo, l_obj] = l;
        const auto&[r_mat, r_geo, r_obj] = r;
        // draw same materials together
        return l_mat < r_mat;
//        // draw transparent objects last
//        if (l_mat->isTransparent() != r_mat->isTransparent()) {
//            return r_mat->isTransparent();
//        }
    });

    // render objects in order
    std::optional<Material *> current_material;
    GLuint sp = 0;
    for (auto[mat, geo, obj] : draw_calls) {
        if (current_material != mat) {
            // switch material
            sp = mat->useShaderProgram(sc, cam, pargs);
            current_material = mat;
        }

        mat->updateUniforms(obj, cam);

        if (!sp) { continue; }
        geo->bindVAO(sp);

        if (geo->hasIndices()) {
            check_err(glDrawElements(GL_TRIANGLES, geo->elementCount(), GL_UNSIGNED_INT, nullptr));
        } else {
            check_err(glDrawArrays(GL_TRIANGLES, 0, geo->elementCount()));
        }
    }
    glUseProgram(0);
}
