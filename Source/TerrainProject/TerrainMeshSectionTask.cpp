// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMeshSectionTask.h"

/**
 * 创建一片地形
 */
void FTerrainMeshSectionTask::CreateTerrainMeshSection() {
	TArray<FVector> Vertices;
	CreateVertices(&Vertices);

	TArray<int32> Triangles;
	CreateTriangles(&Triangles);

	TArray<FVector> Normals;
	CreateNormals(&Normals);

	TArray<FVector2D> UV0;
	CreateUVs(&UV0);

	TArray<FProcMeshTangent> Tangents;
	CreateTangents(&Tangents);

	TArray<FLinearColor> VertexColors;
	CreateColors(&VertexColors);

	ProceduralMeshComponent->CreateMeshSection_LinearColor(Param.SectionIndex, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);
	UE_LOG(LogTemp, Warning, TEXT("Terrain Section %d Builded! X:%d, Y:%d"), Param.SectionIndex, Param.XIndex, Param.YIndex)
}


/**
 * 创建地形顶点
 */
void FTerrainMeshSectionTask::CreateVertices(TArray<FVector>* Vectices) {
	float RecentX = 0, RecentY = 0;
	for (int32 i = 0; i <= Param.TaskParam.SizeX; i++) {
		for (int32 j = 0; j <= Param.TaskParam.SizeY; j++) {
			float LocationX = Param.TaskParam.SizeX * Param.TaskParam.StepX * Param.XIndex + RecentX;
			float LocationY = Param.TaskParam.SizeY * Param.TaskParam.StepY * Param.YIndex + RecentY;
			float LocationZ = DataManager.GetTerrainData(RecentX / Param.TaskParam.SizeX / Param.TaskParam.StepX + Param.XIndex,
																	RecentY / Param.TaskParam.SizeY / Param.TaskParam.StepX + Param.YIndex);
			FVector RecentLocation = FVector(LocationX, LocationY, LocationZ);
			if (Param.XIndex == 0 && Param.YIndex == 5 && i == 0 && j == 0) {
				UE_LOG(LogTemp, Error, TEXT("%f, %f"), LocationX, LocationY)
			}
			//UE_LOG(LogTemp, Error, TEXT("%f"), LocationZ)
			Vectices->Add(RecentLocation);
			RecentY += Param.TaskParam.StepY;
		}
		RecentX += Param.TaskParam.StepX;
		RecentY = 0;
	}
}


/**
 * 创建地形面片
 */
void FTerrainMeshSectionTask::CreateTriangles(TArray<int32>* Triangles) {
	int32 Vertice1 = 0;
	int32 Vertice2 = 1;
	int32 Vertice3 = Param.TaskParam.SizeY + 1;
	int32 Vertice4 = Param.TaskParam.SizeY + 2;
	for (int32 i = 0; i < Param.TaskParam.SizeX; i++) {
		for (int32 j = 0; j < Param.TaskParam.SizeY; j++) {
			Triangles->Add(Vertice1);
			Triangles->Add(Vertice2);
			Triangles->Add(Vertice4);
			Triangles->Add(Vertice4);
			Triangles->Add(Vertice3);
			Triangles->Add(Vertice1);
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


/**
 * 创建地形法线
 */
//TODO 法线有问题
void FTerrainMeshSectionTask::CreateNormals(TArray<FVector>* Normals) {
	for (int32 i = 0; i <= Param.TaskParam.SizeX; i++) {
		for (int32 j = 0; j <= Param.TaskParam.SizeY; j++) {
			FVector Normal = FVector(0, 0, 1);
			Normals->Add(Normal);
		}
	}
}


/**
 * 创建地形UV
 */
void FTerrainMeshSectionTask::CreateUVs(TArray<FVector2D>* UV) {
	float UVStepX = 1 / (Param.TaskParam.SizeX + 1), UVStepY = 1 / (Param.TaskParam.SizeY + 1);
	float RecentX = 0, RecentY = 0;
	for (int32 i = 0; i <= Param.TaskParam.SizeX; i++) {
		for (int32 j = 0; j <= Param.TaskParam.SizeY; j++) {
			RecentY += UVStepY;
			FVector2D Recent = FVector2D(RecentX, RecentY);
			UV->Add(Recent);
		}
		RecentX += UVStepX;
		RecentY = 0;
	}
}


/**
 * 创建地形切线
 */
void FTerrainMeshSectionTask::CreateTangents(TArray<FProcMeshTangent>* Tangents) {
	for (int32 i = 0; i <= Param.TaskParam.SizeX; i++) {
		for (int32 j = 0; j <= Param.TaskParam.SizeY; j++) {
			FProcMeshTangent Tangent = FProcMeshTangent(1, 0, 0);
			Tangents->Add(Tangent);
		}
	}
}


/**
 * 创建地形颜色
 */
void FTerrainMeshSectionTask::CreateColors(TArray<FLinearColor>* VertexColors) {
	for (int32 i = 0; i <= Param.TaskParam.SizeX; i++) {
		for (int32 j = 0; j <= Param.TaskParam.SizeY; j++) {
			FLinearColor Color = FLinearColor(0.75, 0.75, 0.75, 1.0);
			VertexColors->Add(Color);
		}
	}
}

FTerrainMeshSectionTask::FTerrainMeshSectionTask(UProceduralMeshComponent* ProceduralMesh, FTerrainMeshSectionParameter param):DataManager(TerrainDataManager::GetInstance()){
	this->ProceduralMeshComponent = ProceduralMesh;
	this->Param = param;
}