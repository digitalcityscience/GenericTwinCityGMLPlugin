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

#include "CityGMLImporterTypes.h"

namespace GenericTwin {

void SCityGMLSurface::CalculateNormals()
{
	TArray<int32> patchedIndices;
	for(int32 i = 0; i < indices.Num(); i += 3)
	{
		const int32 ind0 = indices[i + 2];
		const int32 ind1 = indices[i + 1];
		const int32 ind2 = indices[i];

		FVector v1 = vertices[ind0].position - vertices[ind1].position;
		FVector v2 = vertices[ind2].position - vertices[ind1].position;
		v1.Normalize();
		v2.Normalize();

		if(v1.Equals(v2, 0.0f) == false)
		{
			patchedIndices.Add(ind0);
			patchedIndices.Add(ind1);
			patchedIndices.Add(ind2);

			FVector nrm = FVector::CrossProduct(v1, v2);
			vertices[ind0].normal += nrm;
			vertices[ind1].normal += nrm;
			vertices[ind2].normal += nrm;
		}
	}

	for (auto &vertex : vertices)
		vertex.normal.Normalize();

	indices = patchedIndices;
}

}	//	GenericTwin
