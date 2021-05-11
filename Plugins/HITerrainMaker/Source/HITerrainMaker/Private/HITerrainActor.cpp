/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include "HITerrainActor.h"
#include "Providers/RuntimeMeshProviderStatic.h"
#include "TerrainDatas/HITerrainChunkData.h"

void AHITerrainActor::Initialize(UHITerrainData* Data, FTerrainInformationPtr InTerrainInformation, const TPair<int32, int32>& InIndex)
{
	Index = InIndex;
	ChunkData = Data->GetChunkData(Index);
	TerrainInformation = InTerrainInformation;
	StaticProvider = NewObject<URuntimeMeshProviderStatic>(this, TEXT("RuntimeMeshProvider-Static"));
	if (StaticProvider)
	{
		GetRuntimeMeshComponent()->Initialize(StaticProvider);
	}
	if(TerrainInformation->bEnableLOD)
	{
		Size = TerrainInformation->ChunkSize / TerrainInformation->LODHighQuality;
		// Step = TerrainInformation->LODHighQuality;
	}
	else
	{
		Size = TerrainInformation->ChunkSize / TerrainInformation->LODLowQuality;
		// Step = TerrainInformation->LODLowQuality;
	}
}

void AHITerrainActor::DeleteChunk()
{
	if(StaticProvider)
	{
		StaticProvider->ClearSection(0, 0);
		bGenerated = false;
		bWaterGenerated = false;
	}
}

void AHITerrainActor::GenerateChunk(ELODLevel LODLevel)
{
	if (StaticProvider){
		StaticProvider->SetupMaterialSlot(0, TEXT("Material"), Material);
		TArray<FVector> Positions;
		TArray<FColor> Colors;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> TexCoords;
		TArray<FRuntimeMeshTangent> Tangents;

		// GeneratePositions(Positions, LODLevel);
		// GenerateTriangles(Triangles, LODLevel);
		// GenerateNormals(Normals, LODLevel);
		// GenerateTexCoords(TexCoords, LODLevel);
		// GenerateTangents(Tangents, LODLevel);
		// GenerateColors(Colors, LODLevel);
		GenerateChunk1(Positions, TexCoords, Colors, LODLevel);
		GenerateChunk2(Normals, Tangents, Colors, LODLevel);
		GenerateChunk3(Triangles, LODLevel);
		if(bGenerated)
		{
			StaticProvider->UpdateSectionFromComponents(0, 0, Positions, Triangles, Normals, TexCoords, Colors, Tangents);
		}
		else
		{
			StaticProvider->CreateSectionFromComponents(0, 0, 0, Positions, Triangles, Normals, TexCoords, Colors, Tangents, ERuntimeMeshUpdateFrequency::Frequent, true);
			bGenerated = true;
		}

		if(ChunkData->Data->ContainsChannel("water"))
		{
			GenerateWater(LODLevel);
		}
	}
}

bool AHITerrainActor::IsGenerated()
{
	return bGenerated;
}

void AHITerrainActor::GenerateChunk1(TArray<FVector>& Positions, TArray<FVector2D>& TexCoords, TArray<FColor>& Colors,
	ELODLevel LODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(LODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(LODLevel);
	int32 OuterSize = ChunkData->GetOuterPointSize(LODLevel);
	bool bContainSediment = ChunkData->Data->ContainsChannel("sediment");
	FColor BasicColor(127, 127, 127, 255);
	FColor SedimentColor(0, 255, 0, 255);
	float Step = ChunkData->GetStepOfLODLevel(LODLevel);
	/*
	 * 内部的点
	 */
	float RecentX = 2 * Step, RecentY = 2 * Step;
	for (int32 i = 0; i < InnerSize; i++) {
		for (int32 j = 0; j < InnerSize; j++) {
			float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
			float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
			float LocationZ = 0.0f;
			if(bContainSediment)
			{
				LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			}
			else
			{
				LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
			}
			Positions.Add(FVector(LocationX, LocationY, LocationZ));
			FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
			TexCoords.Add(UV);
			if(bContainSediment)
			{
				float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
				if(SedimentValue > 1.0f)
				{
					Colors.Add(SedimentColor);
				}
				else
				{
					Colors.Add(BasicColor);
				}
			}
			RecentY += Step;
		}
		RecentX += Step;
		RecentY = 2 * Step;
	}
	/*
	 * 中圈的点
	 */
	RecentX = Step, RecentY = Step;
	for(int32 i = 0; i < MediumSize - 1; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(bContainSediment)
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		if(bContainSediment)
		{
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
		}
		RecentY += Step;
	}
	for(int32 i = 0; i < MediumSize - 1; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(bContainSediment)
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		if(bContainSediment)
		{
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
		}
		RecentX += Step;
	}
	for(int32 i = 0; i < MediumSize - 1; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(bContainSediment)
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		if(bContainSediment)
		{
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
		}
		RecentY -= Step;
	}
	for(int32 i = 0; i < MediumSize - 1; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(bContainSediment)
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		if(bContainSediment)
		{
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
		}
		RecentX -= Step;
	}
	/*
	 * 外圈的点
	 */
	Step = ChunkData->GetStepOfLODLevel(ELODLevel::LOD_HIGH);
	RecentX = 0, RecentY = 0;
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(bContainSediment)
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		if(bContainSediment)
		{
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
		}
		RecentY += Step;
	}
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(bContainSediment)
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		if(bContainSediment)
		{
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
		}
		RecentX += Step;
	}
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(bContainSediment)
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		if(bContainSediment)
		{
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
		}
		RecentY -= Step;
	}
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(bContainSediment)
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		if(bContainSediment)
		{
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
		}
		RecentX -= Step;
	}
}

