// Copyright LowkeyMe. All Rights Reserved. 2022


#include "Maze.h"

#include "Algorithms/Algorithm.h"
#include "Algorithms/Backtracker.h"
#include "Algorithms/Division.h"
#include "Algorithms/Eller.h"
#include "Algorithms/HaK.h"
#include "Algorithms/Kruskal.h"
#include "Algorithms/Prim.h"
#include "Algorithms/Sidewinder.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogMaze);

FMazeSize::FMazeSize(): X(5), Y(5)
{
}

FMazeSize::operator FIntVector2() const
{
	return FIntVector2{X, Y};
}

FMazeCoordinates::FMazeCoordinates(): X(0), Y(0)
{
}

void FMazeCoordinates::ClampByMazeSize(const FMazeSize& MazeSize)
{
	if (X >= MazeSize.X)
	{
		X = MazeSize.X - 1;
	}
	if (Y >= MazeSize.Y)
	{
		Y = MazeSize.Y - 1;
	}
}

bool FMazeCoordinates::operator==(const FMazeCoordinates& Other) const
{
	return X == Other.X && Y == Other.Y;
}

bool FMazeCoordinates::operator!=(const FMazeCoordinates& Other) const
{
	return !(*this == Other);
}

FMazeCoordinates::operator TTuple<int, int>() const
{
	return TPair<int32, int32>{X, Y};
}

AMaze::AMaze()
{
	PrimaryActorTick.bCanEverTick = false;

	GenerationAlgorithms.Add(EGenerationAlgorithm::Backtracker, TSharedPtr<Algorithm>(new Backtracker));
	GenerationAlgorithms.Add(EGenerationAlgorithm::Division, TSharedPtr<Algorithm>(new Division));
	GenerationAlgorithms.Add(EGenerationAlgorithm::HaK, TSharedPtr<Algorithm>(new HaK));
	GenerationAlgorithms.Add(EGenerationAlgorithm::Sidewinder, TSharedPtr<Algorithm>(new Sidewinder));
	GenerationAlgorithms.Add(EGenerationAlgorithm::Kruskal, TSharedPtr<Algorithm>(new Kruskal));
	GenerationAlgorithms.Add(EGenerationAlgorithm::Eller, TSharedPtr<Algorithm>(new Eller));
	GenerationAlgorithms.Add(EGenerationAlgorithm::Prim, TSharedPtr<Algorithm>(new Prim));

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	FloorCells = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FloorCells"));
	if (FloorCells)
	{
		FloorCells->SetupAttachment(GetRootComponent());
	}

	PathFloorCells = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("PathFloorCells"));
	if (PathFloorCells)
	{
		PathFloorCells->SetupAttachment(GetRootComponent());
	}

	WallCells = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WallCells"));
	if (WallCells)
	{
		WallCells->SetupAttachment(GetRootComponent());
	}

	OutlineWallCells = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("OutlineWallCells"));
	if (OutlineWallCells)
	{
		OutlineWallCells->SetupAttachment(GetRootComponent());
	}

	SpawnedEndpointActor = nullptr;
}

