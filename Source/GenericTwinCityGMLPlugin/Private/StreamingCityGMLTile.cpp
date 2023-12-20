/**

The MIT License (MIT)

Copyright (c) 2023 - present, Hamburg Port Authority


Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

**/


#include "StreamingCityGMLTile.h"

UStreamingCityGMLTile::UStreamingCityGMLTile()
	:	m_Layer(0)
	,	m_Actor(0)
{	
}

UStreamingCityGMLTile::~UStreamingCityGMLTile()
{
}

void UStreamingCityGMLTile::Shutdown()
{
	if(m_Actor)
		m_Actor->Release();
}

bool UStreamingCityGMLTile::Load()
{
	if	(	m_Layer
		&&	m_Layer->LoadCityGMLTile(m_TileId)
		)
	{
		m_TileLoadState = ETileLoadState::Loading;
	}
	else
	{
		// if nothing got loaded, consider this tile an empty tile
		m_TileLoadState = ETileLoadState::Empty;
	}


	return m_TileLoadState == ETileLoadState::Loading;
}

void UStreamingCityGMLTile::Unload()
{
	if(m_Actor)
	{
		m_Actor->Release();
		m_Actor->Destroy();
		m_Actor = 0;
	}
}

void UStreamingCityGMLTile::SetIsHidden(bool IsHidden)
{
	if(m_Actor)
	{
		m_Actor->SetActorHiddenInGame(IsHidden);
	}
}

void UStreamingCityGMLTile::OnTileVisibilityChanged()
{
	if(m_Actor)
	{
		m_Actor->SetActorHiddenInGame( m_TileVisibility < 1.0 );
	}
}

void UStreamingCityGMLTile::AddToWorld(const TObjectPtr<AGenericTwinCustomMeshActor> &Actor)
{
	TObjectPtr<AGenericTwinCustomMeshActor> oldActor = m_Actor;

	m_Actor = Actor;
	OnTileVisibilityChanged();

	m_TileLoadState = ETileLoadState::Loaded;

	if(oldActor)
	{
		oldActor->ConditionalBeginDestroy();
	}
}
