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
    GLuint LoadShader(GLenum type, const char* shaderSrc)
    {
        GLuint shader;
        GLint compiled;

        // Create the shader object
        shader = glCreateShader(type);

        if (shader == 0) {
            return 0;
        }

        // Load the shader source
        glShaderSource(shader, 1, &shaderSrc, NULL);

        // Compile the shader
        glCompileShader(shader);

        // Check the compile status
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

        if (!compiled) {
            TRACE(Trace::Error, ("Error compiling shader:\n%s", EGL::ShaderInfoLog(shader).c_str()));
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

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

    class EGLTriangle : public IRender {
    public:
        EGLTriangle(const EGLTriangle&) = delete;
        EGLTriangle& operator=(const EGLTriangle&) = delete;

        EGLTriangle()
            : _display(nullptr)
            , _surface(nullptr)
            , _eglSurface(EGL_NO_SURFACE)
            , _eglContext(EGL_NO_CONTEXT)
            , _eglDisplay(EGL_NO_DISPLAY)
            , _program(GL_FALSE)
            , _width(0)
            , _heigth(0)
            , _frameCount(0)
        {
        }

        virtual ~EGLTriangle()
        {

            eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            eglDestroyContext(_eglDisplay, _eglContext);
            _eglContext = EGL_NO_CONTEXT;

            eglDestroySurface(_eglDisplay, _eglSurface);
            _eglSurface = EGL_NO_SURFACE;

            glDeleteProgram(_program);

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

        void Draw()
        {
            // Set the viewport
            glViewport(0, 0, _width, _heigth);

            // Clear the color buffer
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Use the program object
            glUseProgram(_program);

            // Load the vertex data
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
            glEnableVertexAttribArray(0);

            glDrawArrays(GL_TRIANGLES, 0, 3);
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

        bool Setup(const string& name, const uint32_t width, const uint32_t height) override
        {
            EGLint majorVersion(0);
            EGLint minorVersion(0);
            EGLint eglResult(0);
            EGLint numConfigs(0);

            // ==================================================================================
            // Initialize CompositorClient
            // ==================================================================================
            TRACE(Trace::Information, ("Setup %s width=%d height=%d", name.c_str(), width, height));

            _display = Compositor::IDisplay::Instance(name);
            ASSERT(_display != nullptr);

            _surface = _display->Create(name, width, height);
            ASSERT(_surface != nullptr);

            // ==================================================================================
            // Initialize EGL
            // ==================================================================================
            _eglDisplay = eglGetDisplay(_display->Native());
            ASSERT(_eglDisplay != EGL_NO_DISPLAY);

            TRACE(Trace::Information, ("EGL Display %p", _eglDisplay));

            eglResult = eglInitialize(_eglDisplay, &majorVersion, &minorVersion);
            ASSERT(eglResult == EGL_TRUE);

            eglResult = eglBindAPI(EGL_OPENGL_ES_API);
            ASSERT(eglResult == EGL_TRUE);

            TRACE(Trace::Information, ("Initialized EGL v%d.%d", majorVersion, minorVersion));

            // Choose config
            EGLConfig eglConfig;

            eglResult = eglChooseConfig(_eglDisplay, config_attribs, &eglConfig, 1, &numConfigs);
            ASSERT(eglResult == EGL_TRUE);

            TRACE(Trace::Information, ("Choosen config: %s", EGL::ConfigInfoLog(_eglDisplay, eglConfig).c_str()));

            // Create a surface
            _eglSurface = eglCreateWindowSurface(_eglDisplay, eglConfig, _surface->Native(), nullptr);

            eglQuerySurface(_eglDisplay, _eglSurface, EGL_WIDTH, &_width);
            eglQuerySurface(_eglDisplay, _eglSurface, EGL_HEIGHT, &_heigth);

            TRACE(Trace::Information, ("EGL surface dimension: %dx%d", _width, _heigth));

            // Create a GL context
            _eglContext = eglCreateContext(_eglDisplay, eglConfig, EGL_NO_CONTEXT, context_attribs);
            ASSERT(_eglContext != EGL_NO_CONTEXT);

            // Make the context current
            if (eglMakeCurrent(_eglDisplay, _eglSurface, _eglSurface, _eglContext) != EGL_TRUE) {
                TRACE(Trace::Error, ("Unable to make EGL context current error=%s", EGL::ErrorString(eglGetError())));
                return false;
            }

            // ==================================================================================
            // Initialize Triangle
            // ==================================================================================

            // Load the vertex/fragment shaders
            GLuint vertexShader = LoadShader(GL_VERTEX_SHADER, vertex_shader_source);
            GLuint fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fragment_shader_source);

            // Create the program object
            GLuint programObject = glCreateProgram();

            glAttachShader(programObject, vertexShader);
            glAttachShader(programObject, fragmentShader);

            // Link the program
            glLinkProgram(programObject);

            // Check the link status
            GLint linked(0);
            glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

            if (linked == GL_FALSE) {
                TRACE(Trace::Error, ("Error linking program:\n%s", EGL::ProgramInfoLog(programObject).c_str()));
                glDeleteProgram(programObject);
            } else {
                // Store the program object
                _program = programObject;

                glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
            }

            return (linked != GL_FALSE);
        }

    private:
        Compositor::IDisplay* _display;
        Compositor::IDisplay::ISurface* _surface;

        EGLSurface _eglSurface;
        EGLContext _eglContext;
        EGLDisplay _eglDisplay;

        GLuint _program;

        EGLint _width;
        EGLint _heigth;

        uint32_t _frameCount;
    }; // class EGLTriangle

    IRender* IRender::Instance()
    {
        static EGLTriangle _instance;
        return &_instance;
    }
} // namespace Graphics
} // namespace WPEFramework