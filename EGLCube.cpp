#include "Module.h"

#include "EGLToolbox.h"
#include "IRender.h"
#include "Tracing.h"

#include <compositor/Client.h>

#ifndef GL_ES_VERSION_2_0
#include <GLES2/gl2.h>
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>

#include <esUtil.h>

namespace WPEFramework {
namespace Graphics {
    static constexpr EGLint RedBufferSize = 8;
    static constexpr EGLint GreenBufferSize = 8;
    static constexpr EGLint BlueBufferSize = 8;
    static constexpr EGLint AlphaBufferSize = 8;
    static constexpr EGLint DepthBufferSize = 0;

    constexpr EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    constexpr EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, RedBufferSize,
        EGL_GREEN_SIZE, GreenBufferSize,
        EGL_BLUE_SIZE, BlueBufferSize,
        EGL_ALPHA_SIZE, AlphaBufferSize,
        EGL_BUFFER_SIZE, RedBufferSize + GreenBufferSize + BlueBufferSize + AlphaBufferSize,
        /* EGL_DEPTH_SIZE, (majorVersion == 2) ? 16 : 0, */
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SAMPLES, 0,
        EGL_NONE
    };

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

    class EGLCube : public IRender {
    public:
        EGLCube(const EGLCube&) = delete;
        EGLCube& operator=(const EGLCube&) = delete;

        EGLCube()
            : _display(nullptr)
            , _surface(nullptr)
            , _eglSurface(EGL_NO_SURFACE)
            , _eglContext(EGL_NO_CONTEXT)
            , _eglDisplay(EGL_NO_DISPLAY)
            , _aspect(0)
            , _program(GL_FALSE)
            , _modelviewmatrix(0)
            , _modelviewprojectionmatrix(0)
            , _normalmatrix(0)
            , _vbo(0)
            , _positionsoffset(0)
            , _colorsoffset(sizeof(vVertices))
            , _normalsoffset(sizeof(vVertices) + sizeof(vColors))
        {
        }

        virtual ~EGLCube()
        {
            eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            eglDestroyContext(_eglDisplay, _eglContext);
            _eglContext = EGL_NO_CONTEXT;

            eglDestroySurface(_eglDisplay, _eglSurface);
            _eglSurface = EGL_NO_SURFACE;

            if (_surface != nullptr) {
                _surface->Release();
            }

            if (_display != nullptr) {
                _display->Release();
            }
        }

        uint32_t Frames() const override
        {
            return _frameCount;
        }

        bool Setup(const string& tname, const uint32_t width, const uint32_t height) override
        {
            EGLint majorVersion(0);
            EGLint minorVersion(0);
            EGLint eglResult(0);
            EGLint numConfigs(0);

            std::stringstream strm;
            strm << tname << "-" << time(NULL);

            string name(strm.str());

            // Init CompositorClient
            TRACE(Trace::Information, ("Setup %s width=%d height=%d", name.c_str(), width, height))

            _display = Compositor::IDisplay::Instance(name);
            ASSERT(_display != nullptr);

            _surface = _display->Create(name, width, height);
            ASSERT(_surface != nullptr);

            // Init EGL
            _eglDisplay = eglGetDisplay(_display->Native());
            ASSERT(_eglDisplay != EGL_NO_DISPLAY);

            TRACE(Trace::Information, ("EGL Display %p", _eglDisplay));

            eglResult = eglInitialize(_eglDisplay, &majorVersion, &minorVersion);
            ASSERT(eglResult == EGL_TRUE);

            TRACE(Trace::Information, ("Initialized EGL v%d.%d", majorVersion, minorVersion));

            // eglResult = eglBindAPI(EGL_OPENGL_ES_API);
            // ASSERT(eglResult == EGL_TRUE);

            EGLConfig eglConfig;

            eglResult = eglChooseConfig(_eglDisplay, config_attribs, &eglConfig, 1, &numConfigs);
            ASSERT(eglResult == EGL_TRUE);

            TRACE(Trace::Information, ("Choosen config: %s", EGL::ConfigInfoLog(_eglDisplay, eglConfig).c_str()));

            // std::vector<EGLConfig> configs = EGL::MatchConfigs(_eglDisplay, );
            // ASSERT(configs.size() != 0);

            _eglContext = eglCreateContext(_eglDisplay, eglConfig, EGL_NO_CONTEXT, context_attribs);
            ASSERT(_eglContext != EGL_NO_CONTEXT);

            _eglSurface = eglCreateWindowSurface(_eglDisplay, eglConfig, _surface->Native(), nullptr);

            EGLint s_width, s_heigth;

            eglQuerySurface(_eglDisplay, _eglSurface, EGL_WIDTH, &s_width);
            eglQuerySurface(_eglDisplay, _eglSurface, EGL_HEIGHT, &s_heigth);

            TRACE(Trace::Information, ("EGL surface dimension: %dx%d", s_width, s_heigth));

            // eglResult = eglSwapInterval(_eglDisplay, EGL_TRUE); // enable vsync
            // ASSERT(eglResult == EGL_TRUE);

            if (eglMakeCurrent(_eglDisplay, _eglSurface, _eglSurface, _eglContext) != EGL_TRUE) {
                TRACE(Trace::Error, ("Unable to make EGL context current 0x%x", eglGetError()));
                return false;
            }

            // Init Cube
            EGL::ShaderSourceList shaders = {
                { GL_VERTEX_SHADER, vertex_shader_source },
                { GL_FRAGMENT_SHADER, fragment_shader_source }
            };

            _program = EGL::CreateProgram(shaders);
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

                glViewport(0, 0, width, height);
                glEnable(GL_CULL_FACE);

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

                // /* clear the color buffer */
                // glClearColor(0.5, 0.5, 0.5, 1.0);
                // glClear(GL_COLOR_BUFFER_BIT);

                TRACE(Trace::Information, (_T("Setup done.")));
            } else {
                TRACE(Trace::Error, (_T("Failed to link program.")));
            }