void AMaze::UpdateMaze()
{
	ClearMaze();

	if (!(FloorStaticMesh && WallStaticMesh))
	{
		UE_LOG(LogMaze, Warning, TEXT("To create maze specify FloorStaticMesh and WallStaticMesh."));
		return;
	}

	FloorCells->SetStaticMesh(FloorStaticMesh);
	WallCells->SetStaticMesh(WallStaticMesh);
	if (OutlineStaticMesh)
	{
		OutlineWallCells->SetStaticMesh(OutlineStaticMesh);
	}
	if (PathStaticMesh)
	{
		PathFloorCells->SetStaticMesh(PathStaticMesh);
	}

	MazeCellSize = GetMaxCellSize();

	if (OutlineStaticMesh)
	{
		CreateMazeOutline();
	}
	MazeGrid = GenerationAlgorithms[GenerationAlgorithm]->GetGrid(MazeSize, Seed);

	// Handle maze endpoint (independent of path generation)
	if (bHasEndpoint)
	{
		// Auto-select random edge floor position for endpoint
		TArray<FMazeCoordinates> EdgeFloorPositions;

		// Collect all floor cells on the edges
		for (int32 X = 0; X < MazeSize.X; ++X)
		{
			// North edge (Y=0)
			if (MazeGrid[0][X] == 1)
			{
				FMazeCoordinates Coord;
				Coord.X = X;
				Coord.Y = 0;
				EdgeFloorPositions.Add(Coord);
			}
			// South edge (Y=MazeSize.Y-1)
			if (MazeGrid[MazeSize.Y - 1][X] == 1)
			{
				FMazeCoordinates Coord;
				Coord.X = X;
				Coord.Y = MazeSize.Y - 1;
				EdgeFloorPositions.Add(Coord);
			}
		}

		for (int32 Y = 1; Y < MazeSize.Y - 1; ++Y) // Skip corners already added
		{
			// West edge (X=0)
			if (MazeGrid[Y][0] == 1)
			{
				FMazeCoordinates Coord;
				Coord.X = 0;
				Coord.Y = Y;
				EdgeFloorPositions.Add(Coord);
			}
			// East edge (X=MazeSize.X-1)
			if (MazeGrid[Y][MazeSize.X - 1] == 1)
			{
				FMazeCoordinates Coord;
				Coord.X = MazeSize.X - 1;
				Coord.Y = Y;
				EdgeFloorPositions.Add(Coord);
			}
		}

		// Pick a random edge floor position
		if (EdgeFloorPositions.Num() > 0)
		{
			const int32 RandomIndex = FMath::RandRange(0, EdgeFloorPositions.Num() - 1);
			MazeEndpoint = EdgeFloorPositions[RandomIndex];
			UE_LOG(LogMaze, Log, TEXT("Auto-selected endpoint at edge: (%d,%d)"), MazeEndpoint.X, MazeEndpoint.Y);
		}
		else
		{
			// Fallback: use a corner
			UE_LOG(LogMaze, Warning, TEXT("No edge floor positions found. Using corner as endpoint."));
			MazeEndpoint.X = MazeSize.X - 1;
			MazeEndpoint.Y = MazeSize.Y - 1;
		}

		// Optionally auto-set exit door at endpoint
		if (bCreateDoors && !bGeneratePath)
		{
			ExitDoor = MazeEndpoint;
		}

		// Spawn endpoint mesh if provided (done after maze generation loop)
		// This will be done at the end of UpdateMaze after all cells are placed
	}

	if (bGeneratePath)
	{
		PathStart.ClampByMazeSize(MazeSize);
		PathEnd.ClampByMazeSize(MazeSize);

		// Debug: Check if start and end are on floor cells
		bool bStartIsFloor = MazeGrid[PathStart.Y][PathStart.X] == 1;
		bool bEndIsFloor = MazeGrid[PathEnd.Y][PathEnd.X] == 1;
		UE_LOG(LogMaze, Warning, TEXT("PathStart (%d,%d) is %s. PathEnd (%d,%d) is %s."),
			PathStart.X, PathStart.Y, bStartIsFloor ? TEXT("FLOOR") : TEXT("WALL"),
			PathEnd.X, PathEnd.Y, bEndIsFloor ? TEXT("FLOOR") : TEXT("WALL"));

		MazePathGrid = GetMazePath(PathStart, PathEnd, PathLength);

		// Auto-set doors at path start and end if door creation is enabled
		if (bCreateDoors)
		{
			EntranceDoor = PathStart;
			ExitDoor = PathEnd;
		}
	}
	else if (bCreateDoors)
	{
		// Clamp door positions if path generation is disabled
		EntranceDoor.ClampByMazeSize(MazeSize);
		if (!bHasEndpoint)
		{
			ExitDoor.ClampByMazeSize(MazeSize);
		}
	}

	// Calculate the scaled cell size based on floor width
	const FVector2D ScaledCellSize = MazeCellSize * FloorWidth;
	const FVector FloorScale(FloorWidth, FloorWidth, 1.0f);

	for (int32 Y = 0; Y < MazeSize.Y; ++Y)
	{
		for (int32 X = 0; X < MazeSize.X; ++X)
		{
			// Calculate center position for this logical cell
			const FVector CenterLocation(
				ScaledCellSize.X * X + (ScaledCellSize.X * 0.5f) - (MazeCellSize.X * 0.5f),
				ScaledCellSize.Y * Y + (ScaledCellSize.Y * 0.5f) - (MazeCellSize.Y * 0.5f),
				0.f);

			if (bGeneratePath && PathStaticMesh && MazePathGrid.Num() > 0 && MazePathGrid[Y][X])
			{
				// Scale the path floor cell by FloorWidth from center
				FTransform Transform(CenterLocation);
				Transform.SetScale3D(FloorScale);
				PathFloorCells->AddInstance(Transform);
			}
			else if (MazeGrid[Y][X])
			{
				// Scale the floor cell by FloorWidth from center
				FTransform Transform(CenterLocation);
				Transform.SetScale3D(FloorScale);
				FloorCells->AddInstance(Transform);
			}
			else
			{
				// For walls: check ALL 4 neighbors to determine proper scaling
				// Count adjacent walls in each direction to determine wall continuity

				bool bHasWallLeft = (X > 0 && !MazeGrid[Y][X - 1]);
				bool bHasWallRight = (X < MazeSize.X - 1 && !MazeGrid[Y][X + 1]);
				bool bHasWallTop = (Y > 0 && !MazeGrid[Y - 1][X]);
				bool bHasWallBottom = (Y < MazeSize.Y - 1 && !MazeGrid[Y + 1][X]);

				// Check if on edge
				bool bIsLeftEdge = (X == 0);
				bool bIsRightEdge = (X == MazeSize.X - 1);
				bool bIsTopEdge = (Y == 0);
				bool bIsBottomEdge = (Y == MazeSize.Y - 1);

				// Count wall neighbors in horizontal (X) and vertical (Y) directions
				int32 HorizontalWallNeighbors = (bHasWallLeft ? 1 : 0) + (bHasWallRight ? 1 : 0);
				int32 VerticalWallNeighbors = (bHasWallTop ? 1 : 0) + (bHasWallBottom ? 1 : 0);

				// Add edge contributions
				if (bIsLeftEdge) HorizontalWallNeighbors++;
				if (bIsRightEdge) HorizontalWallNeighbors++;
				if (bIsTopEdge) VerticalWallNeighbors++;
				if (bIsBottomEdge) VerticalWallNeighbors++;

				FVector WallScale;

				// Determine scaling based on wall continuity
				if (HorizontalWallNeighbors >= 1 && VerticalWallNeighbors == 0)
				{
					// Horizontal wall segment: scale along X
					WallScale = FVector(FloorWidth, 1.0f, 1.0f);
				}
				else if (VerticalWallNeighbors >= 1 && HorizontalWallNeighbors == 0)
				{
					// Vertical wall segment: scale along Y
					WallScale = FVector(1.0f, FloorWidth, 1.0f);
				}
				else
				{
					// Corner, intersection, or isolated: scale both or determine dominant direction
					if (HorizontalWallNeighbors > VerticalWallNeighbors)
					{
						// More horizontal continuity
						WallScale = FVector(FloorWidth, 1.0f, 1.0f);
					}
					else if (VerticalWallNeighbors > HorizontalWallNeighbors)
					{
						// More vertical continuity
						WallScale = FVector(1.0f, FloorWidth, 1.0f);
					}
					else
					{
						// Equal or both directions: scale both (corner/intersection)
						WallScale = FVector(FloorWidth, FloorWidth, 1.0f);
					}
				}

				FTransform Transform(CenterLocation);
				Transform.SetScale3D(WallScale);
				WallCells->AddInstance(Transform);
			}
		}
	}

	EnableCollision(bUseCollision);
}

