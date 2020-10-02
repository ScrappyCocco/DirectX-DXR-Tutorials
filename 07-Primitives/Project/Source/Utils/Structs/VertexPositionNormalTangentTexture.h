#pragma once

#include "../../../../../_externals/glm/glm/vec2.hpp"
#include "../../../../../_externals/glm/glm/vec3.hpp"

namespace DirectXUtil
{
	namespace Structs
	{
		struct VertexPositionNormalTangentTexture
		{
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec3 tangent;
			glm::vec2 texCoord;

			VertexPositionNormalTangentTexture(const glm::vec3 pos, const glm::vec3 norm,
			                                   const glm::vec3 tan, const glm::vec2 texCor)
			{
				position = pos;
				normal = norm;
				tangent = tan;
				texCoord = texCor;
			}

			VertexPositionNormalTangentTexture() = default;
		};
	}
}
#pragma once
