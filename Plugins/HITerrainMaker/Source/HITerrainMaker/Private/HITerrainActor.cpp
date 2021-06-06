#include "HITerrainActor.h"
#include "HITerrainFoliage.h"
#include "Engine/StaticMeshActor.h"
#include "TerrainDatas/HITerrainChunkData.h"

void AHITerrainActor::Initialize(UHITerrainData* Data, FTerrainInformationPtr InTerrainInformation, const TPair<int32, int32>& InIndex)
{
	Index = InIndex;
	ChunkData = Data->GetChunkData(Index);
	TerrainInformation = InTerrainInformation;
	ProceduralMesh = NewObject<UProceduralMeshComponent>(this, UProceduralMeshComponent::StaticClass());
	ProceduralMesh->RegisterComponent();
}

void AHITerrainActor::DeleteChunk()
{
	ProceduralMesh->ClearMeshSection(0);
	ProceduralMesh->ClearMeshSection(1);
	bGenerated = false;
	bWaterGenerated = false;
}

void AHITerrainActor::GenerateChunk(ELODLevel InLODLevel)
{
	LODLevel = InLODLevel;
	ProceduralMesh->SetMaterial(0, Material);
	TArray<FVector> Positions;
	TArray<FLinearColor> Colors;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> TexCoords;
	TArray<FProcMeshTangent> Tangents;
	
	GenerateChunk1(Positions, TexCoords, Colors, InLODLevel);
	GenerateChunk2(Normals, Tangents, Colors, InLODLevel);
	GenerateChunk3(Triangles, InLODLevel);
	if(bGenerated) 
	{
		ProceduralMesh->ClearMeshSection(0);
		
	}
	ProceduralMesh->CreateMeshSection_LinearColor(0, Positions, Triangles, Normals, TexCoords, Colors, Tangents, true);
	bGenerated = true;

	if(ChunkData->Data->ContainsChannel("water"))
	{
		GenerateWater(InLODLevel);
	}
	GenerateVegetation(InLODLevel);
}

const ELODLevel& AHITerrainActor::GetLODLevel()
{
	return LODLevel;
}

bool AHITerrainActor::IsGenerated()
{
	return bGenerated;
}

void AHITerrainActor::GenerateChunk1(TArray<FVector>& Positions, TArray<FVector2D>& TexCoords, TArray<FLinearColor>& Colors,
	ELODLevel InLODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(InLODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(InLODLevel);
	int32 OuterSize = ChunkData->GetOuterPointSize(InLODLevel);
	float Step = ChunkData->GetStepOfLODLevel(InLODLevel);
	/*
	 * 内部的点
	 */
	float RecentX = 2 * Step, RecentY = 2 * Step;
	for (int32 i = 0; i < InnerSize; i++) {
		for (int32 j = 0; j < InnerSize; j++) {
			GeneratePointData(Positions, TexCoords, Colors, RecentX, RecentY);
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
		GeneratePointData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentY += Step;
	}
	for(int32 i = 0; i < MediumSize - 1; i++)
	{
		GeneratePointData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentX += Step;
	}
	for(int32 i = 0; i < MediumSize - 1; i++)
	{
		GeneratePointData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentY -= Step;
	}
	for(int32 i = 0; i < MediumSize - 1; i++)
	{
		GeneratePointData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentX -= Step;
	}
	/*
	 * 外圈的点
	 */
	Step = ChunkData->GetStepOfLODLevel(ELODLevel::LOD_HIGH);
	RecentX = 0, RecentY = 0;
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		GeneratePointData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentY += Step;
	}
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		GeneratePointData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentX += Step;
	}
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		GeneratePointData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentY -= Step;
	}
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		GeneratePointData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentX -= Step;
	}
}