void AMaze::CreateMazeOutline() const
{
	// Calculate the scaled cell size based on floor width
	const FVector2D ScaledCellSize = MazeCellSize * FloorWidth;

	FVector Location1{0.f};
	FVector Location2{0.f};

	// Create north and south walls (top and bottom)
	Location1.Y = -MazeCellSize.Y;
	Location2.Y = ScaledCellSize.Y * MazeSize.Y;

	// Place outline walls for each floor width unit
	for (int32 FX = -1; FX < MazeSize.X * FloorWidth + 1; ++FX)
	{
		Location1.X = Location2.X = FX * MazeCellSize.X;

		// Check if this position corresponds to a door
		int32 GridX = FX / FloorWidth;
		bool bSkipNorthWall = bCreateDoors && GridX >= 0 && GridX < MazeSize.X &&
			((EntranceDoor.X == GridX && EntranceDoor.Y == 0) || (ExitDoor.X == GridX && ExitDoor.Y == 0));
		if (!bSkipNorthWall)
		{
			OutlineWallCells->AddInstance(FTransform{Location1});
		}

		bool bSkipSouthWall = bCreateDoors && GridX >= 0 && GridX < MazeSize.X &&
			((EntranceDoor.X == GridX && EntranceDoor.Y == MazeSize.Y - 1) || (ExitDoor.X == GridX && ExitDoor.Y == MazeSize.Y - 1));
		if (!bSkipSouthWall)
		{
			OutlineWallCells->AddInstance(FTransform{Location2});
		}
	}

	// Create west and east walls (left and right)
	Location1.X = -MazeCellSize.X;
	Location2.X = ScaledCellSize.X * MazeSize.X;

	for (int32 FY = 0; FY < MazeSize.Y * FloorWidth; ++FY)
	{
		Location1.Y = Location2.Y = FY * MazeCellSize.Y;

		// Check if this position corresponds to a door
		int32 GridY = FY / FloorWidth;
		bool bSkipWestWall = bCreateDoors && GridY >= 0 && GridY < MazeSize.Y &&
			((EntranceDoor.X == 0 && EntranceDoor.Y == GridY) || (ExitDoor.X == 0 && ExitDoor.Y == GridY));
		if (!bSkipWestWall)
		{
			OutlineWallCells->AddInstance(FTransform{Location1});
		}

		bool bSkipEastWall = bCreateDoors && GridY >= 0 && GridY < MazeSize.Y &&
			((EntranceDoor.X == MazeSize.X - 1 && EntranceDoor.Y == GridY) || (ExitDoor.X == MazeSize.X - 1 && ExitDoor.Y == GridY));
		if (!bSkipEastWall)
		{
			OutlineWallCells->AddInstance(FTransform{Location2});
		}
	}
}

