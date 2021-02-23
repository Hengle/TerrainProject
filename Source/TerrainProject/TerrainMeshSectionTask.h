// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "TerrainActor.h"
#include "TerrainUtils.h"
#include "TerrainDataManager.h"

class TERRAINPROJECT_API FTerrainMeshSectionTask
{

public:
	FTerrainMeshSectionTask(UProceduralMeshComponent* ProceduralMesh, FTerrainMeshSectionParameter param);

	void CreateTerrainMeshSection();

private:
	UProceduralMeshComponent* ProceduralMeshComponent;

	FTerrainMeshSectionParameter Param;

	TerrainDataManager& DataManager;

	noise::module::Perlin PerlinNoise;


private:
	void CreateVertices(TArray<FVector>* Vectices);

	void CreateTriangles(TArray<int32>*);

	void CreateNormals(TArray<FVector>*);

	void CreateUVs(TArray<FVector2D>*);

	void CreateTangents(TArray<FProcMeshTangent>*);

	void CreateColors(TArray<FLinearColor>*);
};
