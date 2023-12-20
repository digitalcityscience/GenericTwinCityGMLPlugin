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

#include "CityGMLGeometryExtractor.h"
#include "Common/BaseTypes.h"
#include "ICityGMLImporter.h"

CityGMLGeometryExtractor::CityGMLGeometryExtractor(const TSharedPtr<GenericTwin::ICityGMLImporter>&Importer)
	:	m_Importer(Importer)
{
}


void CityGMLGeometryExtractor::ExtractBuildings(GenericTwin::ICityGMLCoordinateTransform &Transform, GenericTwin::SMeshSection &WallSection, GenericTwin::SMeshSection &RoofSection)
{
	TArray<GenericTwin::SCityGMLBuilding> buildings = m_Importer->GetBuildings(Transform);

	const float RoofThreshold = 0.9f;
	const float WallThreshold = 0.2f;

	for (auto &building : buildings)
	{
		for (auto &surface : building.surfaces)
		{
			GenericTwin::SCityGMLSurface::Type surfaceType = surface.surface_type;
			if(surfaceType == GenericTwin::SCityGMLSurface::Type::Unknown)
			{
				surfaceType = surface.vertices[surface.indices[0]].normal.Z <= WallThreshold ? GenericTwin::SCityGMLSurface::Type::Wall : GenericTwin::SCityGMLSurface::Type::Roof;
			}

			if	(	surfaceType != GenericTwin::SCityGMLSurface::Type::Wall
				&&	surfaceType != GenericTwin::SCityGMLSurface::Type::Roof
				)
				continue;

			GenericTwin::SMeshSection &meshSection = surfaceType == GenericTwin::SCityGMLSurface::Type::Wall ? WallSection : RoofSection;

			const int32 startOffset = meshSection.Vertices.Num();
			for(const auto &vertex : surface.vertices)
			{
				meshSection.Vertices.Add(vertex.position);
				meshSection.UV0.Add(vertex.texture_coordinate);
				meshSection.Normals.Add(vertex.normal);
			}

			for(int32 i = 0, count = surface.indices.Num() / 3; i < count; ++i)
			{
				meshSection.Indices.Add(surface.indices[3 * i] + startOffset);
				meshSection.Indices.Add(surface.indices[3 * i + 1] + startOffset);
				meshSection.Indices.Add(surface.indices[3 * i + 2] + startOffset);
			}
		}
	}

}