TArray<TArray<uint8>> AMaze::GetMazePath(const FMazeCoordinates& Start, const FMazeCoordinates& End, int32& OutLength)
{
	TArray<TArray<int32>> Graph;
	Graph.Reserve(MazeGrid.Num() * MazeGrid[0].Num());

	// Graph creation.
	for (int32 GraphVertex,
	           Y = 0; Y < MazeGrid.Num(); ++Y)
	{
		for (int32 X = 0; X < MazeGrid[Y].Num(); ++X)
		{
			GraphVertex = Y * MazeGrid[Y].Num() + X;

			Graph.Emplace(TArray<int32>());
			if (!MazeGrid[Y][X])
			{
				continue;
			}

			Graph[GraphVertex].Reserve(4); // There are only 4 directions possible.

			if (X > 0 && MazeGrid[Y][X - 1]) // West direction.
			{
				Graph[GraphVertex].Emplace(GraphVertex - 1);
			}
			if (X + 1 < MazeGrid[Y].Num() && MazeGrid[Y][X + 1]) // East direction.
			{
				Graph[GraphVertex].Emplace(GraphVertex + 1);
			}
			if (Y > 0 && MazeGrid[Y - 1][X]) // North direction.
			{
				Graph[GraphVertex].Emplace(GraphVertex - MazeGrid[Y].Num());
			}
			if (Y + 1 < MazeGrid.Num() && MazeGrid[Y + 1][X]) // South direction.
			{
				Graph[GraphVertex].Emplace(GraphVertex + MazeGrid[Y].Num());
			}

			Graph[GraphVertex].Shrink();
		}
	}

	const int32 StartVertex = Start.Y * MazeGrid[0].Num() + Start.X;
	const int32 EndVertex = End.Y * MazeGrid[0].Num() + End.X;


	TQueue<int32> Vertices;

	const int32 VerticesAmount = MazeGrid.Num() * MazeGrid[0].Num();

	TArray<bool> Visited;
	Visited.Init(false, VerticesAmount);

	TArray<int32> Parents;
	Parents.Init(-1, VerticesAmount);

	TArray<int32> Distances;
	Distances.Init(0, VerticesAmount);

	int32 Vertex;
	Vertices.Enqueue(StartVertex);
	Visited[StartVertex] = true;
	while (Vertices.Dequeue(Vertex))
	{
		for (int32 i = 0; i < Graph[Vertex].Num(); ++i)
		{
			if (const int32 Adjacent = Graph[Vertex][i]; !Visited[Adjacent])
			{
				Visited[Adjacent] = true;
				Vertices.Enqueue(Adjacent);
				Distances[Adjacent] = Distances[Vertex] + 1;
				Parents[Adjacent] = Vertex;
			}
		}
	}

	TArray<int32> GraphPath;
	if (!Visited[EndVertex])
	{
		UE_LOG(LogMaze, Warning, TEXT("Path is not reachable."));
		return TArray<TArray<uint8>>();
	}

	for (int VertexNumber = EndVertex; VertexNumber != -1; VertexNumber = Parents[VertexNumber])
	{
		GraphPath.Emplace(VertexNumber);
	}

	Algo::Reverse(GraphPath);

	TArray<TArray<uint8>> Path;
	Path.Init(TArray<uint8>(), MazeGrid.Num());
	for (int Y = 0; Y < MazeGrid.Num(); ++Y)
	{
		Path[Y].SetNumZeroed(MazeGrid[Y].Num());
	}

	for (int32 VertexNumber, i = 0; i < GraphPath.Num(); ++i)
	{
		VertexNumber = GraphPath[i];

		Path[VertexNumber / MazeGrid[0].Num()][VertexNumber % MazeGrid[0].Num()] = 1;
	}


	OutLength = Distances[EndVertex] + 1;
	return Path;
}

