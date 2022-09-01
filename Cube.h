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

#include "IRender.h"
#include <simpleworker/SimpleWorker.h>

namespace WPEFramework {
namespace Plugin {
    class Cube : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        Cube(const Cube&) = delete;
        Cube& operator=(const Cube&) = delete;

        Cube()
            : _skipURL(0)
            , _eglRender(*this)
            , _service(nullptr)
        {
        }

        virtual ~Cube() override
        {
        }

        BEGIN_INTERFACE_MAP(Cube)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    private:
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);

        class EGLRender {
        private:
            static constexpr uint16_t RenderUpdateInterval = 5000;

            uint64_t Trigger()
            {
                Core::Time NextTick(Core::Time::Now());
                NextTick.Add(1000 / _fps);

                // _eglModel->Present();

                return (NextTick.Ticks());
            }

            uint64_t Report()
            {
                Core::Time NextTick(Core::Time::Now());
                NextTick.Add(RenderUpdateInterval);

                _parent.RenderUpdate();

                return (NextTick.Ticks());
            }

            class Renderer : public Core::SimpleWorker::ICallback {
            public:
                Renderer() = delete;
                Renderer(const Renderer&) = delete;
                Renderer& operator=(const Renderer&) = delete;

                Renderer(EGLRender& parent)
                    : _parent(parent)
                {
                }

                virtual ~Renderer() = default;

            public:
                uint64_t Activity()
                {
                    return _parent.Trigger();
                }

            private:
                EGLRender& _parent;
            }; // class Renderer

            class Reporter : public Core::SimpleWorker::ICallback {
            public:
                Reporter() = delete;
                Reporter(const Reporter&) = delete;
                Reporter& operator=(const Reporter&) = delete;

                Reporter(EGLRender& parent)
                    : _parent(parent)
                {
                }

                virtual ~Reporter() = default;

            public:
                uint64_t Activity()
                {
                    return _parent.Report();
                }

            private:
                EGLRender& _parent;
            }; // class Reporter

        public:
            EGLRender(const EGLRender&) = delete;
            EGLRender& operator=(const EGLRender&) = delete;

            EGLRender(Cube& parent)
                : _parent(parent)
                , _fps(25)
                , _framesRendered(0)
                , _renderer(*this)
                , _reporter(*this)
                , _eglModel(Graphics::IRender::Instance())
            {
                ASSERT(_eglModel != nullptr);
                Core::SimpleWorker::Instance().Submit(&_reporter);
            }

            virtual ~EGLRender()
            {
                Core::SimpleWorker::Instance().Revoke(&_reporter);
                Stop();
            }

            void Initialize(const string& name, const uint32_t width, const uint32_t height, const uint16_t fps)
            {
                TRACE(Trace::Information, ("EGLRender::%s name=%s width=%d, heigth=%d fps=%d", __FUNCTION__, name.c_str(), width, height, fps));
                _fps = fps;
                _eglModel->Setup(name, width, height);
                _eglModel->Present();
            }

            inline float FramesPerSecond()
            {
                uint32_t previousFrames(_framesRendered);

                _framesRendered = _eglModel->Frames();

                return (_framesRendered - previousFrames) / (RenderUpdateInterval / 1000);
            }

            inline uint32_t Start()
            {
                TRACE(Trace::Information, ("EGLRender::%s", __FUNCTION__));
                return Core::SimpleWorker::Instance().Submit(&_renderer);
            }

            inline uint32_t Stop()
            {
                TRACE(Trace::Information, ("EGLRender::%s", __FUNCTION__));
                return Core::SimpleWorker::Instance().Revoke(&_renderer);
            }

        private:
            Cube& _parent;
            uint16_t _fps;
            uint32_t _framesRendered;
            Renderer _renderer;
            Reporter _reporter;
            Graphics::IRender* _eglModel;
        }; // class EGLRender

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Height(720)
                , Width(1280)
                , FPS(25)
            {
                Add(_T("height"), &Height);
                Add(_T("width"), &Width);
                Add(_T("fps"), &FPS);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Height;
            Core::JSON::DecUInt16 Width;
            Core::JSON::DecUInt8 FPS;
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
            return _eglRender.Stop();
        }

        inline uint32_t Resume()
        {
            return _eglRender.Start();
        }

    private:
        uint8_t _skipURL;
        EGLRender _eglRender;
        PluginHost::IShell* _service;
    };
} // namespace Plugin
} // namespace WPEFramework
