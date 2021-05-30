#include "TerrainMaths/Modules/HITerrainVoronoi.h"

void FHITerrainVoronoi::SetSeed(int32 InSeed)
{
	Seed = InSeed;
}

void FHITerrainVoronoi::SetNumSites(int32 InNumSites)
{
	NumSites = InNumSites;
}

void FHITerrainVoronoi::SetAmplitude(float InAmplitude)
{
	Amplitude = InAmplitude;
}

void FHITerrainVoronoi::SetSizeX(int32 InSizeX)
{
	SizeX = InSizeX;
}

void FHITerrainVoronoi::SetSizeY(int32 InSizeY)
{
	SizeY = InSizeY;
}

void FHITerrainVoronoi::ApplyModule(UHITerrainData* Data)
{
	Bounds = FBox(FVector(0.0f, 0.0f, 0.0f), FVector(SizeX, SizeY, 0.0f));
	FRandomStream RandomStream(Seed);
	for(int i = 0; i < NumSites; i++)
	{
		Sites.Add(FVector(RandomStream.FRandRange(0.0f, SizeX), RandomStream.FRandRange(0.0f, SizeY), 0.0f));
	}
	VoronoiDiagram = new FVoronoiDiagram(Sites, Bounds, 0.0f);
	VoronoiDiagram->ComputeAllCells(AllCells);
	Sites.Empty();
	/*
	* 因为前面的Sites是随机生成的，这里算一下各Cell的中心点，然后再更新一次Sites，生成效果会更好一点
	* 算中心点的方法不是很好，以后看看是不是要改进。
	*/
	for(int32 i = 0; i < NumSites; i++)
	{
		FVector Site(0.0f, 0.0f, 0.0f);
		for(int j = 0; j < AllCells[i].Vertices.Num(); j++)
		{
			Site.X += AllCells[i].Vertices[j].X;
			Site.Y += AllCells[i].Vertices[j].Y;
		}
		Site /= AllCells[i].Vertices.Num();
		Sites.Add(Site);
	}
	delete VoronoiDiagram;
	VoronoiDiagram = new FVoronoiDiagram(Sites, Bounds, 0.0f);
	VoronoiDiagram->ComputeAllCells(AllCells);
}

float FHITerrainVoronoi::GetCellValue(float X, float Y)
{
	int32 ValueSeed = Position2Cell(X, Y);
	FRandomStream RandomStream(ValueSeed);
	return RandomStream.FRand();
}

const TArray<FVoronoiCellInfo>& FHITerrainVoronoi::GetAllCells()
{
	return AllCells;
}

int32 FHITerrainVoronoi::Position2Cell(float X, float Y)
{
	return VoronoiDiagram->FindCell(FVector(X, Y, 0.0f));
}

int32 FHITerrainVoronoi::Position2Cell(FVector Position)
{
	return VoronoiDiagram->FindCell(FVector(Position.X, Position.Y, 0.0f));
}

const TArray<FVector>& FHITerrainVoronoi::GetSites()
{
	return Sites;
}

FHITerrainVoronoi::~FHITerrainVoronoi()
{
	if(VoronoiDiagram)
	{
		delete VoronoiDiagram;
	}
}