void AMaze::EnableCollision(const bool bShouldEnable)
{
	if (bShouldEnable)
	{
		FloorCells->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WallCells->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		OutlineWallCells->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PathFloorCells->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		FloorCells->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WallCells->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OutlineWallCells->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PathFloorCells->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}


void AMaze::ClearMaze()
{
	if (FloorCells)
	{
		FloorCells->ClearInstances();
	}
	if (WallCells)
	{
		WallCells->ClearInstances();
	}
	if (OutlineWallCells)
	{
		OutlineWallCells->ClearInstances();
	}
	if (PathFloorCells)
	{
		PathFloorCells->ClearInstances();
	}

	// Destroy previously spawned endpoint actor
	if (SpawnedEndpointActor && IsValid(SpawnedEndpointActor))
	{
		SpawnedEndpointActor->Destroy();
		SpawnedEndpointActor = nullptr;
	}
}

FVector2D AMaze::GetMaxCellSize() const
{
	const FVector FloorSize3D = FloorStaticMesh->GetBoundingBox().GetSize();
	const FVector WallSize3D = WallStaticMesh->GetBoundingBox().GetSize();

	const FVector2D FloorSize2D{FloorSize3D.X, FloorSize3D.Y};
	const FVector2D WallSize2D{WallSize3D.X, WallSize3D.Y};

	const FVector2D MaxCellSize = FVector2D::Max(FloorSize2D, WallSize2D);
	if (OutlineStaticMesh)
	{
		const FVector OutlineSize3D = OutlineStaticMesh->GetBoundingBox().GetSize();
		if (const FVector2D OutlineSize2D{OutlineSize3D.X, OutlineSize3D.Y};
			OutlineSize2D.ComponentwiseAllGreaterThan(MaxCellSize))
		{
			return OutlineSize2D;
		}
	}
	return MaxCellSize;
}

void AMaze::Randomize()
{
	MazeSize.X = FMath::RandRange(3, 101) | 1; // | 1 to make odd.
	MazeSize.Y = FMath::RandRange(3, 101) | 1;

	TArray<EGenerationAlgorithm> Algorithms;
	const int32 Num = GenerationAlgorithms.GetKeys(Algorithms);
	GenerationAlgorithm = Algorithms[FMath::RandRange(0, Num - 1)];

	Seed = FMath::RandRange(MIN_int32, MAX_int32);

	if (bForceEdgeDoors)
	{
		// Generate maze first to know which cells are floors
		MazeGrid = GenerationAlgorithms[GenerationAlgorithm]->GetGrid(MazeSize, Seed);

		// Find random floor positions on edges for entrance and exit
		TArray<FMazeCoordinates> EdgeFloorPositions;

		// Collect all floor cells on the edges
		for (int32 X = 0; X < MazeSize.X; ++X)
		{
			// North edge (Y=0)
			if (MazeGrid[0][X] == 1)
			{
				FMazeCoordinates Coord;
				Coord.X = X;
				Coord.Y = 0;
				EdgeFloorPositions.Add(Coord);
			}
			// South edge (Y=MazeSize.Y-1)
			if (MazeGrid[MazeSize.Y - 1][X] == 1)
			{
				FMazeCoordinates Coord;
				Coord.X = X;
				Coord.Y = MazeSize.Y - 1;
				EdgeFloorPositions.Add(Coord);
			}
		}

		for (int32 Y = 0; Y < MazeSize.Y; ++Y)
		{
			// West edge (X=0)
			if (MazeGrid[Y][0] == 1)
			{
				FMazeCoordinates Coord;
				Coord.X = 0;
				Coord.Y = Y;
				EdgeFloorPositions.Add(Coord);
			}
			// East edge (X=MazeSize.X-1)
			if (MazeGrid[Y][MazeSize.X - 1] == 1)
			{
				FMazeCoordinates Coord;
				Coord.X = MazeSize.X - 1;
				Coord.Y = Y;
				EdgeFloorPositions.Add(Coord);
			}
		}

		// Pick two random different positions
		if (EdgeFloorPositions.Num() >= 2)
		{
			const int32 StartIndex = FMath::RandRange(0, EdgeFloorPositions.Num() - 1);
			PathStart = EdgeFloorPositions[StartIndex];

			// Pick a different position for exit
			int32 EndIndex;
			do
			{
				EndIndex = FMath::RandRange(0, EdgeFloorPositions.Num() - 1);
			} while (EndIndex == StartIndex && EdgeFloorPositions.Num() > 1);

			PathEnd = EdgeFloorPositions[EndIndex];
		}
		else
		{
			// Fallback to corners if not enough edge floors found
			UE_LOG(LogMaze, Warning, TEXT("Not enough floor cells on edges. Using corners."));
			PathStart.X = 0;
			PathStart.Y = 0;
			PathEnd.X = MazeSize.X - 1;
			PathEnd.Y = MazeSize.Y - 1;
		}
	}
	else
	{
		// Original behavior - use corners
		PathStart.X = 0;
		PathStart.Y = 0;
		PathEnd.X = MazeSize.X - 1;
		PathEnd.Y = MazeSize.Y - 1;
	}

	UpdateMaze();
}


TArray<FVector> AMaze::GetRandomFloorLocations(int32 Count)
{
	TArray<FVector> AllFloorLocations = GetAllFloorLocations();

	if (Count >= AllFloorLocations.Num())
	{
		return AllFloorLocations;
	}

	// Shuffle the array to randomize
	for (int32 i = AllFloorLocations.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		AllFloorLocations.Swap(i, j);
	}

	// Take only the first Count elements
	TArray<FVector> RandomLocations;
	RandomLocations.Reserve(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		RandomLocations.Add(AllFloorLocations[i]);
	}

	return RandomLocations;
}

TArray<FVector> AMaze::GetAllFloorLocations()
{
	TArray<FVector> FloorLocations;

	if (MazeGrid.Num() == 0)
	{
		UE_LOG(LogMaze, Warning, TEXT("Maze grid is empty. Call UpdateMaze() first."));
		return FloorLocations;
	}

	UE_LOG(LogMaze, Log, TEXT("GetAllFloorLocations: MazeGrid size is %dx%d"), MazeSize.X, MazeSize.Y);

	// Reserve space accounting for FloorWidth (each logical cell has FloorWidth^2 physical tiles)
	FloorLocations.Reserve(MazeSize.X * MazeSize.Y * FloorWidth * FloorWidth / 2);

	// Calculate the scaled cell size based on floor width
	const FVector2D ScaledCellSize = MazeCellSize * FloorWidth;

	const FTransform& ActorTransform = GetActorTransform();

	for (int32 Y = 0; Y < MazeSize.Y; ++Y)
	{
		for (int32 X = 0; X < MazeSize.X; ++X)
		{
			// Check if this is a floor cell (value = 1)
			if (MazeGrid[Y][X])
			{
				// For each logical floor cell, return all physical floor tile positions
				for (int32 FY = 0; FY < FloorWidth; ++FY)
				{
					for (int32 FX = 0; FX < FloorWidth; ++FX)
					{
						// Calculate position of each physical floor tile
						const FVector LocalPosition(
							ScaledCellSize.X * X + MazeCellSize.X * FX + (MazeCellSize.X * 0.5f),
							ScaledCellSize.Y * Y + MazeCellSize.Y * FY + (MazeCellSize.Y * 0.5f),
							0.f);

						// Transform to world space
						const FVector WorldPosition = ActorTransform.TransformPosition(LocalPosition);

						FloorLocations.Add(WorldPosition);
					}
				}
			}
		}
	}

	UE_LOG(LogMaze, Log, TEXT("GetAllFloorLocations: Found %d floor locations"), FloorLocations.Num());

	return FloorLocations;
}

FTransform AMaze::GetPathStartTransform()
{
	// Calculate the scaled cell size based on floor width
	const FVector2D ScaledCellSize = MazeCellSize * FloorWidth;

	// Calculate base position of the logical cell
	FVector LocalPosition(
		ScaledCellSize.X * PathStart.X,
		ScaledCellSize.Y * PathStart.Y,
		0.f);

	// Add offset to center of the first physical floor tile within the cell
	LocalPosition.X += MazeCellSize.X * 0.5f;
	LocalPosition.Y += MazeCellSize.Y * 0.5f;

	// Calculate rotation based on which edge the entrance is on
	FRotator Rotation = FRotator::ZeroRotator;

	if (PathStart.Y == 0)
	{
		// North edge (Y=0) - face south (into maze)
		Rotation.Yaw = 90.0f;
	}
	else if (PathStart.Y == MazeSize.Y - 1)
	{
		// South edge - face north (into maze)
		Rotation.Yaw = -90.0f;
	}
	else if (PathStart.X == 0)
	{
		// West edge (X=0) - face east (into maze)
		Rotation.Yaw = 0.0f;
	}
	else if (PathStart.X == MazeSize.X - 1)
	{
		// East edge - face west (into maze)
		Rotation.Yaw = 180.0f;
	}

	// Create local transform
	FTransform LocalTransform(Rotation, LocalPosition);

	// Transform to world space
	return LocalTransform * GetActorTransform();
}

FTransform AMaze::GetPathEndTransform()
{
	// Calculate the scaled cell size based on floor width
	const FVector2D ScaledCellSize = MazeCellSize * FloorWidth;

	// Calculate base position of the logical cell
	FVector LocalPosition(
		ScaledCellSize.X * PathEnd.X,
		ScaledCellSize.Y * PathEnd.Y,
		0.f);

	// Add offset to center of the first physical floor tile within the cell
	LocalPosition.X += MazeCellSize.X * 0.5f;
	LocalPosition.Y += MazeCellSize.Y * 0.5f;

	// Calculate rotation based on which edge the exit is on
	FRotator Rotation = FRotator::ZeroRotator;

	if (PathEnd.Y == 0)
	{
		// North edge (Y=0) - face south (into maze)
		Rotation.Yaw = 90.0f;
	}
	else if (PathEnd.Y == MazeSize.Y - 1)
	{
		// South edge - face north (into maze)
		Rotation.Yaw = -90.0f;
	}
	else if (PathEnd.X == 0)
	{
		// West edge (X=0) - face east (into maze)
		Rotation.Yaw = 0.0f;
	}
	else if (PathEnd.X == MazeSize.X - 1)
	{
		// East edge - face west (into maze)
		Rotation.Yaw = 180.0f;
	}

	// Create local transform
	FTransform LocalTransform(Rotation, LocalPosition);

	// Transform to world space
	return LocalTransform * GetActorTransform();
}

FTransform AMaze::GetMazeEndpointTransform()
{
	if (!bHasEndpoint)
	{
		UE_LOG(LogMaze, Warning, TEXT("GetMazeEndpointTransform called but bHasEndpoint is false!"));
		return GetActorTransform();
	}

	// Calculate the scaled cell size based on floor width
	const FVector2D ScaledCellSize = MazeCellSize * FloorWidth;

	// Calculate base position of the logical cell
	FVector LocalPosition(
		ScaledCellSize.X * MazeEndpoint.X,
		ScaledCellSize.Y * MazeEndpoint.Y,
		0.f);

	// Add offset to center of the scaled cell (center of the endpoint area)
	LocalPosition.X += ScaledCellSize.X * 0.5f;
	LocalPosition.Y += ScaledCellSize.Y * 0.5f;

	// Calculate rotation based on which edge the endpoint is on
	FRotator Rotation = FRotator::ZeroRotator;

	if (MazeEndpoint.Y == 0)
	{
		// North edge (Y=0) - face south (into maze)
		Rotation.Yaw = 90.0f;
	}
	else if (MazeEndpoint.Y == MazeSize.Y - 1)
	{
		// South edge - face north (into maze)
		Rotation.Yaw = -90.0f;
	}
	else if (MazeEndpoint.X == 0)
	{
		// West edge (X=0) - face east (into maze)
		Rotation.Yaw = 0.0f;
	}
	else if (MazeEndpoint.X == MazeSize.X - 1)
	{
		// East edge - face west (into maze)
		Rotation.Yaw = 180.0f;
	}

	// Create local transform
	FTransform LocalTransform(Rotation, LocalPosition);

	// Transform to world space
	return LocalTransform * GetActorTransform();
}

AActor* AMaze::SpawnEndpointActor()
{
	if (!bHasEndpoint)
	{
		UE_LOG(LogMaze, Warning, TEXT("SpawnEndpointActor called but bHasEndpoint is false!"));
		return nullptr;
	}

	if (!EndpointActorClass)
	{
		UE_LOG(LogMaze, Warning, TEXT("SpawnEndpointActor called but EndpointActorClass is not set!"));
		return nullptr;
	}

	// Validate world exists
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogMaze, Error, TEXT("SpawnEndpointActor: World is null!"));
		return nullptr;
	}

	// Destroy existing endpoint actor if any
	if (SpawnedEndpointActor && IsValid(SpawnedEndpointActor))
	{
		SpawnedEndpointActor->Destroy();
		SpawnedEndpointActor = nullptr;
	}

	// Get the endpoint transform
	FTransform EndpointTransform = GetMazeEndpointTransform();

	// Spawn the actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedEndpointActor = World->SpawnActor<AActor>(EndpointActorClass, EndpointTransform, SpawnParams);

	if (SpawnedEndpointActor)
	{
		UE_LOG(LogMaze, Log, TEXT("Spawned endpoint actor '%s' at (%s)"),
			*SpawnedEndpointActor->GetName(),
			*EndpointTransform.GetLocation().ToString());
	}
	else
	{
		UE_LOG(LogMaze, Error, TEXT("Failed to spawn endpoint actor of class '%s'!"),
			*EndpointActorClass->GetName());
	}

	return SpawnedEndpointActor;
}

