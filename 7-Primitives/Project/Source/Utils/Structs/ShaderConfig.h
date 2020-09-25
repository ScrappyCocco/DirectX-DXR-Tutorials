#pragma once

#include <cstdint>
#include <d3d12.h>

namespace DirectXUtil
{
	namespace Structs
	{
		struct ShaderConfig
		{
			ShaderConfig(const uint32_t maxAttributeSizeInBytes, const uint32_t maxPayloadSizeInBytes)
			{
				shaderConfig.MaxAttributeSizeInBytes = maxAttributeSizeInBytes;
				shaderConfig.MaxPayloadSizeInBytes = maxPayloadSizeInBytes;

				subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
				subobject.pDesc = &shaderConfig;
			}

			D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
			D3D12_STATE_SUBOBJECT subobject = {};
		};
	}
}