void AHITerrainActor::GenerateChunk2(TArray<FVector>& Normals, TArray<FRuntimeMeshTangent>& Tangents,
	TArray<FColor>& Colors, ELODLevel LODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(LODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(LODLevel);
	int32 OuterSize = ChunkData->GetOuterPointSize(LODLevel);
	bool bContainSediment = ChunkData->Data->ContainsChannel("sediment");
	/*
	 * 内部的点
	 */
	for (int32 i = 0; i < InnerSize; i++) {
		for (int32 j = 0; j < InnerSize; j++) {
			Normals.Add(FVector(0, 0, 1));
			Tangents.Add(FRuntimeMeshTangent(1, 0, 0));
			if(!bContainSediment)
			{
				Colors.Add(FColor(127, 127, 127, 255));
			}
		}
	}
	/*
	 * 中圈的点
	 */
	for(int32 i = 0; i < (MediumSize - 1) * 4; i++)
	{
		Normals.Add(FVector(0, 0, 1));
		Tangents.Add(FRuntimeMeshTangent(1, 0, 0));
		if(!bContainSediment)
		{
			Colors.Add(FColor(127, 127, 127, 255));
		}
	}
	/*
	 * 外圈的点
	 */
	for(int32 i = 0; i < (OuterSize - 1) * 4; i++)
	{
		Normals.Add(FVector(0, 0, 1));
		Tangents.Add(FRuntimeMeshTangent(1, 0, 0));
		if(!bContainSediment)
		{
			Colors.Add(FColor(127, 127, 127, 255));
		}
	}
}

void AHITerrainActor::GenerateChunk3(TArray<int32>& Triangles, ELODLevel LODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(LODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(LODLevel);
	int32 OuterSize = ChunkData->GetOuterPointSize(LODLevel);
	/*
	 * 内部的三角面
	 */
	int32 Vertice1 = 0;
	int32 Vertice2 = 1;
	int32 Vertice3 = InnerSize;
	int32 Vertice4 = InnerSize + 1;
	for (int32 i = 0; i < InnerSize - 1; i++) {
		for (int32 j = 0; j < InnerSize - 1; j++) {
			Triangles.Add(Vertice1);	Triangles.Add(Vertice2);	Triangles.Add(Vertice4);
			Triangles.Add(Vertice4);	Triangles.Add(Vertice3);	Triangles.Add(Vertice1);
			Vertice1++;	Vertice2++;	Vertice3++;	Vertice4++;
		}
		Vertice1++;	Vertice2++;	Vertice3++;	Vertice4++;
	}
	/*
	* 中圈的三角面
	*/
	Vertice1 = 0;
	Vertice2 = 1;
	Vertice3 = InnerSize * InnerSize;
	Vertice4 = InnerSize * InnerSize + 1;
	for(int32 i = 0; i < InnerSize - 1; i++)
	{
		Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
		Triangles.Add(Vertice4);	Triangles.Add(Vertice2);	Triangles.Add(Vertice1);
		Vertice1++;	Vertice2++;	Vertice3++;	Vertice4++;
	}
	Vertice2 += InnerSize - 1;
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
	Vertice3++;	Vertice4++;
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
	Vertice3++; Vertice4++;
	for(int32 i = 0; i < InnerSize - 1; i++)
	{
		Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
		Triangles.Add(Vertice4);	Triangles.Add(Vertice2);	Triangles.Add(Vertice1);
		Vertice1 += InnerSize;	Vertice2 += InnerSize;	Vertice3++;	Vertice4++;
	}
	Vertice2 -= InnerSize + 1;
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
	Vertice3++;	Vertice4++;
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
	Vertice3++; Vertice4++;
	for(int32 i = 0; i < InnerSize - 1; i++)
	{
		Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
		Triangles.Add(Vertice4);	Triangles.Add(Vertice2);	Triangles.Add(Vertice1);
		Vertice1 -= 1;	Vertice2 -= 1;	Vertice3++;	Vertice4++;
	}
	Vertice2 -= InnerSize - 1;
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
	Vertice3++;	Vertice4++;
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
	Vertice3++; Vertice4++;
	for(int32 i = 0; i < InnerSize - 1; i++)
	{
		Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
		Triangles.Add(Vertice4);	Triangles.Add(Vertice2);	Triangles.Add(Vertice1);
		Vertice1 -= InnerSize;	Vertice2 -= InnerSize;	Vertice3++;	Vertice4++;
	}
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
	Vertice3++;	Vertice4 = InnerSize * InnerSize;
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
	/*
	 * 外圈的三角面
	 */
	int32 OuterScale = ChunkData->GetOuterPointScale(LODLevel);
	Vertice1 = InnerSize * InnerSize;
	Vertice2 = InnerSize * InnerSize + 1;
	Vertice3 = InnerSize * InnerSize + (InnerSize + 1) * 4;
	Vertice4 = InnerSize * InnerSize + (InnerSize + 1) * 4 + 1;
	for(int32 OuterLoop = 0; OuterLoop < 4; OuterLoop++)
	{
		for(int32 i = 0; i < OuterScale / 2; i++)
		{
			Triangles.Add(Vertice1);	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);
			Vertice3++;	Vertice4++;
		}
		for(int32 i = 0; i < MediumSize; i++)
		{
			if(Vertice1 >= MediumSize * MediumSize)
			{
				Vertice1 = InnerSize * InnerSize;
			}
			if(Vertice2 >= MediumSize * MediumSize)
			{
				Vertice2 = InnerSize * InnerSize;
			}
			for(int32 j = 0; j < OuterScale; j++)
			{
				Triangles.Add(Vertice1);	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);
				Vertice3++;	Vertice4++;
			}
			if(i == MediumSize - 1)
			{
				for(int32 k = 0; k < (OuterScale - 1) / 2 + 1; k++)
				{
					if(Vertice4 >= MediumSize * MediumSize + (OuterSize - 1) * 4)
					{
						Vertice4 = InnerSize * InnerSize + (InnerSize + 1) * 4;;
					}
					Triangles.Add(Vertice1);	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);
					Vertice3++;	Vertice4++;
				}
			}
			else
			{
				Triangles.Add(Vertice1);	Triangles.Add(Vertice3);	Triangles.Add(Vertice2);
				Vertice1 ++; Vertice2++;
			}
		}
	}
}


