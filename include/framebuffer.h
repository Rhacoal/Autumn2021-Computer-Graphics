#ifndef ASSIGNMENT_FRAMEBUFFER_H
#define ASSIGNMENT_FRAMEBUFFER_H

#include <cg_common.g>
#include <cg_fwd.h>

namespace cg {
class FrameBufferImpl {
    GLuint vao{}, vbo{}, ebo{};
    GLuint fbo{}, rbo{};
    Texture colorBuffer;
public:
    FrameBufferImpl(int width, int height, GLint internalFormat);

    template<typename DataType>
    void setData(int width, int height, const DataType *data, int components) {
        glGenFramebuffers(1, &fbo);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        // create a color attachment texture
        colorBuffer.generate(width, height, GL_RGB32F);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer.tex(), 0);
        // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        // use a single renderbuffer object for both a depth AND stencil buffer.
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fputs("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n", stderr);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};
}

#endif //ASSIGNMENT_FRAMEBUFFER_H
