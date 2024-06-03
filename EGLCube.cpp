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

    constexpr GLfloat vVertices[] = {
        // front
        -1.0f, -1.0f, +1.0f, // front
        +1.0f, -1.0f, +1.0f, // front
        -1.0f, +1.0f, +1.0f, // front
        +1.0f, +1.0f, +1.0f, // front
        // back
        +1.0f, -1.0f, -1.0f, // back
        -1.0f, -1.0f, -1.0f, // back
        +1.0f, +1.0f, -1.0f, // back
        -1.0f, +1.0f, -1.0f, // back
        // right
        +1.0f, -1.0f, +1.0f, // right
        +1.0f, -1.0f, -1.0f, // right
        +1.0f, +1.0f, +1.0f, // right
        +1.0f, +1.0f, -1.0f, // right
        // left
        -1.0f, -1.0f, -1.0f, // left
        -1.0f, -1.0f, +1.0f, // left
        -1.0f, +1.0f, -1.0f, // left
        -1.0f, +1.0f, +1.0f, // left
        // top
        -1.0f, +1.0f, +1.0f, // top
        +1.0f, +1.0f, +1.0f, // top
        -1.0f, +1.0f, -1.0f, // top
        +1.0f, +1.0f, -1.0f, // top
        // bottom
        -1.0f, -1.0f, -1.0f, // bottom
        +1.0f, -1.0f, -1.0f, // bottom
        -1.0f, -1.0f, +1.0f, // bottom
        +1.0f, -1.0f, +1.0f, // bottom
    };

    constexpr GLfloat vColors[] = {
        // front
        0.0f, 0.0f, 1.0f, // blue
        1.0f, 0.0f, 1.0f, // magenta
        0.0f, 1.0f, 1.0f, // cyan
        1.0f, 1.0f, 1.0f, // white
        // back
        1.0f, 0.0f, 0.0f, // red
        0.0f, 0.0f, 0.0f, // black
        1.0f, 1.0f, 0.0f, // yellow
        0.0f, 1.0f, 0.0f, // green
        // right
        1.0f, 0.0f, 1.0f, // magenta
        1.0f, 0.0f, 0.0f, // red
        1.0f, 1.0f, 1.0f, // white
        1.0f, 1.0f, 0.0f, // yellow
        // left
        0.0f, 0.0f, 0.0f, // black
        0.0f, 0.0f, 1.0f, // blue
        0.0f, 1.0f, 0.0f, // green
        0.0f, 1.0f, 1.0f, // cyan
        // top
        0.0f, 1.0f, 1.0f, // cyan
        1.0f, 1.0f, 1.0f, // white
        0.0f, 1.0f, 0.0f, // green
        1.0f, 1.0f, 0.0f, // yellow
        // bottom
        0.0f, 0.0f, 0.0f, // black
        1.0f, 0.0f, 0.0f, // red
        0.0f, 0.0f, 1.0f, // blue
        1.0f, 0.0f, 1.0f // magenta
    };

    constexpr GLfloat vNormals[] = {
        // front
        +0.0f, +0.0f, +1.0f, // forward
        +0.0f, +0.0f, +1.0f, // forward
        +0.0f, +0.0f, +1.0f, // forward
        +0.0f, +0.0f, +1.0f, // forward
        // back
        +0.0f, +0.0f, -1.0f, // backward
        +0.0f, +0.0f, -1.0f, // backward
        +0.0f, +0.0f, -1.0f, // backward
        +0.0f, +0.0f, -1.0f, // backward
        // right
        +1.0f, +0.0f, +0.0f, // right
        +1.0f, +0.0f, +0.0f, // right
        +1.0f, +0.0f, +0.0f, // right
        +1.0f, +0.0f, +0.0f, // right
        // left
        -1.0f, +0.0f, +0.0f, // left
        -1.0f, +0.0f, +0.0f, // left
        -1.0f, +0.0f, +0.0f, // left
        -1.0f, +0.0f, +0.0f, // left
        // top
        +0.0f, +1.0f, +0.0f, // up
        +0.0f, +1.0f, +0.0f, // up
        +0.0f, +1.0f, +0.0f, // up
        +0.0f, +1.0f, +0.0f, // up
        // bottom
        +0.0f, -1.0f, +0.0f, // down
        +0.0f, -1.0f, +0.0f, // down
        +0.0f, -1.0f, +0.0f, // down
        +0.0f, -1.0f, +0.0f // down
    };

    constexpr char vertex_shader_source[] = "uniform mat4 modelviewMatrix;      \n"
                                            "uniform mat4 modelviewprojectionMatrix;\n"
                                            "uniform mat3 normalMatrix;         \n"
                                            "                                   \n"
                                            "attribute vec4 in_position;        \n"
                                            "attribute vec3 in_normal;          \n"
                                            "attribute vec4 in_color;           \n"
                                            "\n"
                                            "vec4 lightSource = vec4(2.0, 2.0, 20.0, 0.0);\n"
                                            "                                   \n"
                                            "varying vec4 vVaryingColor;        \n"
                                            "                                   \n"
                                            "void main()                        \n"
                                            "{                                  \n"
                                            "    gl_Position = modelviewprojectionMatrix * in_position;\n"
                                            "    vec3 vEyeNormal = normalMatrix * in_normal;\n"
                                            "    vec4 vPosition4 = modelviewMatrix * in_position;\n"
                                            "    vec3 vPosition3 = vPosition4.xyz / vPosition4.w;\n"
                                            "    vec3 vLightDir = normalize(lightSource.xyz - vPosition3);\n"
                                            "    float diff = max(0.0, dot(vEyeNormal, vLightDir));\n"
                                            "    vVaryingColor = vec4(diff * in_color.rgb, 1.0);\n"
                                            "}                                  \n";

    constexpr char fragment_shader_source[] = "precision mediump float;           \n"
                                              "                                   \n"
                                              "varying vec4 vVaryingColor;        \n"
                                              "                                   \n"
                                              "void main()                        \n"
                                              "{                                  \n"
                                              "    gl_FragColor = vVaryingColor;  \n"
                                              "}                                  \n";

    class EGLCube : public IModel {
    public:
        EGLCube(const EGLCube&) = delete;
        EGLCube& operator=(const EGLCube&) = delete;

        EGLCube()
            : _background(rand())
            , _program(GL_FALSE)
            , _modelviewmatrix(0)
            , _modelviewprojectionmatrix(0)
            , _normalmatrix(0)
            , _vbo(0)
            , _positionsoffset(0)
            , _colorsoffset(sizeof(vVertices))
            , _normalsoffset(sizeof(vVertices) + sizeof(vColors))
            , _width(0)
            , _height(0)
        {
        }

        virtual ~EGLCube()
        {
            if (_program != EGL_FALSE) {
                glDeleteProgram(_program);
            }
        }

        bool Construct()
        {
            EGLint eglResult(0);

            _program = EGL::CreateProgram(vertex_shader_source, fragment_shader_source);
            ASSERT(_program != GL_FALSE);

            glBindAttribLocation(_program, 0, "in_position");
            glBindAttribLocation(_program, 1, "in_normal");
            glBindAttribLocation(_program, 2, "in_color");

            eglResult = EGL::LinkProgram(_program);

            if (eglResult == GL_TRUE) {

                glUseProgram(_program);

                _modelviewmatrix = glGetUniformLocation(_program, "modelviewMatrix");
                _modelviewprojectionmatrix = glGetUniformLocation(_program, "modelviewprojectionMatrix");
                _normalmatrix = glGetUniformLocation(_program, "normalMatrix");

                glGenBuffers(1, &_vbo);
                glBindBuffer(GL_ARRAY_BUFFER, _vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices) + sizeof(vColors) + sizeof(vNormals), 0, GL_STATIC_DRAW);

                glBufferSubData(GL_ARRAY_BUFFER, _positionsoffset, sizeof(vVertices), &vVertices[0]);
                glBufferSubData(GL_ARRAY_BUFFER, _normalsoffset, sizeof(vNormals), &vNormals[0]);
                glBufferSubData(GL_ARRAY_BUFFER, _colorsoffset, sizeof(vColors), &vColors[0]);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(intptr_t)_positionsoffset);
                glEnableVertexAttribArray(0);

                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(intptr_t)_normalsoffset);
                glEnableVertexAttribArray(1);

                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(intptr_t)_colorsoffset);
                glEnableVertexAttribArray(2);

                TRACE(Trace::Information, (_T("Setup done.")));
            } else {
                TRACE(Trace::Error, (_T("Failed to link program.")));
                glDeleteProgram(_program);
            }

            return (eglResult == GL_TRUE);
        }

        void Process() override
        {
            static uint32_t frameNumber;
            int r, g, b;

            r = (((_background & 0x00ff0000) >> 16) + frameNumber) % 512;
            g = (((_background & 0x0000ff00) >> 8) + frameNumber) % 512;
            b = ((_background & 0x000000ff) + frameNumber) % 512;

            if (r >= 256)
                r = 511 - r;
            if (g >= 256)
                g = 511 - g;
            if (b >= 256)
                b = 511 - b;

            glViewport(0, 0, _width, _height);
            glEnable(GL_CULL_FACE);

            /*
             * Different color every frame
             */
            glClearColor(r / 256.0, g / 256.0, b / 256.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);

            /* clear the color buffer */
            // glClearColor(0.5, 0.5, 0.5, 1.0);
            // glClear(GL_COLOR_BUFFER_BIT);

            ESMatrix modelview;

            esMatrixLoadIdentity(&modelview);
            esTranslate(&modelview, 0.0f, 0.0f, -8.0f);
            esRotate(&modelview, 45.0f + (0.25f * frameNumber), 1.0f, 0.0f, 0.0f);
            esRotate(&modelview, 45.0f - (0.5f * frameNumber), 0.0f, 1.0f, 0.0f);
            esRotate(&modelview, 10.0f + (0.15f * frameNumber), 0.0f, 0.0f, 1.0f);

            GLfloat aspect = (GLfloat)(_height) / (GLfloat)(_width);

            ESMatrix projection;
            esMatrixLoadIdentity(&projection);
            esFrustum(&projection, -2.8f, +2.8f, -2.8f * aspect, +2.8f * aspect, 6.0f, 10.0f);

            ESMatrix modelviewprojection;
            esMatrixLoadIdentity(&modelviewprojection);
            esMatrixMultiply(&modelviewprojection, &modelview, &projection);

            float normal[9];
            normal[0] = modelview.m[0][0];
            normal[1] = modelview.m[0][1];
            normal[2] = modelview.m[0][2];
            normal[3] = modelview.m[1][0];
            normal[4] = modelview.m[1][1];
            normal[5] = modelview.m[1][2];
            normal[6] = modelview.m[2][0];
            normal[7] = modelview.m[2][1];
            normal[8] = modelview.m[2][2];

            glUniformMatrix4fv(_modelviewmatrix, 1, GL_FALSE, &modelview.m[0][0]);
            glUniformMatrix4fv(_modelviewprojectionmatrix, 1, GL_FALSE, &modelviewprojection.m[0][0]);
            glUniformMatrix3fv(_normalmatrix, 1, GL_FALSE, normal);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
            glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
            glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
            glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
            glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

            glDisable(GL_CULL_FACE);

            ++frameNumber;
        }

        void Position(const DimensionType& dimension) override
        {
        }

        void Size(const SizeType& surfaceSize) override
        {
            _height = surfaceSize.Height;
            _width = surfaceSize.Width;
        }

        void Opacity(const uint8_t opacity) override
        {
        }

        bool IsValid() const override
        {
            return (_program != GL_FALSE);
        }

    private:
        GLuint _background;
        GLuint _program;
        GLint _modelviewmatrix;
        GLint _modelviewprojectionmatrix;
        GLint _normalmatrix;
        GLuint _vbo;
        GLuint _positionsoffset;
        GLuint _colorsoffset;
        GLuint _normalsoffset;
        GLuint _width;
        GLuint _height;
    }; // class EGLCube

    Core::ProxyType<IModel> IModel::Create(const ModelConfig& config)
    {
        Core::ProxyType<EGLCube> proxy = Core::ProxyType<EGLCube>::Create(config);

        return Core::ProxyType<IModel>(proxy);
    }
} // namespace Graphics
} // namespace Thunder