void AHITerrainActor::GeneratePointData(TArray<FVector>& Positions, TArray<FVector2D>& TexCoords,
	TArray<FLinearColor>& Colors, float RecentX, float RecentY)
{
	bool bContainSediment = ChunkData->Data->ContainsChannel("sediment");
	FLinearColor PointColor;
	float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
	float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
	float LocationZ = 0.0f;
	if(bContainSediment)
	{
		LocationZ = ChunkData->GetHeightValue(LocationX, LocationY)
				  + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY);
	}
	else
	{
		LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
	}
	FVector Location(LocationX, LocationY, LocationZ);
	Positions.Add(Location);
	FVector2D UV = ChunkData->GetUV(LocationX, LocationY, 3);
	TexCoords.Add(UV);
	if(bContainSediment)
	{
		// float SedimentValue = ChunkData->GetChannelFloatValue("slope", LocationX, LocationY);
		// PointColor.R = FMath::Clamp(SedimentValue, 0.0f, 1.0f);
		PointColor.R = ChunkData->GetChannelFloatValue("r", LocationX, LocationY);
		PointColor.G = ChunkData->GetChannelFloatValue("g", LocationX, LocationY);
		PointColor.B = ChunkData->GetChannelFloatValue("b", LocationX, LocationY);
		PointColor.A = ChunkData->GetChannelFloatValue("a", LocationX, LocationY);
	}
	Colors.Add(PointColor);
}

