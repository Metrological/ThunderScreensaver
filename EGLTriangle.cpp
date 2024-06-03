#include "Module.h"

#include "EGLToolbox.h"
#include "IModel.h"

#include "Tracing.h"

#ifndef GL_ES_VERSION_2_0
#include <GLES2/gl2.h>
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>

#include <esUtil.h>

namespace Thunder {
namespace Graphics {
    constexpr char vertex_shader_source[] = "#version 300 es                          \n"
                                            "layout(location = 0) in vec4 vPosition;  \n"
                                            "void main()                              \n"
                                            "{                                        \n"
                                            "   gl_Position = vPosition;              \n"
                                            "}                                        \n";

    constexpr char fragment_shader_source[] = "#version 300 es                              \n"
                                              "precision mediump float;                     \n"
                                              "out vec4 fragColor;                          \n"
                                              "void main()                                  \n"
                                              "{                                            \n"
                                              "   fragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );  \n"
                                              "}                                            \n";

    constexpr GLfloat vVertices[] = { 0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f };

    class EGLTriangle : public IModel {
    public:
        EGLTriangle(const EGLTriangle&) = delete;
        EGLTriangle& operator=(const EGLTriangle&) = delete;

        EGLTriangle()
            : _program(GL_FALSE)
            , _background(rand())
        {
        }

        virtual ~EGLTriangle()
        {
            if (_program != EGL_FALSE) {
                glDeleteProgram(_program);
            }
        }

        void Process() override
        {
            static uint32_t frameNumber;
            int r, g, b;
            // Clear the color buffer

            r = (((_background & 0x00ff0000) >> 16) + frameNumber) % 512;
            g = (((_background & 0x0000ff00) >> 8) + frameNumber) % 512;
            b = ((_background & 0x000000ff) + frameNumber) % 512;

            if (r >= 256)
                r = 511 - r;
            if (g >= 256)
                g = 511 - g;
            if (b >= 256)
                b = 511 - b;

            // Use the program object
            glUseProgram(_program);

            /*
             * Different color every frame
             */
            glClearColor(r / 256.0, g / 256.0, b / 256.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);

            // Load the vertex data
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
            glEnableVertexAttribArray(0);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            ++frameNumber;

            // printf("%s:%d [%s]\n", __FILE__, __LINE__, __FUNCTION__);
        }

        bool Construct() override
        {
            GLuint programObject = EGL::CreateProgram(vertex_shader_source,fragment_shader_source);
            ASSERT(programObject != GL_FALSE);

            EGLint linked = EGL::LinkProgram(programObject);

            if (linked == GL_TRUE) {
                _program = programObject;
                glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
            } else {
                TRACE(Trace::Error, ("Error linking program:\n%s", EGL::ProgramInfoLog(programObject).c_str()));
                glDeleteProgram(programObject);
            }

            return (linked == GL_TRUE);
        }

        void Position(const DimensionType& dimension) override
        {
        }

        void Size(const SizeType& surfaceSize) override
        {
        }

        void Opacity(const uint8_t opacity) override
        {
        }

        bool IsValid() const override
        {
            return (_program != GL_FALSE);
        }

    private:
        GLuint _program;
        GLuint _background;
    }; // class EGLTriangle

    Core::ProxyType<IModel> IModel::Create(const ModelConfig& config)
    {
        Core::ProxyType<EGLTriangle> proxy = Core::ProxyType<EGLTriangle>::Create(config);

        return Core::ProxyType<IModel>(proxy);
    }
} // namespace Graphics
} // namespace Thunder