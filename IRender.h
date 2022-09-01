#pragma once

#include <core/core.h>

namespace WPEFramework {
namespace Graphics {
    struct EXTERNAL IRender {
        static IRender* Instance();

        virtual ~IRender() = default;

        virtual bool Setup(const string& name, const uint32_t width, const uint32_t height) = 0;
        
        virtual void Present() = 0;
        virtual uint32_t Frames() const = 0;
    };
} // namespace Graphics
} // namespace WPEFramework