void AHITerrainActor::GenerateChunk2(TArray<FVector>& Normals, TArray<FProcMeshTangent>& Tangents,
                                     TArray<FLinearColor>& Colors, ELODLevel InLODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(InLODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(InLODLevel);
	int32 OuterSize = ChunkData->GetOuterPointSize(InLODLevel);
	bool bContainSediment = ChunkData->Data->ContainsChannel("sediment");
	/*
	 * 内部的点
	 */
	for (int32 i = 0; i < InnerSize; i++) {
		for (int32 j = 0; j < InnerSize; j++) {
			Normals.Add(FVector(0, 0, 1));
			Tangents.Add(FProcMeshTangent(1, 0, 0));
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
		Tangents.Add(FProcMeshTangent(1, 0, 0));
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
		Tangents.Add(FProcMeshTangent(1, 0, 0));
		if(!bContainSediment)
		{
			Colors.Add(FColor(127, 127, 127, 255));
		}
	}
}

void AHITerrainActor::GenerateChunk3(TArray<int32>& Triangles, ELODLevel InLODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(InLODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(InLODLevel);
	int32 OuterSize = ChunkData->GetOuterPointSize(InLODLevel);
	/*
	 * 内部的三角面
	 * 这一部分比较单纯，就是内圈的方形区域。
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
	* 这一部分比较麻烦，因为外圈始终是最高LOD等级的，因此做一个中圈方便外圈那边写代码。
	* for循环是在四个方向依次连三角形，for循环外面的是单独补的缺失的部分。
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
	 * 有了中圈的部分，这一部分就好写很多了。
	 */
	int32 OuterScale = ChunkData->GetOuterPointScale(InLODLevel);
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

void AHITerrainActor::GenerateWater(ELODLevel InLODLevel)
{
	ProceduralMesh->SetMaterial(1, WaterMaterial);
	TArray<FVector> Positions;
	TArray<FLinearColor> Colors;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> TexCoords;
	TArray<FProcMeshTangent> Tangents;
	
	GenerateWater1(Positions, TexCoords, Colors, InLODLevel);
	GenerateWater2(Normals, Tangents, Colors, InLODLevel);
	GenerateWater3(Triangles, InLODLevel);
	if(bWaterGenerated)
	{
		ProceduralMesh->ClearMeshSection(1);
	}
	ProceduralMesh->CreateMeshSection_LinearColor(1, Positions, Triangles, Normals, TexCoords, Colors, Tangents, false);
	bWaterGenerated = true;
}

void AHITerrainActor::GenerateWater1(TArray<FVector>& Positions, TArray<FVector2D>& TexCoords,
	TArray<FLinearColor>& Colors, ELODLevel InLODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(InLODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(InLODLevel);
	int32 OuterSize = ChunkData->GetOuterPointSize(InLODLevel);
	float Step = ChunkData->GetStepOfLODLevel(InLODLevel);
	/*
	* 内部的点
	*/
	float RecentX = 2 * Step, RecentY = 2 * Step;
	for (int32 i = 0; i < InnerSize; i++) {
		for (int32 j = 0; j < InnerSize; j++) {
			GeneratePointWaterData(Positions, TexCoords, Colors, RecentX, RecentY);
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
		GeneratePointWaterData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentY += Step;
	}
	for(int32 i = 0; i < MediumSize - 1; i++)
	{
		GeneratePointWaterData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentX += Step;
	}
	for(int32 i = 0; i < MediumSize - 1; i++)
	{
		GeneratePointWaterData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentY -= Step;
	}
	for(int32 i = 0; i < MediumSize - 1; i++)
	{
		GeneratePointWaterData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentX -= Step;
	}
	/*
	* 外圈的点
	*/
	Step = ChunkData->GetStepOfLODLevel(ELODLevel::LOD_HIGH);
	RecentX = 0, RecentY = 0;
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		GeneratePointWaterData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentY += Step;
	}
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		GeneratePointWaterData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentX += Step;
	}
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		GeneratePointWaterData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentY -= Step;
	}
	for(int32 i = 0; i < OuterSize - 1; i++)
	{
		GeneratePointWaterData(Positions, TexCoords, Colors, RecentX, RecentY);
		RecentX -= Step;
	}
}

void AHITerrainActor::GeneratePointWaterData(TArray<FVector>& Positions, TArray<FVector2D>& TexCoords,
	TArray<FLinearColor>& Colors, float RecentX, float RecentY)
{
	FLinearColor PointColor;
	float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
	float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
	float LocationZ = 0.0f;
	LocationZ = ChunkData->GetHeightValue(LocationX, LocationY)
			  + ChunkData->GetChannelFloatValue("sediment", LocationX, LocationY)
			  + ChunkData->GetChannelFloatValue("water", LocationX, LocationY) - 1.0f;
	FVector Location(LocationX, LocationY, LocationZ);
	Positions.Add(Location);
	FVector2D UV = ChunkData->GetUV(LocationX, LocationY, 3);
	TexCoords.Add(UV);
	Colors.Add(PointColor);
}

void AHITerrainActor::GenerateWater2(TArray<FVector>& Normals, TArray<FProcMeshTangent>& Tangents,
	TArray<FLinearColor>& Colors, ELODLevel InLODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(InLODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(InLODLevel);
	int32 OuterSize = ChunkData->GetOuterPointSize(InLODLevel);
	/*
	* 内部的点
	*/
	for (int32 i = 0; i < InnerSize; i++) {
		for (int32 j = 0; j < InnerSize; j++) {
			Normals.Add(FVector(0, 0, 1));
			Tangents.Add(FProcMeshTangent(1, 0, 0));
		}
	}
	/*
	* 中圈的点
	*/
	for(int32 i = 0; i < (MediumSize - 1) * 4; i++)
	{
		Normals.Add(FVector(0, 0, 1));
		Tangents.Add(FProcMeshTangent(1, 0, 0));
	}
	/*
	* 外圈的点
	*/
	for(int32 i = 0; i < (OuterSize - 1) * 4; i++)
	{
		Normals.Add(FVector(0, 0, 1));
		Tangents.Add(FProcMeshTangent(1, 0, 0));
	}
}

void AHITerrainActor::GenerateWater3(TArray<int32>& Triangles, ELODLevel InLODLevel)
{
	int32 InnerSize = ChunkData->GetInnerPointSize(InLODLevel);
	int32 MediumSize = ChunkData->GetMediumPointSize(InLODLevel);
	int32 OuterSize = ChunkData->GetOuterPointSize(InLODLevel);
	/*
	 * 内部的三角面
	 * 这一部分比较单纯，就是内圈的方形区域。
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
	* 这一部分比较麻烦，因为外圈始终是最高LOD等级的，因此做一个中圈方便外圈那边写代码。
	* for循环是在四个方向依次连三角形，for循环外面的是单独补的缺失的部分。
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
	 * 有了中圈的部分，这一部分就好写很多了。
	 */
	int32 OuterScale = ChunkData->GetOuterPointScale(InLODLevel);
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


void AHITerrainActor::GenerateWaterPositions(TArray<FVector>& Positions, ELODLevel InLODLevel)
{
	float RecentX = 0, RecentY = 0;
	int32 Size = ChunkData->GetPointSize(InLODLevel);
	float Step = ChunkData->GetStepOfLODLevel(InLODLevel);
	for (int32 i = 0; i < Size; i++) {
		for (int32 j = 0; j < Size; j++) {
			float LocationX = ChunkData->GetChunkSize() * Index.Key + RecentX;
			float LocationY = ChunkData->GetChunkSize() * Index.Value + RecentY;
			float LocationZ = ChunkData->GetHeightValue(LocationX, LocationY)
							+ ChunkData->Data->GetSedimentValue(LocationX, LocationY)
							+ ChunkData->Data->GetWaterValue(LocationX, LocationY) - 0.1f;
			Positions.Add(FVector(LocationX, LocationY, LocationZ));
			RecentY += Step;
		}
		RecentX += Step;
		RecentY = 0.0f;
	}
}

void AHITerrainActor::GenerateWaterTriangles(TArray<int32>& Triangles, ELODLevel InLODLevel)
{
	int32 Size = ChunkData->GetPointSize(InLODLevel) - 1;
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

void AHITerrainActor::GenerateWaterNormals(TArray<FVector>& Normals, ELODLevel InLODLevel)
{
	int32 Size = ChunkData->GetPointSize(InLODLevel);
	for (int32 i = 0; i < Size; i++) {
		for (int32 j = 0; j < Size; j++) {
			Normals.Add(FVector(0, 0, 1));
		}
	}
}

void AHITerrainActor::GenerateWaterTangents(TArray<FRuntimeMeshTangent>& Tangents, ELODLevel InLODLevel)
{
	int32 Size = ChunkData->GetPointSize(InLODLevel);
	for (int32 i = 0; i < Size; i++) {
		for (int32 j = 0; j < Size; j++) {
			Tangents.Add(FRuntimeMeshTangent(1, 0, 0));
		}
	}
}

void AHITerrainActor::GenerateWaterTexCoords(TArray<FVector2D>& TexCoords, ELODLevel InLODLevel)
{
	int32 Size = ChunkData->GetPointSize(InLODLevel);
	bool bEvenX = false, bEvenY = false;
	float RecentX = 0, RecentY = 0;
	for (int32 i = 0; i < Size; i++) {
		for (int32 j = 0; j < Size; j++) {
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

void AHITerrainActor::GenerateWaterColors(TArray<FLinearColor>& Colors, ELODLevel InLODLevel)
{
	int32 Size = ChunkData->GetPointSize(InLODLevel);
	for (int32 i = 0; i < Size; i++) {
		for (int32 j = 0; j < Size; j++) {
			Colors.Add(FColor(212, 241, 249, 127));
		}
	}
}

void AHITerrainActor::GenerateVegetation(ELODLevel InLODLevel)
{
	if(InLODLevel == ELODLevel::NONE || InLODLevel == ELODLevel::LOD_LOW)
	{
		for(AStaticMeshActor* StaticMeshActor: StaticMeshActors)
		{
			StaticMeshActor->Destroy();
		}
		StaticMeshActors.Empty();
	}
	else
	{
		UHITerrainFoliage* FoliageManager = UHITerrainFoliage::Get();
		if(StaticMeshActors.Num() == 0)
		{
			TArray<FFoliageData> FoliageDatas = ChunkData->GetChunkFoliage();
			for(const FFoliageData& FoliageData: FoliageDatas)
			{
				AStaticMeshActor* FoliageActor = (AStaticMeshActor*)GetWorld()->SpawnActor(AStaticMeshActor::StaticClass(), &FoliageData.Location, &FoliageData.Rotation);
				FoliageActor->SetMobility(EComponentMobility::Movable);
				FoliageActor->GetStaticMeshComponent()->SetStaticMesh(FoliageManager->GetFoliageOfType(FoliageData.Type));
				FoliageActor->SetMobility(EComponentMobility::Static);
				StaticMeshActors.Add(FoliageActor);
			}
		}
	}
}

void AHITerrainActor::Destroyed()
{
	for(auto StaticMesh: StaticMeshActors)
	{
		StaticMesh->Destroy();
	}
}
