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

#include "Cube.h"

namespace WPEFramework {
namespace Plugin {
    namespace {
        static Metadata<Cube> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {});
    }

    /* virtual */ const string Cube::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        string message;
        Config config;

        config.FromString(service->ConfigLine());
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        _service = service;

        _eglRender.Initialize(service->Callsign(), config.Width.Value(), config.Height.Value(), config.FPS.Value());

        JSONRPCRegister();

        if (message.empty() == false) {
            Deinitialize(service);
        }

        _eglRender.Start();

        return message;
    }

    /* virtual */ void Cube::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        _eglRender.Stop();

        JSONRPCUnregister();

        ASSERT(_service == service);
        _service = nullptr;
    }

    /* virtual */ string Cube::Information() const
    {
        // No additional info to report.
        return (string());
    }

    Core::ProxyType<Web::Response> Cube::PutMethod(Core::TextSegmentIterator& index, const Web::Request& /*request*/)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        result->ErrorCode = Web::STATUS_NOT_FOUND;
        result->Message = string(_T("Unknown request path specified."));

        if (index.IsValid() == true && index.Next() == true) {
            // PUT .../Cube/Press|Release : send a code to the end point
            if (index.Current() == _T("Resume")) {
                result->ErrorCode = Web::STATUS_ACCEPTED;
                result->Message = string((_T("setup ok")));
            } else if (index.Current() == _T("Pause")) {

                result->ErrorCode = Web::STATUS_ACCEPTED;
                result->Message = string((_T("reset ok")));
            }
        }

        return (result);
    }

    /* virtual */ void Cube::Inbound(Web::Request& /*request*/)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response> Cube::Process(const Web::Request& request)
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
    void Cube::JSONRPCRegister()
    {
        Register<void, void>(_T("pause"), &Cube::Pause, this);
        Register<void, void>(_T("resume"), &Cube::Resume, this);
    }
    void Cube::JSONRPCUnregister()
    {
        Unregister(_T("pause"));
        Unregister(_T("resume"));
    }

    void Cube::RenderUpdate()
    {
        std::stringstream stream;
        stream.precision(2);

        float fps = _eglRender.FramesPerSecond();

        stream << fps;

        string message("{ \"fps\": " + stream.str() + " }");

        // TRACE(Trace::Information, ("Cube::%s: [%.2f] %s", __FUNCTION__, fps, message.c_str()));

        if (_service != nullptr) {
            _service->Notify(message);
        }

        Notify(message);
    }
} // namespace Plugin
} // namespace WPEFramework
