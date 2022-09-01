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
    typedef std::map<GLenum, const char*> ShaderSourceList;

    #define CASE_STR( value ) case value: return #value; 
    const char* ErrorString( EGLint error )
    {
        switch( error )
        {
        CASE_STR( EGL_SUCCESS             )
        CASE_STR( EGL_NOT_INITIALIZED     )
        CASE_STR( EGL_BAD_ACCESS          )
        CASE_STR( EGL_BAD_ALLOC           )
        CASE_STR( EGL_BAD_ATTRIBUTE       )
        CASE_STR( EGL_BAD_CONTEXT         )
        CASE_STR( EGL_BAD_CONFIG          )
        CASE_STR( EGL_BAD_CURRENT_SURFACE )
        CASE_STR( EGL_BAD_DISPLAY         )
        CASE_STR( EGL_BAD_SURFACE         )
        CASE_STR( EGL_BAD_MATCH           )
        CASE_STR( EGL_BAD_PARAMETER       )
        CASE_STR( EGL_BAD_NATIVE_PIXMAP   )
        CASE_STR( EGL_BAD_NATIVE_WINDOW   )
        CASE_STR( EGL_CONTEXT_LOST        )
        default: return "Unknown";
        }
    }
    #undef CASE_STR


    static string ShaderInfoLog(GLuint handle)
    {
        ASSERT(glIsShader(handle));

        string log;
        GLint length;

        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &length);

        if (length > 1) {
            log.resize(length);
            glGetShaderInfoLog(handle, length, NULL, &log[0]);
        }

        return log;
    }

    static string ProgramInfoLog(GLuint handle)
    {
        ASSERT(glIsProgram(handle));

        string log;
        GLint length(0);

        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length);

        if (length > 1) {
            log.resize(length);
            glGetProgramInfoLog(handle, length, NULL, &log[0]);
        }

        return log;
    }

    static GLuint CompileShader(GLenum type, const char* code)
    {
        GLint status(GL_FALSE);
        GLuint handle = glCreateShader(type);

        glShaderSource(handle, 1, &code, NULL);
        glCompileShader(handle);

        glGetShaderiv(handle, GL_COMPILE_STATUS, &status);

        if (status == GL_FALSE) {
            TRACE_GLOBAL(Trace::Error, ("Shader %d compilation failed: %s", handle, ShaderInfoLog(handle).c_str()));

            glDeleteShader(handle);
            handle = 0;
        }

        return handle;
    }

    static GLuint CreateProgram(const ShaderSourceList& sources)
    {
        GLuint handle(0);

        std::vector<GLuint> ids;

        for (auto shader : sources) {
            TRACE_GLOBAL(Trace::Information, ("Compiling shader type 0x%04X", shader.first));

            GLuint id = CompileShader(shader.first, shader.second);

            if (id > 0) {
                ids.push_back(id);
            };
        }

        if (ids.size() == sources.size()) {
            handle = glCreateProgram();

            for (auto id : ids) {
                glAttachShader(handle, id);
            }
        } else {
            TRACE_GLOBAL(Trace::Error, ("Not all shaders were compiled correctly."));
        }

        return handle;
    }

    static GLint LinkProgram(GLuint handle)
    {
        GLint status(GL_FALSE);

        glLinkProgram(handle);

        glGetProgramiv(handle, GL_LINK_STATUS, &status);

        if (status == GL_FALSE) {
            TRACE_GLOBAL(Trace::Error, ("Program linking failed: %s", ProgramInfoLog(handle).c_str()));
        }

        return status;
    }

    static string ConfigInfoLog(EGLDisplay dpy, EGLConfig config)
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

        stream << "id=" << id << " buffer=" << bufferSize << " red=" << redSize << " green=" << greenSize << " blue=" << blueSize << " alpha=" << alphaSize << " depth=" << depthSize << " stencil=" << stencilSize;

        return stream.str();
    }

    static std::vector<EGLConfig> MatchConfigs(EGLDisplay dpy, const EGLint attrib_list[])
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

} // namespace EGL
} // namespace WPEFramework
