/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Module.h"

#include "EGLRender.h"
#include "IModel.h"

#include <simpleworker/SimpleWorker.h>
#include <virtualinput/virtualinput.h>

namespace WPEFramework {
namespace Plugin {
    class Screensaver : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    private:
        class InputServer {
        public:
            InputServer(const InputServer&) = delete;
            InputServer& operator=(const InputServer&) = delete;

            struct EXTERNAL ICallback {
                virtual ~ICallback() = default;
                virtual void Trigger() = 0;
            }; // struct ICallback

            InputServer();
            ~InputServer();

            static void VirtualKeyboardCallback(keyactiontype type VARIABLE_IS_NOT_USED, unsigned int code VARIABLE_IS_NOT_USED)
            {
                Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);
                if (_callback != nullptr) {
                    _callback->Trigger();
                }
            }

            static void VirtualMouseCallback(mouseactiontype type VARIABLE_IS_NOT_USED, unsigned short button VARIABLE_IS_NOT_USED, signed short horizontal VARIABLE_IS_NOT_USED, signed short vertical VARIABLE_IS_NOT_USED)
            {
                Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);
                if (_callback != nullptr) {
                    _callback->Trigger();
                }
            }

            static void VirtualTouchScreenCallback(touchactiontype type VARIABLE_IS_NOT_USED, unsigned short index VARIABLE_IS_NOT_USED, unsigned short x VARIABLE_IS_NOT_USED, unsigned short y VARIABLE_IS_NOT_USED)
            {
                Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);
                if (_callback != nullptr) {
                    _callback->Trigger();
                }
            }

            void Callback(ICallback* callback)
            {
                Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);
                ASSERT((callback == nullptr) ^ (_callback == nullptr));
                _callback = callback;
            }

            void Connect(const string& connector);
            void Disconnect();
        private:
        private:
            static Core::CriticalSection _adminLock;
            static ICallback* _callback;
            void* _virtualinput;
        }; // class InputServer

    public:
        Screensaver(const Screensaver&) = delete;
        Screensaver& operator=(const Screensaver&) = delete;
        Screensaver();

        virtual ~Screensaver() override;

        BEGIN_INTERFACE_MAP(Screensaver)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    private:
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);

        class InputSink : public InputServer::ICallback {
        public:
            InputSink() = delete;
            InputSink(const InputSink&) = delete;
            InputSink& operator=(const InputSink&) = delete;

            InputSink(Screensaver& parent)
                : _parent(parent)
            {
            }

            ~InputSink() = default;

            void Trigger()
            {
                TRACE(Trace::Information, ("Input detected!"));
                _parent.Trigger();
            }

        private:
            Screensaver& _parent;
        };

        class Tick : public Core::IDispatch {
        public:
            Tick(Screensaver& parent)
                : _parent(parent)
                , _interval(0)
            {
                TRACE(Trace::Information, ("Created Tick %p", this));
            }

            void Schedule(const uint16_t interval)
            {
                _interval = interval;
                Core::IWorkerPool::Instance()
                    .Schedule(
                        Core::Time::Now().Add(_interval),
                        Core::ProxyType<Core::IDispatch>(*this));
            }

            void Dispatch() override
            {
                _parent.Tack();

                if (_parent.IsActive()) {
                    Schedule(_interval);
                }
            }

        private:
            Screensaver& _parent;
            uint16_t _interval;
        };

    public:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , Height(720)
                , Width(1280)
                , FPS(25)
                , TimeOut(15 * 60) /* 15 minutes in s */
                , FadeIn(0) /* milliseconds*/
                , Instant(false)
                , Interval(5) /* seconds; 0 = no off*/
                , ReportFPS(false)
                , Models()
            {
                Add(_T("height"), &Height);
                Add(_T("width"), &Width);
                Add(_T("fps"), &FPS);
                Add(_T("timeout"), &TimeOut);
                Add(_T("fadein"), &FadeIn);
                Add(_T("instant"), &Instant);
                Add(_T("interval"), &Interval);
                Add(_T("reportfps"), &ReportFPS);
                Add(_T("models"), &Models);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Height;
            Core::JSON::DecUInt16 Width;
            Core::JSON::DecUInt8 FPS;
            Core::JSON::DecUInt16 TimeOut;
            Core::JSON::DecUInt16 FadeIn;
            Core::JSON::Boolean Instant;
            Core::JSON::DecUInt8 Interval;
            Core::JSON::Boolean ReportFPS;
            Core::JSON::ArrayType<Graphics::ModelConfig> Models;
        };

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        // JSON RPC
        // -------------------------------------------------------------------------------------------------------
        void JSONRPCRegister();
        void JSONRPCUnregister();
        uint32_t JSONRPCPause();
        uint32_t JSONRPCResumed();

    private:
        void RenderUpdate();

        inline uint32_t Pause()
        {
            _eglRender.Pause();
            return Core::ERROR_NONE;
        }

        inline uint32_t Resume()
        {
            _eglRender.Resume();
            _ticker->Schedule(_interval);
            return Core::ERROR_NONE;
        }

        inline uint32_t Hide()
        {
            Trigger();
            return Core::ERROR_NONE;
        }

        inline uint32_t Show()
        {
            _eglRender.Show();
            _ticker->Schedule(_interval);
            return Core::ERROR_NONE;
        }

        void Trigger()
        {
            if (Core::Time::Now().Add(_timeOut * 1000).Ticks() >= (_startTime + (Core::Time::TicksPerMillisecond * 1000 * 2))) {
                TRACE(Trace::Information, ("Input detected!"));
                _eglRender.Hide();
                _startTime = Core::Time::Now().Add(_timeOut * 1000).Ticks();
                _triggered = false;
            } else {
                TRACE(Trace::Information, ("Hey input! Cooldown!"));
            }
        }

        bool Tack()
        {
            if ((_startTime - Core::Time::Now().Ticks() <= 0) && (_triggered == false)) {
                _eglRender.Show();
                _triggered = true;
            }

            if (_reportFPS == true) {
                RenderUpdate();
            }

            return _triggered;
        }

        bool IsActive() const
        {
            return _eglRender.IsActive();
        }

    private:
        uint8_t _skipURL;
        Graphics::EGLRender _eglRender;
        PluginHost::IShell* _service;

        bool _triggered;

        uint32_t _previousFrames;
        uint64_t _previousTimeMS;

        uint16_t _interval;
        uint16_t _timeOut;
        uint64_t _startTime;

        bool _reportFPS;

        InputSink _inputSink;
        Core::ProxyType<Tick> _ticker;

        InputServer _inputServer;
    };
} // namespace Plugin
} // namespace WPEFramework