            return (eglResult == GL_TRUE);
        }

        void Draw()
        {
            ESMatrix modelview;

            /* clear the color buffer */
            glClearColor(0.5, 0.5, 0.5, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);

            esMatrixLoadIdentity(&modelview);
            esTranslate(&modelview, 0.0f, 0.0f, -8.0f);
            esRotate(&modelview, 45.0f + (0.25f * _frameCount), 1.0f, 0.0f, 0.0f);
            esRotate(&modelview, 45.0f - (0.5f * _frameCount), 0.0f, 1.0f, 0.0f);
            esRotate(&modelview, 10.0f + (0.15f * _frameCount), 0.0f, 0.0f, 1.0f);

            ESMatrix projection;
            esMatrixLoadIdentity(&projection);
            esFrustum(&projection, -2.8f, +2.8f, -2.8f * _aspect, +2.8f * _aspect, 6.0f, 10.0f);

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
        }

        void Present() override
        {
            ASSERT(_eglDisplay != EGL_NO_DISPLAY);
            ASSERT(_eglSurface != EGL_NO_SURFACE);
            ASSERT(_eglContext != EGL_NO_CONTEXT);

            // Make the context current
            if (eglMakeCurrent(_eglDisplay, _eglSurface, _eglSurface, _eglContext) == EGL_TRUE) {
                Draw();

                if (eglSwapBuffers(_eglDisplay, _eglSurface) == TRUE) {
                    ++_frameCount;
                } else {
                    TRACE(Trace::Error, ("eglSwapBuffers failed error=%s", EGL::ErrorString(eglGetError())));
                }

            } else {
                TRACE(Trace::Error, ("Unable to make EGL context current error=%s", EGL::ErrorString(eglGetError())));
            }
        }

    private:
        Compositor::IDisplay* _display;
        Compositor::IDisplay::ISurface* _surface;

        EGLSurface _eglSurface;
        EGLContext _eglContext;
        EGLDisplay _eglDisplay;

        GLfloat _aspect;
        GLuint _program;
        GLint _modelviewmatrix;
        GLint _modelviewprojectionmatrix;
        GLint _normalmatrix;
        GLuint _vbo;
        GLuint _positionsoffset;
        GLuint _colorsoffset;
        GLuint _normalsoffset;

        uint32_t _frameCount;
    }; // class EGLCube

    IRender* IRender::Instance()
    {
        static EGLCube _instance;
        return &_instance;
    }
} // namespace Graphics
} // namespace WPEFramework