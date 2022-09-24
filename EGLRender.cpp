#include "Module.h"

#include "EGLRender.h"
#include "EGLToolbox.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <compositor/Client.h>
#include <simpleworker/SimpleWorker.h>

namespace WPEFramework {
namespace Graphics {
    static constexpr uint8_t RenderUpdateIntervalSeconds = 5;

    static constexpr EGLint RedBufferSize = 8;
    static constexpr EGLint GreenBufferSize = 8;
    static constexpr EGLint BlueBufferSize = 8;
    static constexpr EGLint AlphaBufferSize = 8;
    static constexpr EGLint DepthBufferSize = 0;

    constexpr EGLint defaultContextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    constexpr EGLint defaultConfigAttribs[] = {
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

    EGLRender::EGLRender()
        : _adminLock()
        , _display(nullptr)
        , _surface(nullptr)
        , _eglSurface(EGL_NO_SURFACE)
        , _eglContext(EGL_NO_CONTEXT)
        , _eglDisplay(EGL_NO_DISPLAY)
        , _fps(60)
        , _framesRendered(0)
        , _models()
        , _suspend(false)
    {
    }

    EGLRender::~EGLRender()
    {
        Hide();

        Stop();
        Wait(WPEFramework::Core::Thread::STOPPED, WPEFramework::Core::infinite);

        DeinitEGL();

        for (auto model : _models) {
            model.second.Release();
        }

        if (_surface != nullptr) {
            _surface->Release();
        }

        if (_display != nullptr) {
            _display->Release();
        }
    }

    void EGLRender::Initialize(const string& name, const uint32_t width, const uint32_t height, const uint16_t fps)
    {
        std::stringstream strm;

        TRACE(Trace::Information, ("EGLRender::%s name=%s width=%d, heigth=%d fps=%d", __FUNCTION__, name.c_str(), width, height, fps));

        _fps = fps;

        strm << name << "-" << time(NULL);

        _display = Compositor::IDisplay::Instance(strm.str());
        ASSERT(_display != nullptr);

        _surface = _display->Create(strm.str(), width, height);
        ASSERT(_surface != nullptr);

        InitEGL();
    }

    uint32_t EGLRender::Add(const ModelConfig config)
    {
        static uint32_t identifier = 1;

        _models.emplace(std::piecewise_construct,
            std::forward_as_tuple(identifier),
            std::forward_as_tuple(IModel::Create(config)));

        TRACE(Trace::Information, ("Added Model %d", identifier));

        return identifier++;
    }

    void EGLRender::Remove(const uint32_t identifier)
    {
        ModelMap::iterator index(_models.find(identifier));

        ASSERT(index != _models.end());

        if (index != _models.end()) {
            index->second.Release();
            _models.erase(index);
            TRACE(Trace::Information, ("Removed Model %d", identifier));
        }
    }

    bool EGLRender::InitEGL()
    {
        ASSERT(_eglDisplay == EGL_NO_DISPLAY);
        ASSERT(_eglSurface == EGL_NO_SURFACE);
        ASSERT(_eglContext == EGL_NO_CONTEXT);

        EGLint majorVersion(0);
        EGLint minorVersion(0);
        EGLint eglResult(0);
        EGLint numConfigs(0);
        EGLConfig eglConfig;

        _eglDisplay = eglGetDisplay(_display->Native());
        ASSERT(_eglDisplay != EGL_NO_DISPLAY);

        TRACE(Trace::Information, ("EGL Display %p", _eglDisplay));

        eglResult = eglInitialize(_eglDisplay, &majorVersion, &minorVersion);
        ASSERT(eglResult == EGL_TRUE);

        TRACE(Trace::Information, ("Initialized EGL v%d.%d", majorVersion, minorVersion));

        eglResult = eglBindAPI(EGL_OPENGL_ES_API);
        ASSERT(eglResult == EGL_TRUE);

        eglResult = eglChooseConfig(_eglDisplay, defaultConfigAttribs, &eglConfig, 1, &numConfigs);
        ASSERT(eglResult == EGL_TRUE);

        TRACE(Trace::Information, ("Choosen config: %s", EGL::ConfigInfoLog(_eglDisplay, eglConfig).c_str()));

        _eglContext = eglCreateContext(_eglDisplay, eglConfig, EGL_NO_CONTEXT, defaultContextAttribs);
        ASSERT(_eglContext != EGL_NO_CONTEXT);

        _eglSurface = eglCreateWindowSurface(_eglDisplay, eglConfig, _surface->Native(), nullptr);
        ASSERT(_eglSurface != EGL_NO_SURFACE);

        EGLint width, heigth;
        eglQuerySurface(_eglDisplay, _eglSurface, EGL_WIDTH, &width);
        eglQuerySurface(_eglDisplay, _eglSurface, EGL_HEIGHT, &heigth);

        TRACE(Trace::Information, ("EGL surface dimension: %dx%d", width, heigth));

        if (eglMakeCurrent(_eglDisplay, _eglSurface, _eglSurface, _eglContext) == EGL_FALSE) {
            TRACE(Trace::Error, ("Unable to make EGL context current error=%s", EGL::ErrorString(eglGetError())));
        } else {
            TRACE(Trace::Information, ("EGL Ready: %s %s",  EGL::EGLInfo(_eglDisplay).c_str(), EGL::OpenGLInfo().c_str()));
            eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }

        return (_eglSurface != EGL_NO_SURFACE);
    }

    bool EGLRender::DeinitEGL()
    {
        if (_eglDisplay != EGL_NO_DISPLAY) {
            if (eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE) {
                TRACE(Trace::Error, ("EGL make current failed error=%s", EGL::ErrorString(eglGetError())));
            }

            if (eglDestroySurface(_eglDisplay, _eglSurface) == EGL_TRUE) {
                _eglSurface = EGL_NO_SURFACE;
            } else {
                TRACE(Trace::Error, ("EGL destroy surface failed error=%s", EGL::ErrorString(eglGetError())));
            }

            if (eglDestroyContext(_eglDisplay, _eglContext) == EGL_TRUE) {
                _eglContext = EGL_NO_CONTEXT;
            } else {
                TRACE(Trace::Error, ("EGL destroy context failed error=%s", EGL::ErrorString(eglGetError())));
            }

            if (eglTerminate(_eglDisplay) == EGL_TRUE) {
                _eglDisplay = EGL_NO_DISPLAY;
            } else {
                TRACE(Trace::Error, ("EGL terminate failed error=%s", EGL::ErrorString(eglGetError())));
            }
        }

        ASSERT(_eglDisplay == EGL_NO_DISPLAY);
        ASSERT(_eglSurface == EGL_NO_SURFACE);
        ASSERT(_eglContext == EGL_NO_CONTEXT);

        return ((_eglDisplay == EGL_NO_DISPLAY) && (_eglSurface == EGL_NO_SURFACE) && (_eglContext == EGL_NO_CONTEXT));
    }

    void EGLRender::LockContext()
    {
        _adminLock.Lock();

        EGLBoolean result = eglMakeCurrent(_eglDisplay, _eglSurface, _eglSurface, _eglContext);

        if (result == EGL_FALSE) {
            TRACE(Trace::Error, ("Unable to make EGL context current error=%s", EGL::ErrorString(eglGetError())));
        }

        ASSERT(result != GL_FALSE);
    }

    void EGLRender::UnlockContext()
    {
        EGLBoolean result = eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        _adminLock.Unlock();

        if (result == EGL_FALSE) {
            TRACE(Trace::Error, ("Unable to make EGL context current error=%s", EGL::ErrorString(eglGetError())));
        }

        ASSERT(result != GL_FALSE);
    }

    void EGLRender::Show()
    {
        if ((_eglDisplay != EGL_NO_DISPLAY) && (_active == false)) {
            TRACE(Trace::Information, ("Show Render"));

            Block();

            LockContext();

            for (auto model : _models) {
                if (model.second->IsValid() == false) {
                    model.second->Construct();
                }
            }

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            Present();

            _active = true;

            UnlockContext();

            Resume();
        }
    }

    void EGLRender::Hide()
    {
        if ((_eglDisplay != EGL_NO_DISPLAY) && (_active == true)) {
            Pause();

            LockContext();

            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            Present();

            for (auto model : _models) {
                if (model.second->IsValid() == true) {
                    model.second->Destroy();
                }
            }

            _active = false;

            UnlockContext();

            TRACE(Trace::Information, ("Hide Render render blocked=%s", IsBlocked() ? "yes" : "no"));
        }
    }

    void EGLRender::Pause()
    {
        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        if ((_active == true) && (_suspend == false)) {
            _suspend = true;
            Wait(Core::Thread::BLOCKED, 1000);
            TRACE(Trace::Information, ("Paused Render"));
        }
    }

    void EGLRender::Resume()
    {
        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        if (_active == true) {
            _suspend = false;
            Run();
            TRACE(Trace::Information, ("Resumed Render"));
        }
    }

    void EGLRender::Present()
    {
        if (eglSwapBuffers(_eglDisplay, _eglSurface) == GL_TRUE) {
            ++_framesRendered;
        } else {
            TRACE(Trace::Error, ("eglSwapBuffers failed error=%s", EGL::ErrorString(eglGetError())));
        }
    }

    uint32_t EGLRender::Worker()
    {
        LockContext();

        if (_suspend == false) {
            for (auto model : _models) {
                if (model.second->IsValid() == true) {
                    model.second->Process();
                }
            }

            Present();
        }

        Block();

        UnlockContext();

        return ((_fps == 0) || (_suspend == true)) ? Core::infinite : (1000 / _fps);
    }

} // namespace Graphics
} // namespace WPEFramework