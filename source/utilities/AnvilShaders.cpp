#include "AnvilShaders.h"

namespace AnvilShaders
{
    inline shaderc_shader_kind toShadercShaderKind(const ShaderType inShaderType)
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