void AHITerrainActor::GeneratePositions(TArray<FVector>& Positions, ELODLevel LODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(LODLevel);
	float Step = ChunkData->GetStepOfLODLevel(LODLevel);
	float RecentX = 2 * Step, RecentY = 2 * Step;
	for (int32 i = 0; i <= InnerSize; i++) {
		for (int32 j = 0; j <= InnerSize; j++) {
			float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
			float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
			float LocationZ = 0.0f;
			if(ChunkData->Data->ContainsChannel("sediment"))
			{
				LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			}
			else
			{
				LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
			}
			Positions.Add(FVector(LocationX, LocationY, LocationZ));
			RecentY += Step;
		}
		RecentX += Step;
		RecentY = 2 * Step;
	}
	int32 MediumSize = ChunkData->GetMediumPointSize(LODLevel);
	RecentX = Step;
	RecentY = Step;
	for(int32 i = 0; i < MediumSize; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(ChunkData->Data->ContainsChannel("sediment"))
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		RecentY += Step;
	}
	RecentY += Step;
	for(int32 i = 0; i < MediumSize; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(ChunkData->Data->ContainsChannel("sediment"))
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		RecentX += Step;
	}
	RecentX += Step;
	for(int32 i = 0; i < MediumSize; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(ChunkData->Data->ContainsChannel("sediment"))
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		RecentY -= Step;
	}
	RecentY -= Step;
	for(int32 i = 0; i < MediumSize; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		float LocationZ = 0.0f;
		if(ChunkData->Data->ContainsChannel("sediment"))
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
		}
		else
		{
			LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
		}
		Positions.Add(FVector(LocationX, LocationY, LocationZ));
		RecentX -= Step;
	}
}

