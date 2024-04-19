#pragma once

#include "Module.h"

#include <tracing/tracing.h>

#include "Tracing.h"

#ifndef GL_ES_VERSION_2_0
#include <GLES2/gl2.h>
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

namespace WPEFramework {
namespace EGL {
#define CASE_STR(value) \
    case value:         \
        return #value;
    static inline const char* ErrorString(EGLint error)
    {
        switch (error) {
            CASE_STR(EGL_SUCCESS)
            CASE_STR(EGL_NOT_INITIALIZED)
            CASE_STR(EGL_BAD_ACCESS)
            CASE_STR(EGL_BAD_ALLOC)
            CASE_STR(EGL_BAD_ATTRIBUTE)
            CASE_STR(EGL_BAD_CONTEXT)
            CASE_STR(EGL_BAD_CONFIG)
            CASE_STR(EGL_BAD_CURRENT_SURFACE)
            CASE_STR(EGL_BAD_DISPLAY)
            CASE_STR(EGL_BAD_SURFACE)
            CASE_STR(EGL_BAD_MATCH)
            CASE_STR(EGL_BAD_PARAMETER)
            CASE_STR(EGL_BAD_NATIVE_PIXMAP)
            CASE_STR(EGL_BAD_NATIVE_WINDOW)
            CASE_STR(EGL_CONTEXT_LOST)
        default:
            return "Unknown";
        }
    }
#undef CASE_STR

    static inline string ShaderInfoLog(GLuint handle)
    {

        string log;

        if (glIsShader(handle) == true) {
            GLint length;

            glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &length);

            if (length > 1) {
                log.resize(length);
                glGetShaderInfoLog(handle, length, NULL, &log[0]);
            }
        } else {
            log = ("Handle is a invalid shader handle");
        }

        return log;
    }

    static inline string ProgramInfoLog(GLuint handle)
    {
        string log;
        if (glIsProgram(handle)) {
            GLint length(0);

            glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length);

            if (length > 1) {
                log.resize(length);
                glGetProgramInfoLog(handle, length, NULL, &log[0]);
            }
        } else {
            log = ("Handle is a invalid program handle");
        }

