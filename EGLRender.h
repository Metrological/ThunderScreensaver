#pragma once

#include "Module.h"

#include "IModel.h"
#include "Tracing.h"

#ifndef GL_ES_VERSION_2_0
#include <GLES2/gl2.h>
#endif
#include <EGL/egl.h>

#include <compositor/Client.h>

#include <condition_variable>
#include <mutex>

namespace Thunder {
namespace Graphics {
    class EGLRender : public Core::Thread, public Compositor::IDisplay::ISurface::ICallback {
    public:
        struct EXTERNAL INotification {
            virtual ~INotification() = default;
            virtual void Triggered() = 0;
        };

    private:
        bool InitEGL();
        bool DeinitEGL();

        void Present();
        void UnlockContext();
        void LockContext();

    public:
        EGLRender(const EGLRender&) = delete;
        EGLRender& operator=(const EGLRender&) = delete;

        EGLRender();

        virtual ~EGLRender();

        bool Initialize(const string& name, const uint32_t width, const uint32_t height, const uint16_t fps);
        void Deinitialize();

        uint32_t Add(const ModelConfig config);
        void Remove(uint32_t id);

        inline uint32_t FramesRendered() const
        {
            return _framesRendered;
        }

        // ICallback methods
        void Rendered(Compositor::IDisplay::ISurface* surface) override;
        void Published(Compositor::IDisplay::ISurface* surface) override;

        // IThread methods
        uint32_t Worker() override;

        void Show();
        void Hide();
        void Pause();
        void Resume();

        bool IsActive() const
        {
            return _active;
        }

    private:
        void WaitForVSync(uint32_t timeoutMs)
        {
            std::unique_lock<std::mutex> lock(_rendering);

            if (timeoutMs == Core::infinite) {
                _vsync.wait(lock);
            } else {
                _vsync.wait_for(lock, std::chrono::milliseconds(timeoutMs));
            }
        }

        typedef std::map<uint32_t, Core::ProxyType<IModel>> ModelMap;

        mutable Core::CriticalSection _adminLock;

        Compositor::IDisplay* _display;
        Compositor::IDisplay::ISurface* _surface;

        EGLSurface _eglSurface;
        EGLContext _eglContext;
        EGLDisplay _eglDisplay;

        uint16_t _fps;
        uint32_t _framesRendered;

        ModelMap _models;

        bool _suspend;
        bool _active;

        std::condition_variable _vsync;
        std::mutex _rendering;

    }; // class EGLRender

} // namespace Graphics
} // namespace Thunder