//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Terrain.h"
#include "Others/Noise.h"


namespace maple 
{
	Terrain::Terrain(int32_t width /*= 500*/, int32_t height /*= 500*/, int32_t lowSide /*= 50*/, int32_t lowScale /*= 10*/, float xRand /*= 1.0f*/, float yRand /*= 150.0f*/, float zRand /*= 1.0f*/, float texRandX /*= 1.0f / 16.0f*/, float texRandZ /*= 1.0f / 16.0f*/)
	{
	
	}


	Terrain::Terrain(const std::shared_ptr<VertexBuffer>& vertexBuffer, const std::shared_ptr<IndexBuffer>& indexBuffer)
		:Mesh(vertexBuffer,indexBuffer)
	{
	
	}

	auto Terrain::onUpdate(const Timestep& time, Camera* camera) -> void
	{

	}

};