        return log;
    }

    static inline GLuint CompileShader(GLenum type, const char* code)
    {
        GLint status(GL_FALSE);
        GLuint handle = glCreateShader(type);

        glShaderSource(handle, 1, &code, NULL);
        glCompileShader(handle);

        glGetShaderiv(handle, GL_COMPILE_STATUS, &status);

        if (status == GL_FALSE) {
            TRACE_GLOBAL(Trace::Error, ("Shader %d compilation failed: %s", handle, ShaderInfoLog(handle).c_str()));
            glDeleteShader(handle);
            handle = GL_FALSE;
        }

        return handle;
    }

    static inline GLuint CreateProgram(const string& vertexShaderSource, const string& fragmentShaderSource)
    {
        GLuint handle(GL_FALSE);

        GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());

        if ((vs != GL_FALSE) && (fs != GL_FALSE)) {
            handle = glCreateProgram();

            glAttachShader(handle, vs);
            glAttachShader(handle, fs);
        } else {
            TRACE_GLOBAL(Trace::Error, ("Error Creating Program"));
        }

        return handle;
    }

    static inline GLint LinkProgram(GLuint handle)
    {
        ASSERT(glIsProgram(handle));

        GLint status(GL_FALSE);

        glLinkProgram(handle);

        glGetProgramiv(handle, GL_LINK_STATUS, &status);

        if (status == GL_FALSE) {
            TRACE_GLOBAL(Trace::Error, ("Program linking failed: %s", ProgramInfoLog(handle).c_str()));
        }

        return status;
    }

    static inline void DeleteProgram(GLuint handle)
    {
        // ASSERT(glIsProgram(handle));
        if (glIsProgram(handle)) {
            GLint numShaders = 0;
            glGetProgramiv(handle, GL_ATTACHED_SHADERS, &numShaders);

            GLuint* shaderNames = new GLuint[numShaders];
            glGetAttachedShaders(handle, numShaders, NULL, shaderNames);

            for (int i = 0; i < numShaders; ++i) {
                glDetachShader(handle, shaderNames[i]);
                glDeleteShader(shaderNames[i]);
            }

            glGetProgramiv(handle, GL_ATTACHED_SHADERS, &numShaders);

            TRACE_GLOBAL(Trace::EGL, ("Shaders Destroyed: %s", (numShaders == 0) ? "OK" : "FAILED"));

            glDeleteProgram(handle);
        }
    }

    static inline string ConfigInfoLog(EGLDisplay dpy, EGLConfig config)
    {
        std::stringstream stream;

        EGLint id, bufferSize, redSize, greenSize, blueSize, alphaSize, depthSize, stencilSize;

        eglGetConfigAttrib(dpy, config, EGL_CONFIG_ID, &id);
        eglGetConfigAttrib(dpy, config, EGL_BUFFER_SIZE, &bufferSize);
        eglGetConfigAttrib(dpy, config, EGL_RED_SIZE, &redSize);
        eglGetConfigAttrib(dpy, config, EGL_GREEN_SIZE, &greenSize);
        eglGetConfigAttrib(dpy, config, EGL_BLUE_SIZE, &blueSize);
        eglGetConfigAttrib(dpy, config, EGL_ALPHA_SIZE, &alphaSize);
        eglGetConfigAttrib(dpy, config, EGL_DEPTH_SIZE, &depthSize);
        eglGetConfigAttrib(dpy, config, EGL_STENCIL_SIZE, &stencilSize);

        stream << "id=" << id << "  -- buffer=" << bufferSize << "  -- red=" << redSize << "  -- green=" << greenSize << "  -- blue=" << blueSize << "  -- alpha=" << alphaSize << "  -- depth=" << depthSize << "  -- stencil=" << stencilSize;

        return stream.str();
    }

    static inline std::vector<EGLConfig> MatchConfigs(EGLDisplay dpy, const EGLint attrib_list[])
    {
        EGLint count(0);

        if (eglGetConfigs(dpy, nullptr, 0, &count) != EGL_TRUE) {
            TRACE_GLOBAL(Trace::Error, ("Unable to get EGL configs 0x%x", eglGetError()));
        }

        TRACE_GLOBAL(Trace::EGL, ("Got a total of %d EGL configs.", count));

        std::vector<EGLConfig> configs(count, nullptr);

        EGLint matches(0);

        if (eglChooseConfig(dpy, attrib_list, configs.data(), configs.size(), &matches) != EGL_TRUE) {
            TRACE_GLOBAL(Trace::Error, ("No EGL configs with appropriate attributes: 0x%x", eglGetError()));
        }

        TRACE_GLOBAL(Trace::EGL, ("Found %d matching EGL configs", matches));

        configs.erase(std::remove_if(configs.begin(), configs.end(),
                          [](EGLConfig& c) { return (c == nullptr); }),
            configs.end());

        uint8_t i(0);

        for (auto cnf : configs) {
            TRACE_GLOBAL(Trace::EGL, ("%d. %s", i++, ConfigInfoLog(dpy, cnf).c_str()));
        }

        return configs;
    }

    static inline string OpenGLInfo()
    {
        std::stringstream stream;

        stream << std::endl
               << "OpenGL information:" << std::endl
               << "  -- version: " << glGetString(GL_VERSION) << std::endl
               << "  -- shading language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl
               << "  -- vendor: " << glGetString(GL_VENDOR) << std::endl
               << "  -- renderer: " << glGetString(GL_RENDERER) << std::endl
               << "  -- extensions: " << glGetString(GL_EXTENSIONS) << std::endl;

        return stream.str();
    }

    static inline string EGLInfo(EGLDisplay dpy)
    {
        std::stringstream stream;

        stream << std::endl
               << "EGL information:" << std::endl
               << "  -- version: " << eglQueryString(dpy, EGL_VERSION) << std::endl
               << "  -- vendor: " << eglQueryString(dpy, EGL_VENDOR) << std::endl
               << "  -- client extensions: " << eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS) << std::endl
               << "  -- display extensions: " << eglQueryString(dpy, EGL_EXTENSIONS) << std::endl;

        return stream.str();
    }
} // namespace EGL
} // namespace WPEFramework
