/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "Module.h"

#include "Screensaver.h"

namespace WPEFramework {

namespace Plugin {
    namespace {
        static Metadata<Screensaver> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {});
    }

    /* static */ Core::CriticalSection Screensaver::InputServer::_adminLock;
    /* static */ Screensaver::InputServer::ICallback* Screensaver::InputServer::_callback = nullptr;

    constexpr char connectorNameVirtualInput[] = "/tmp/keyhandler";
    constexpr char clientNameVirtualInput[] = "Screensaver";

    Screensaver::InputServer::InputServer()
        : _virtualinput(nullptr)
    {
        _virtualinput = virtualinput_open(clientNameVirtualInput, connectorNameVirtualInput, VirtualKeyboardCallback, VirtualMouseCallback, VirtualTouchScreenCallback);

        if (_virtualinput == nullptr) {
            TRACE(Trace::Error, ("Initialization of virtualinput failed!"));
        }
    }

    Screensaver::Screensaver()
        : _skipURL(0)
        , _eglRender()
        , _service(nullptr)
        , _previousFrames(0)
        , _previousTimeMS(0)
        , _interval(5000)
        , _timeOut(0)
        , _startTime(0)
        , _inputSink(*this)
        , _ticker(Core::ProxyType<Tick>::Create(*this))
    {
        InputServer::Instance().Callback(&_inputSink);
    }

    /* virtual */ Screensaver::~Screensaver()
    {
        InputServer::Instance().Callback(nullptr);
    }

    /* virtual */ const string Screensaver::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        string message;
        Config config;

        config.FromString(service->ConfigLine());

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        _service = service;
        _previousTimeMS = Core::Time::Now().Ticks() / Core::Time::TicksPerMillisecond;

        _timeOut = config.TimeOut.Value();
        _startTime = Core::Time::Now().Add(_timeOut * 1000).Ticks();

        Core::IWorkerPool::Instance().Schedule(
                Core::Time::Now().Add(_interval), 
                Core::ProxyType<Core::IDispatch>(_ticker));

        JSONRPCRegister();

        if (config.Models.Length() > 0) {
            _eglRender.Initialize(service->Callsign(), config.Width.Value(), config.Height.Value(), config.FPS.Value());
            
            uint16_t index = rand() % config.Models.Length();

            TRACE(Trace::Information, ("Found %d model%s picking number %d", config.Models.Length(), (config.Models.Length() > 1) ? "s" : "", index));

            Graphics::ModelConfig current = config.Models[index];

            if ((config.Models[index].FragmentShaderFile.IsSet() == true) && (config.Models[index].FragmentShaderFile.Value()[0] != '/')) {
                current.FragmentShaderFile = service->DataPath() + "/shaders/" + config.Models[index].FragmentShaderFile.Value();

                TRACE(Trace::Information, ("Fragment file %s", current.FragmentShaderFile.Value().c_str()));
            }

            if ((config.Models[index].VertexShaderFile.IsSet() == true) && (config.Models[index].VertexShaderFile.Value()[0] != '/')) {
                current.VertexShaderFile = service->DataPath() + "/shaders/" + config.Models[index].VertexShaderFile.Value();

                TRACE(Trace::Information, ("Vertex file %s", current.VertexShaderFile.Value().c_str()));
            }

            if (current.Width.Value() == 0) {
                current.Width = config.Width.Value();
            }

            if (current.Height.Value() == 0) {
                current.Height = config.Height.Value();
            }

            uint32_t id = _eglRender.Add(current);

            TRACE(Trace::Information, ("Added model id=%d", id));

            if (config.Instant.Value() == true) {
                _eglRender.Show();
            }            
        } else {
            message = "No render models found.";
        }

        if (message.empty() == false) {
            Deinitialize(service);
        }

        return message;
    }

    /* virtual */ void Screensaver::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        _eglRender.Hide();

        Core::IWorkerPool::Instance().Revoke(
            Core::ProxyType<Core::IDispatch>(_ticker), 
            Core::infinite);

        JSONRPCUnregister();

        ASSERT(_service == service);
        _service = nullptr;
    }

    /* virtual */ string Screensaver::Information() const
    {
        // No additional info to report.
        return (string());
    }

    Core::ProxyType<Web::Response> Screensaver::PutMethod(Core::TextSegmentIterator& index, const Web::Request& /*request*/)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        result->ErrorCode = Web::STATUS_NOT_FOUND;
        result->Message = string(_T("Unknown request path specified."));

        if (index.IsValid() == true && index.Next() == true) {
            // PUT .../Screensaver/Press|Release : send a code to the end point
            if (index.Current() == _T("Resume")) {
                Resume();
                result->ErrorCode = Web::STATUS_ACCEPTED;
                result->Message = string((_T("resumed")));
            } else if (index.Current() == _T("Pause")) {
                Pause();
                result->ErrorCode = Web::STATUS_ACCEPTED;
                result->Message = string((_T("paused")));
            } else if (index.Current() == _T("Show")) {
                Show();
                result->ErrorCode = Web::STATUS_ACCEPTED;
                result->Message = string((_T("shown")));
            } else if (index.Current() == _T("Hide")) {
                Hide();
                result->ErrorCode = Web::STATUS_ACCEPTED;
                result->Message = string((_T("hidden")));
            }
        }

        return (result);
    }

    /* virtual */ void Screensaver::Inbound(Web::Request& /*request*/)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response> Screensaver::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        // If there is nothing or only a slashe, we will now jump over it, and otherwise, we have data.
        if (request.Verb == Web::Request::HTTP_PUT) {
            result = PutMethod(index, request);
        } else {
            result = PluginHost::IFactories::Instance().Response();
            result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
            result->Message = string(_T("Unknown method used."));
        }
        return result;
    }

    // Registration
    void Screensaver::JSONRPCRegister()
    {
        Register<void, void>(_T("pause"), &Screensaver::Pause, this);
        Register<void, void>(_T("resume"), &Screensaver::Resume, this);
        Register<void, void>(_T("hide"), &Screensaver::Hide, this);
        Register<void, void>(_T("show"), &Screensaver::Show, this);
    }
    void Screensaver::JSONRPCUnregister()
    {
        Unregister(_T("pause"));
        Unregister(_T("resume"));
        Unregister(_T("hide"));
        Unregister(_T("show"));
    }

    void Screensaver::RenderUpdate()
    {
        uint64_t currentTimeMS = Core::Time::Now().Ticks() / Core::Time::TicksPerMillisecond;
        uint32_t currentFrames = _eglRender.FramesRendered();

        float fps(0);

        std::stringstream stream;
        stream.precision(2);

        fps = (currentFrames - _previousFrames) / ((currentTimeMS - _previousTimeMS) / Core::Time::MilliSecondsPerSecond);

        stream << fps;

        string message("{ \"fps\": " + stream.str() + " }");

        TRACE(Trace::Information, ("Screensaver::%s: [%.2f] %s", __FUNCTION__, fps, message.c_str()));

        if (_service != nullptr) {
            _service->Notify(message);
        }

        Notify(message);

        _previousTimeMS = currentTimeMS;
        _previousFrames = currentFrames;
    }
} // namespace Plugin
} // namespace WPEFramework
