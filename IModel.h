#pragma once

#include <core/core.h>
#include <map>

#include "Module.h"

namespace WPEFramework {
namespace Graphics {
    class EXTERNAL ModelConfig : public Core::JSON::Container {
    public:
        ModelConfig(const ModelConfig& copy)
            : Core::JSON::Container()
            , X(copy.X)
            , Y(copy.X)
            , Z(copy.Z)
            , Height(copy.Height)
            , Width(copy.Width)
            , VertexShaderSource(copy.VertexShaderSource)
            , VertexShaderFile(copy.VertexShaderFile)
            , FragmentShaderSource(copy.FragmentShaderSource)
            , FragmentShaderFile(copy.FragmentShaderFile)
        {
            Add(_T("x"), &X);
            Add(_T("y"), &Y);
            Add(_T("z"), &Z);
            Add(_T("height"), &Height);
            Add(_T("width"), &Width);
            Add(_T("vertexfile"), &VertexShaderFile);
            Add(_T("vertexsource"), &VertexShaderSource);
            Add(_T("fragmentfile"), &FragmentShaderFile);
            Add(_T("fragmentsource"), &FragmentShaderSource);
        }

        ModelConfig& operator=(const ModelConfig& RHS)
        {
            X = RHS.X;
            Y = RHS.Y;
            Z = RHS.Z;
            Height = RHS.Height;
            Width = RHS.Width;
            VertexShaderSource = RHS.VertexShaderSource;
            VertexShaderFile = RHS.VertexShaderFile;
            FragmentShaderSource = RHS.VertexShaderSource;
            FragmentShaderFile = RHS.FragmentShaderFile;

            return (*this);
        }

        ModelConfig()
            : Core::JSON::Container()
            , X(0)
            , Y(0)
            , Z(0)
            , Height(0)
            , Width(0)
            , VertexShaderSource()
            , VertexShaderFile()
            , FragmentShaderSource()
            , FragmentShaderFile()
        {
            Add(_T("x"), &X);
            Add(_T("y"), &Y);
            Add(_T("z"), &Z);
            Add(_T("height"), &Height);
            Add(_T("width"), &Width);
            Add(_T("vertexfile"), &VertexShaderFile);
            Add(_T("vertexsource"), &VertexShaderSource);
            Add(_T("fragmentfile"), &FragmentShaderFile);
            Add(_T("fragmentsource"), &FragmentShaderSource);
        }

        virtual ~ModelConfig()
        {
        }

    public:
        Core::JSON::DecUInt16 X;
        Core::JSON::DecUInt16 Y;
        Core::JSON::DecUInt16 Z;
        Core::JSON::DecUInt16 Height;
        Core::JSON::DecUInt16 Width;
        Core::JSON::String VertexShaderSource;
        Core::JSON::String VertexShaderFile;
        Core::JSON::String FragmentShaderSource;
        Core::JSON::String FragmentShaderFile;
    };

    typedef struct Size {
        Size()
            : Width(0)
            , Height(0)
        {
        }

        Size(uint16_t width, uint16_t height)
            : Width(width)
            , Height(height)
        {
        }
        uint16_t Width;
        uint16_t Height;
    } SizeType;

    typedef struct Dimension {
        Dimension()
            : X(0)
            , Y(0)
            , Z(0)
        {
        }

        Dimension(uint16_t x, uint16_t y, uint16_t z = 0)
            : X(x)
            , Y(y)
            , Z(z)
        {
        }
        uint16_t X;
        uint16_t Y;
        uint16_t Z;
    } DimensionType;

    struct EXTERNAL IModel {
        static Core::ProxyType<IModel> Create(const ModelConfig& config);

        virtual ~IModel() = default;

        virtual bool IsValid() const = 0;

        virtual bool Construct() = 0;
        virtual bool Destroy() = 0;

        virtual void Process() = 0;

        virtual void Position(const DimensionType& dimension) = 0;
        virtual void Size(const SizeType& size) = 0;
        virtual void Opacity(const uint8_t opacity) = 0;
    };
} // namespace Graphics
} // namespace WPEFramework