#include "Module.h"

#include "EGLToolbox.h"

#include "IModel.h"

#include "Tracing.h"

#include <core/core.h>

#ifndef GL_ES_VERSION_2_0
#include <GLES2/gl2.h>
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>
#include <esUtil.h>

namespace WPEFramework {
namespace Graphics {
    constexpr GLfloat vVertices[] = {
        -1.0f, -1.0f, 0.0f, //
        1.0f, -1.0f, 0.0f, //
        -1.0f, 1.0f, 0.0f, //
        1.0f, 1.0f, 0.0f, //
    };

    class EGLShader : public IModel {
    public:
        EGLShader(const EGLShader&) = delete;
        EGLShader& operator=(const EGLShader&) = delete;

        virtual ~EGLShader()
        {
            Destroy();
        }

        bool Construct() override
        {
            if (IsValid() == false) {
                _program = EGL::CreateProgram(_vertexShaderSource, _fragmentShaderSource);

                if (glIsProgram(_program)) {
                    glBindAttribLocation(_program, 0, "vPosition");
                    glBindAttribLocation(_program, 1, "vOpacity");

                    if (EGL::LinkProgram(_program) == GL_TRUE) {
                        glUseProgram(_program);

                        _uTime = glGetUniformLocation(_program, "u_time");
                        _uResolution = glGetUniformLocation(_program, "u_resolution");
                        _uOpacity = glGetUniformLocation(_program, "u_opacity");

                        glUniform3f(_uResolution, _width, _height, 0);
                        glUniform1f(_uOpacity, _opacity);

                        glGenBuffers(1, &_vbo);
                        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
                        glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), 0, GL_STATIC_DRAW);
                        glBufferSubData(GL_ARRAY_BUFFER, _inPosition, sizeof(vVertices), &vVertices[0]);

                        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(intptr_t)_inPosition);
                        glEnableVertexAttribArray(0);

                        TRACE(Trace::Information, (_T("Setup done.")));
                    } else {
                        TRACE(Trace::Error, ("Error linking program:\n%s", EGL::ProgramInfoLog(_program).c_str()));
                        glDeleteProgram(_program);
                        _program = GL_FALSE;
                    }
                }
            }

            TRACE(Trace::EGL, ("Shader Constructed: %s", IsValid() ? "OK" : "FAILED"));

            return (IsValid() == true);
        }

        bool Destroy() override
        {
            if (IsValid() == true) {
                EGL::DeleteProgram(_program);
                _program = EGL_FALSE;
            }

            return (IsValid() == false);
        }

        void Process() override
        {
            if (IsValid() == true) {

                // fprintf(stdout, "%s:%d [%s] frameNumber=%ld\n", __FILE__, __LINE__, __FUNCTION__, _frameNumber);fflush(stdout);

                glViewport(0, 0, _width, _height);
                glEnable(GL_CULL_FACE);

                /* clear the color buffer */
                glClearColor(0.5, 0.5, 0.5, 1.0);
                glClear(GL_COLOR_BUFFER_BIT);

                glUseProgram(_program);

                // float now = float(_frameNumber / 60.0f);
                float now = (Core::Time::Now().Ticks() - _start) / float(Core::Time::TicksPerMillisecond) / float(Core::Time::MilliSecondsPerSecond);

                glUniform1f(_uTime, now);
                glUniform1f(_uOpacity, _opacity);
                glUniform3f(_uResolution, _width, _height, 0);

                glBindBuffer(GL_ARRAY_BUFFER, _vbo);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(intptr_t)_inPosition);
                glEnableVertexAttribArray(0);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                glDisableVertexAttribArray(0);

                glDisable(GL_CULL_FACE);

                ++_frameNumber;
            }
        }

        void Position(const DimensionType& /*dimension*/) override
        {
        }

        void Size(const SizeType& surfaceSize) override
        {
            _height = surfaceSize.Height;
            _width = surfaceSize.Width;
        }

        void Opacity(const uint8_t opacity) override
        {
            _opacity = opacity;
        }

        bool IsValid() const override
        {
            return (_program != GL_FALSE);
        }

    public:
        EGLShader(const ModelConfig& config)
            : _frameNumber(0)
            , _start(Core::Time::Now().Ticks())
            , _width(0)
            , _height(0)
            , _opacity(255)
            , _vertexShaderSource()
            , _fragmentShaderSource()
            , _program(GL_FALSE)
            , _vbo(0)
            , _inPosition(0)
            , _uTime(0)
            , _uResolution(0)
            , _uOpacity(0)
        {
            _vertexShaderSource = config.FragmentShaderSource.Value();
            _fragmentShaderSource = config.FragmentShaderSource.Value();

            if ((config.VertexShaderFile.IsSet() == true) && (config.VertexShaderFile.Value().empty() == false)) {
                Core::File vertex(config.VertexShaderFile.Value());

                if ((vertex.Exists() == true) && (vertex.Size() > 0)) {
                    _vertexShaderSource.clear();
                    _vertexShaderSource.resize(vertex.Size());

                    vertex.Open();
                    vertex.Position(false, 0);

                    vertex.Read(reinterpret_cast<uint8_t*>(&_vertexShaderSource[0]), _vertexShaderSource.size());

                    vertex.Close();
                } else {
                    TRACE(Trace::Error, (("Failed to open vertex source file %s size(%" PRIu64 ")"), vertex.FileName().c_str(), vertex.Size()));
                }
            }

            if ((config.FragmentShaderFile.IsSet() == true) && (config.FragmentShaderFile.Value().empty() == false)) {
                Core::File fragment(config.FragmentShaderFile.Value());

                if ((fragment.Exists() == true) && (fragment.Size() > 0)) {
                    _fragmentShaderSource.clear();
                    _fragmentShaderSource.resize(fragment.Size());

                    fragment.Open();
                    fragment.Position(false, 0);

                    fragment.Read(reinterpret_cast<uint8_t*>(&_fragmentShaderSource[0]), _fragmentShaderSource.size());

                    fragment.Close();
                } else {
                    TRACE(Trace::Error, (("Failed to open fragment source file %s size(%" PRIu64 ")"), fragment.FileName().c_str(), fragment.Size()));
                }
            }

            if (config.Width.IsSet() == true) {
                _width = config.Width.Value();
            }

            if (config.Height.IsSet() == true) {
                _height = config.Height.Value();
            }

            TRACE(Trace::Information, ("Created EGL Model %p %dhx%dw", this, _height, _width));

            // TRACE(Trace::EGL, ("Vertex shader:\n====START====================\n%s\n====END========================", _vertexShaderSource.c_str()));
            // TRACE(Trace::EGL, ("Fragment shader:\n====START====================\n%s\n==END==========================", _fragmentShaderSource.c_str()));
        }

    private:
        uint32_t _frameNumber;
        const uint64_t _start;

        uint16_t _width; // in pixels
        uint16_t _height; // in pixels
        uint8_t _opacity; // in 0-255;
        string _vertexShaderSource;
        string _fragmentShaderSource;

        GLuint _program;
        GLuint _vbo;

        // vertex variables
        GLuint _inPosition;

        // fragment variables
        GLint _uTime; // running time in seconds
        GLint _uResolution;
        GLint _uOpacity;
    }; // class EGLShader

    Core::ProxyType<IModel> IModel::Create(const ModelConfig& config)
    {
        Core::ProxyType<EGLShader> proxy = Core::ProxyType<EGLShader>::Create(config);

        return Core::ProxyType<IModel>(proxy);
    }
} // namespace Graphics
} // namespace WPEFramework