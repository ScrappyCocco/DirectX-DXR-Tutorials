#pragma once

#include <cstdint>
#include <d3d12.h>

namespace DirectXUtil
{
	namespace Structs
	{
		struct PipelineConfig
		{
			PipelineConfig(const uint32_t maxTraceRecursionDepth)
			{
				config.MaxTraceRecursionDepth = maxTraceRecursionDepth;

				subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
				subobject.pDesc = &config;
			}

			D3D12_RAYTRACING_PIPELINE_CONFIG config = {};
			D3D12_STATE_SUBOBJECT subobject = {};
		};
	}
}