void AHITerrainActor::GenerateTriangles(TArray<int32>& Triangles, ELODLevel LODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(LODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(LODLevel);
	int32 Vertice1 = 0;
	int32 Vertice2 = 1;
	int32 Vertice3 = InnerSize + 1;
	int32 Vertice4 = InnerSize + 2;
	for (int32 i = 0; i < InnerSize; i++) {
		for (int32 j = 0; j < InnerSize; j++) {
			Triangles.Add(Vertice1);	Triangles.Add(Vertice2);	Triangles.Add(Vertice4);
			Triangles.Add(Vertice4);	Triangles.Add(Vertice3);	Triangles.Add(Vertice1);
			Vertice1++;	Vertice2++;	Vertice3++;	Vertice4++;
		}
		Vertice1++;	Vertice2++;	Vertice3++;	Vertice4++;
	}
	Vertice1 = 0;
	Vertice2 = 1;
	Vertice3 = (InnerSize + 1) * (InnerSize + 1) + 1;
	Vertice4 = (InnerSize + 1) * (InnerSize + 1) + 2;
	for(int32 i = 0; i < InnerSize - 1; i++)
	{
		Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
		Triangles.Add(Vertice4);	Triangles.Add(Vertice2);	Triangles.Add(Vertice1);
		Vertice1++;	Vertice2++;	Vertice3++;	Vertice4++;
	}
	Vertice1++;	Vertice3++;	Vertice4++;
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
	Vertice3++; Vertice4++; Vertice2 += InnerSize;
	for(int32 i = 0; i < InnerSize - 1; i++)
	{
		Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
		Triangles.Add(Vertice4);	Triangles.Add(Vertice2);	Triangles.Add(Vertice1);
		Vertice1 += InnerSize;	Vertice2 += InnerSize;	Vertice3++;	Vertice4++;
	}
	Vertice1 += InnerSize;	Vertice3++;	Vertice4++;
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
	Vertice3++; Vertice4++; Vertice2 -= 1;
	for(int32 i = 0; i < InnerSize - 1; i++)
	{
		Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
		Triangles.Add(Vertice4);	Triangles.Add(Vertice2);	Triangles.Add(Vertice1);
		Vertice1 -= 1;	Vertice2 -= 1;	Vertice3++;	Vertice4++;
	}
	Vertice1 -= 1;	Vertice3++;	Vertice4++;
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
	Vertice3++; Vertice4++; Vertice2 -= InnerSize;
	for(int32 i = 0; i < InnerSize - 1; i++)
	{
		Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
		Triangles.Add(Vertice4);	Triangles.Add(Vertice2);	Triangles.Add(Vertice1);
		Vertice1 -= InnerSize;	Vertice2 -= InnerSize;	Vertice3++;	Vertice4++;
	}
	Vertice1 -= InnerSize;	Vertice3++;	Vertice4 = (InnerSize + 1) * (InnerSize + 1) + 1;
	Triangles.Add(Vertice3);	Triangles.Add(Vertice4);	Triangles.Add(Vertice1);
}

void AHITerrainActor::GenerateNormals(TArray<FVector>& Normals, ELODLevel LODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(LODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(LODLevel);
	for (int32 i = 0; i <= InnerSize; i++) {
		for (int32 j = 0; j <= InnerSize; j++) {
			Normals.Add(FVector(0, 0, 1));
		}
	}
	for(int32 i = 0; i < MediumSize * 4; i++)
	{
		Normals.Add(FVector(0, 0, 1));
	}
}

void AHITerrainActor::GenerateTangents(TArray<FRuntimeMeshTangent>& Tangents, ELODLevel LODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(LODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(LODLevel);
	for (int32 i = 0; i <= InnerSize; i++) {
		for (int32 j = 0; j <= InnerSize; j++) {
			Tangents.Add(FRuntimeMeshTangent(1, 0, 0));
		}
	}
	for(int32 i = 0; i < MediumSize * 4; i++)
	{
		Tangents.Add(FRuntimeMeshTangent(1, 0, 0));
	}
}

void AHITerrainActor::GenerateTexCoords(TArray<FVector2D>& TexCoords, ELODLevel LODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(LODLevel);
	float Step = ChunkData->GetStepOfLODLevel(LODLevel);
	float RecentX = 2 * Step, RecentY = 2 * Step;
	for (int32 i = 0; i <= InnerSize; i++) {
		for (int32 j = 0; j <= InnerSize; j++) {
			float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
			float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
			FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
			TexCoords.Add(UV);
			RecentY += Step;
		}
		RecentX += Step;
		RecentY = 2 * Step;
	}
	int32 MediumSize = ChunkData->GetMediumPointSize(LODLevel);
	RecentX = Step;
	RecentY = Step;
	for(int32 i = 0; i < MediumSize; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		RecentY += Step;
	}
	RecentY += Step;
	for(int32 i = 0; i < MediumSize; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		RecentX += Step;
	}
	RecentX += Step;
	for(int32 i = 0; i < MediumSize; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		RecentY -= Step;
	}
	RecentY -= Step;
	for(int32 i = 0; i < MediumSize; i++)
	{
		float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
		float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
		FVector2D UV = ChunkData->GetUV(LocationX, LocationY);
		TexCoords.Add(UV);
		RecentX -= Step;
	}
}

void AHITerrainActor::GenerateColors(TArray<FColor>& Colors, ELODLevel LODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(LODLevel);
	FColor BasicColor(127, 127, 127, 255);
	FColor SedimentColor(0, 255, 0, 255);
	if(ChunkData->Data->ContainsChannel("sediment"))
	{
		for (int32 i = 0; i <= InnerSize; i++) {
			for (int32 j = 0; j <= InnerSize; j++) {
				float SedimentValue = ChunkData->GetChannelFloatValue("sediment", i, j);
				if(SedimentValue > 1.0f)
				{
					Colors.Add(SedimentColor);
				}
				else
				{
					Colors.Add(BasicColor);
				}
			}
		}
		int32 MediumSize = ChunkData->GetMediumPointSize(LODLevel);
		float Step = ChunkData->GetStepOfLODLevel(LODLevel);
		float RecentX = Step;
		float RecentY = Step;
		for(int32 i = 0; i < MediumSize; i++)
		{
			float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
			float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
			RecentY += Step;
		}
		RecentY += Step;
		for(int32 i = 0; i < MediumSize; i++)
		{
			float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
			float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
			RecentX += Step;
		}
		RecentX += Step;
		for(int32 i = 0; i < MediumSize; i++)
		{
			float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
			float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
			RecentY -= Step;
		}
		RecentY -= Step;
		for(int32 i = 0; i < MediumSize; i++)
		{
			float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
			float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
			float SedimentValue = ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
			if(SedimentValue > 1.0f)
			{
				Colors.Add(SedimentColor);
			}
			else
			{
				Colors.Add(BasicColor);
			}
			RecentX -= Step;
		}
	}
	else
	{
		for (int32 i = 0; i <= InnerSize; i++) {
			for (int32 j = 0; j <= InnerSize; j++) {
				Colors.Add(FColor(127, 127, 127, 255));
			}
		}
		int32 MediumSize = ChunkData->GetMediumPointSize(LODLevel);
		for(int32 i = 0; i < MediumSize * 4; i++)
		{
			Colors.Add(FColor(127, 127, 127, 255));
		}
	}
}

void AHITerrainActor::GenerateWater(ELODLevel LODLevel)
{
	if (StaticProvider){
		StaticProvider->SetupMaterialSlot(1, TEXT("Material"), WaterMaterial);
		TArray<FVector> Positions;
		TArray<FColor> Colors;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> TexCoords;
		TArray<FRuntimeMeshTangent> Tangents;

		GenerateWaterPositions(Positions, LODLevel);
		GenerateWaterTriangles(Triangles, LODLevel);
		GenerateWaterNormals(Normals, LODLevel);
		GenerateWaterTexCoords(TexCoords, LODLevel);
		GenerateWaterTangents(Tangents, LODLevel);
		GenerateWaterColors(Colors, LODLevel);
		if(bWaterGenerated)
		{
			StaticProvider->UpdateSectionFromComponents(0, 1, Positions, Triangles, Normals, TexCoords, Colors, Tangents);
		}
		else
		{
			StaticProvider->CreateSectionFromComponents(0, 1, 1, Positions, Triangles, Normals, TexCoords, Colors, Tangents, ERuntimeMeshUpdateFrequency::Frequent, false);
			bWaterGenerated = true;
		}
		
	}
}

void AHITerrainActor::GenerateWaterPositions(TArray<FVector>& Positions, ELODLevel LODLevel)
{
	float RecentX = 0, RecentY = 0;
	// TODO 陆地那边测试完再改水
	float Step = ChunkData->GetStepOfLODLevel(ELODLevel::NONE);
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
			float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
			// 这里在LOD下会有BUG，待解决
			// float LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", i, j) + ChunkData->GetChannelFloatValue("water", i, j);
			float LocationZ = ChunkData->GetHeightValue(LocationX, LocationY) + ChunkData->GetChannelFloatValue("sediment", i, j) + ChunkData->GetChannelFloatValue("water", i, j) - 0.1f;
			Positions.Add(FVector(LocationX, LocationY, LocationZ));
			RecentY += Step;
		}
		RecentX += Step;
		RecentY = 0.0f;
	}
}

void AHITerrainActor::GenerateWaterTriangles(TArray<int32>& Triangles, ELODLevel LODLevel)
{
	int32 Vertice1 = 0;
	int32 Vertice2 = 1;
	int32 Vertice3 = Size + 1;
	int32 Vertice4 = Size + 2;
	for (int32 i = 0; i < Size; i++) {
		for (int32 j = 0; j < Size; j++) {
			Triangles.Add(Vertice1);
			Triangles.Add(Vertice2);
			Triangles.Add(Vertice4);
			Triangles.Add(Vertice4);
			Triangles.Add(Vertice3);
			Triangles.Add(Vertice1);
			Vertice1++;
			Vertice2++;
			Vertice3++;
			Vertice4++;
		}
		Vertice1++;
		Vertice2++;
		Vertice3++;
		Vertice4++;
	}
}

void AHITerrainActor::GenerateWaterNormals(TArray<FVector>& Normals, ELODLevel LODLevel)
{
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			Normals.Add(FVector(0, 0, 1));
		}
	}
}