FTransform AMaze::GetRandomSpawnTransform()
{
	TArray<FVector> FloorLocations = GetRandomFloorLocations(1);

	if (FloorLocations.Num() == 0)
	{
		UE_LOG(LogMaze, Warning, TEXT("GetRandomSpawnTransform: No floor locations available!"));
		return GetActorTransform();
	}

	// Return transform at random floor location with default rotation
	return FTransform(FRotator::ZeroRotator, FloorLocations[0]);
}

TArray<FVector> AMaze::GetRandomFloorLocationsExcluding(int32 Count, const TArray<FVector>& ExcludePositions, float ExclusionRadius)
{
	TArray<FVector> AllFloorLocations = GetAllFloorLocations();
	TArray<FVector> ValidLocations;

	// Filter out locations that are too close to excluded positions
	for (const FVector& FloorLocation : AllFloorLocations)
	{
		bool bTooClose = false;

		for (const FVector& ExcludedPos : ExcludePositions)
		{
			const float DistanceSquared = FVector::DistSquared(FloorLocation, ExcludedPos);
			const float RadiusSquared = ExclusionRadius * ExclusionRadius;

			if (DistanceSquared < RadiusSquared)
			{
				bTooClose = true;
				break;
			}
		}

		if (!bTooClose)
		{
			ValidLocations.Add(FloorLocation);
		}
	}

	if (ValidLocations.Num() == 0)
	{
		UE_LOG(LogMaze, Warning, TEXT("GetRandomFloorLocationsExcluding: No valid locations found after exclusion!"));
		return TArray<FVector>();
	}

	// Return all valid locations if requested count exceeds available
	if (Count >= ValidLocations.Num())
	{
		UE_LOG(LogMaze, Log, TEXT("GetRandomFloorLocationsExcluding: Requested %d, returning all %d valid locations"), Count, ValidLocations.Num());
		return ValidLocations;
	}

	// Shuffle and return random subset
	TArray<FVector> RandomLocations;
	RandomLocations.Reserve(Count);

	// Shuffle the valid locations
	for (int32 i = ValidLocations.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		ValidLocations.Swap(i, j);
	}

	// Take first Count elements
	for (int32 i = 0; i < Count; ++i)
	{
		RandomLocations.Add(ValidLocations[i]);
	}

	UE_LOG(LogMaze, Log, TEXT("GetRandomFloorLocationsExcluding: Returning %d random locations"), RandomLocations.Num());
	return RandomLocations;
}

void AMaze::BeginPlay()
{
	Super::BeginPlay();

	// Regenerate the maze at runtime since MazeGrid is not serialized
	UpdateMaze();
}

void AMaze::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	if (Transform.Equals(LastMazeTransform))
	{
#endif
		UpdateMaze();
#if WITH_EDITOR
	}
	LastMazeTransform = Transform;
#endif
}
