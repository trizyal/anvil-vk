#include "AnvilShaders.h"

#include <shaderc/shaderc.hpp>

namespace AnvilShaders
{
    shaderc_shader_kind ConvertToShadercKind(const ShaderType inShaderType)
    {
        switch (inShaderType)
        {
        case ST_Vertex:
            return shaderc_vertex_shader;

        case ST_Fragment:
            return shaderc_fragment_shader;

        case ST_Compute:
            return shaderc_compute_shader;

        default:
            return shaderc_glsl_infer_from_source;
        }
    }
}