void AHITerrainActor::GenerateWaterTangents(TArray<FRuntimeMeshTangent>& Tangents, ELODLevel LODLevel)
{
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			Tangents.Add(FRuntimeMeshTangent(1, 0, 0));
		}
	}
}

void AHITerrainActor::GenerateWaterTexCoords(TArray<FVector2D>& TexCoords, ELODLevel LODLevel)
{
	// float UVStep = 1.0 / Size;
	// float RecentX = 0, RecentY = 0;
	// for (int32 i = 0; i <= Size; i++)
	// {
	// 	for (int32 j = 0; j <= Size; j++)
	// 	{
	// 		TexCoords.Add(FVector2D(RecentX, RecentY));
	// 		RecentY += UVStep;
	// 	}
	// 	RecentX += UVStep;
	// 	RecentY = 0.0;
	// }
	// 1个格子一个uv的实现
	bool bEvenX = false, bEvenY = false;
	float RecentX = 0, RecentY = 0;
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			if(bEvenX && bEvenY){
				TexCoords.Add(FVector2D(1, 1));
			}
			if (bEvenX && !bEvenY) {
				TexCoords.Add(FVector2D(1, 0));
			}
			if (!bEvenX && bEvenY) {
				TexCoords.Add(FVector2D(0, 1));
			}
			if (!bEvenX && !bEvenY) {
				TexCoords.Add(FVector2D(0, 0));
			}
			bEvenX = !bEvenX;
		}
		bEvenY = !bEvenY;
		bEvenX = false;
	}
}

void AHITerrainActor::GenerateWaterColors(TArray<FColor>& Colors, ELODLevel LODLevel)
{
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			Colors.Add(FColor(212, 241, 249, 127));
		}
	}
}
