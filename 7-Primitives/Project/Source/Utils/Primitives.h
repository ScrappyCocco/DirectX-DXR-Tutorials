#pragma once

#include <vector>

#include "Structs/VertexPositionNormalTangentTexture.h"

namespace DirectXUtil
{
	class Primitives
	{
	public:
		/**
		 * General Shape information
		 */
		struct Shape
		{
			std::vector<Structs::VertexPositionNormalTangentTexture> vertexData;
			std::vector<unsigned short> indexData;
		};
		/**
		 * Sphere Primitive Generation
		 */
		static Shape createSphere(float diameter, int tessellation, bool uvHorizontalFlip = false, bool uvVerticalFlip = false);
	private:
		static void calculateTangentSpace(Shape& shape);
	};
}
