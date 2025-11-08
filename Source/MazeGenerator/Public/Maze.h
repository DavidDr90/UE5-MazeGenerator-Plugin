// Copyright LowkeyMe. All Rights Reserved. 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Maze.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMaze, Warning, All);

UENUM(BlueprintType)
enum class EGenerationAlgorithm : uint8
{
	Backtracker UMETA(DisplayName="Recursive Backtracker"),
	Division UMETA(DisplayName="Recursive Division"),
	HaK UMETA(DisplayName="Hunt-and-Kill"),
	Sidewinder,
	Kruskal,
	Eller,
	Prim
};

USTRUCT(BlueprintType)
struct FMazeSize
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Maze",
		meta=(ClampMin=3, UIMin=5, UIMax=101, ClampMax=9999, NoResetToDefault))
	int32 X;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Maze",
		meta=(ClampMin=3, UIMin=5, UIMax=101, ClampMax=9999, NoResetToDefault))
	int32 Y;

	FMazeSize();

	operator FIntVector2() const;
};

USTRUCT(BlueprintType)
struct FMazeCoordinates
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Maze",
		meta=(NoSpinbox=true, ClampMin=0, NoResetToDefault))
	int32 X;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Maze",
		meta=(NoSpinbox=true, ClampMin=0, Delta=1, NoResetToDefault))
	int32 Y;

	FMazeCoordinates();

	void ClampByMazeSize(const FMazeSize& MazeSize);

	bool operator==(const FMazeCoordinates& Other) const;

	bool operator!=(const FMazeCoordinates& Other) const;

	operator TPair<int32, int32>() const;
};

class Algorithm;
class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class MAZEGENERATOR_API AMaze : public AActor
{
	GENERATED_BODY()

public:
	AMaze();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze",
		meta=(NoResetToDefault, ExposeOnSpawn, DisplayPriority=0))
	EGenerationAlgorithm GenerationAlgorithm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze", meta=(ExposeOnSpawn, DisplayPriority=1))
	int32 Seed;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Maze", meta=(ExposeOnSpawn, DisplayPriority=2))
	FMazeSize MazeSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Floor", Category="Maze|Cells",
		meta=(NoResetToDefault, ExposeOnSpawn, DisplayPriority=0))
	UStaticMesh* FloorStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Wall", Category="Maze|Cells",
		meta=(NoResetToDefault, ExposeOnSpawn, DisplayPriority=1))
	UStaticMesh* WallStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Outline Wall", Category="Maze|Cells",
		meta=(ExposeOnSpawn, DisplayPriority=2))
	UStaticMesh* OutlineStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Endpoint", meta=(ExposeOnSpawn))
	bool bHasEndpoint = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Maze|Endpoint",
		meta=(ExposeOnSpawn, EditCondition="bHasEndpoint", EditConditionHides))
	FMazeCoordinates MazeEndpoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Endpoint Actor Class", Category="Maze|Endpoint",
		meta=(ExposeOnSpawn, EditCondition="bHasEndpoint", EditConditionHides))
	TSubclassOf<AActor> EndpointActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Pathfinder", meta=(ExposeOnSpawn))
	bool bGeneratePath = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Maze|Pathfinder",
		meta=(ExposeOnSpawn, EditCondition="bGeneratePath", EditConditionHides))
	FMazeCoordinates PathStart;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Maze|Pathfinder",
		meta=(ExposeOnSpawn, EditCondition="bGeneratePath", EditConditionHides))
	FMazeCoordinates PathEnd;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Doors", meta=(ExposeOnSpawn))
	bool bCreateDoors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Doors", meta=(ExposeOnSpawn))
	bool bForceEdgeDoors = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Maze|Doors",
		meta=(ExposeOnSpawn, EditCondition="bCreateDoors", EditConditionHides))
	FMazeCoordinates EntranceDoor;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Maze|Doors",
		meta=(ExposeOnSpawn, EditCondition="bCreateDoors", EditConditionHides))
	FMazeCoordinates ExitDoor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="Path Floor", Category="Maze|Pathfinder",
		meta=(ExposeOnSpawn, EditCondition="bGeneratePath", EditConditionHides))
	UStaticMesh* PathStaticMesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Maze|Pathfinder",
		meta=(EditCondition="bGeneratePath", EditConditionHides))
	int32 PathLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze")
	bool bUseCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Cells",
		meta=(ClampMin=1, UIMin=1, UIMax=9, ClampMax=99, NoResetToDefault))
	int32 FloorWidth = 1;

protected:
	TArray<TArray<uint8>> MazeGrid;

	TArray<TArray<uint8>> MazePathGrid;

	TMap<EGenerationAlgorithm, TSharedPtr<Algorithm>> GenerationAlgorithms;

	UPROPERTY()
	UHierarchicalInstancedStaticMeshComponent* FloorCells;

	UPROPERTY()
	UHierarchicalInstancedStaticMeshComponent* WallCells;

	UPROPERTY()
	UHierarchicalInstancedStaticMeshComponent* OutlineWallCells;

	UPROPERTY()
	UHierarchicalInstancedStaticMeshComponent* PathFloorCells;

	UPROPERTY()
	AActor* SpawnedEndpointActor;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Maze|Cells")
	FVector2D MazeCellSize;	

public:
	// Update Maze according to pre-set parameters: Size, Generation Algorithm, Seed and Path-related params.
	UFUNCTION(BlueprintCallable, Category="Maze")
	virtual void UpdateMaze();

	/**
	 * Returns an array of random floor locations in world space.
	 * @param Count - Number of random locations to return. If Count exceeds available floor cells, returns all available.
	 * @return Array of world space positions on floor cells
	 */
	UFUNCTION(BlueprintCallable, Category="Maze")
	virtual TArray<FVector> GetRandomFloorLocations(int32 Count);

	/**
	 * Returns all floor locations in the maze in world space.
	 * @return Array of world space positions of all floor cells
	 */
	UFUNCTION(BlueprintCallable, Category="Maze")
	virtual TArray<FVector> GetAllFloorLocations();

	/**
	 * Returns the maze endpoint position and rotation in world space.
	 * This is the target location where the player should reach to win.
	 * @return Transform with position at the endpoint
	 */
	UFUNCTION(BlueprintCallable, Category="Maze")
	virtual FTransform GetMazeEndpointTransform();

	/**
	 * Spawns the endpoint actor at the maze endpoint location.
	 * Returns the spawned actor, or nullptr if failed.
	 * @return Pointer to the spawned endpoint actor
	 */
	UFUNCTION(BlueprintCallable, Category="Maze")
	virtual AActor* SpawnEndpointActor();

	/**
	 * Returns a random floor position as a spawn point for the player.
	 * @return Transform with a random floor position
	 */
	UFUNCTION(BlueprintCallable, Category="Maze")
	virtual FTransform GetRandomSpawnTransform();

	/**
	 * Returns random floor locations excluding already used positions.
	 * Useful for spawning enemies without overlap with player/endpoint.
	 * @param Count - Number of random locations to return
	 * @param ExcludePositions - Array of world positions to exclude (player, endpoint, etc.)
	 * @param ExclusionRadius - Radius around excluded positions to avoid
	 * @return Array of world space positions that don't overlap with excluded positions
	 */
	UFUNCTION(BlueprintCallable, Category="Maze")
	virtual TArray<FVector> GetRandomFloorLocationsExcluding(int32 Count, const TArray<FVector>& ExcludePositions, float ExclusionRadius = 100.0f);

	/**
	 * Returns the path start position and rotation in world space.
	 * Rotation is calculated to face into the maze from the entrance.
	 * @return Transform with position and rotation facing into the maze
	 */
	UFUNCTION(BlueprintCallable, Category="Maze")
	virtual FTransform GetPathStartTransform();

	/**
	 * Returns the path end position and rotation in world space.
	 * Rotation is calculated to face into the maze from the exit.
	 * @return Transform with position and rotation facing into the maze
	 */
	UFUNCTION(BlueprintCallable, Category="Maze")
	virtual FTransform GetPathEndTransform();

	/**
	 * Updates Maze every time any parameter has been changed(except transform).
	 *
	 * Remember: this method is being called before BeginPlay.
	 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/**
	 * Called when the game starts or when spawned.
	 * Regenerates the maze at runtime.
	 */
	virtual void BeginPlay() override;

	/**
	 * Returns path grid mapped into MazeGrid constrains. Creates a graph every time it is called.
	 *
	 * Note :
	 * 
	 * Optimization is possible:
	 * if the beginning or end has not changed, there is actually no need to create a new graph,
	 * but due to the many parameters that can be changed, it is difficult to determine what exactly has changed,
	 * so this optimization has been neglected.
	 */
	virtual TArray<TArray<uint8>> GetMazePath(const FMazeCoordinates& Start, const FMazeCoordinates& End,
	                                          int32& OutLength);
protected:
	/**
	 * Generate Maze with random size, seed and 
	 * algorithm with path connecting top-left and bottom-right corners.
	 */
	UFUNCTION(CallInEditor, Category="Maze", meta=(DisplayPriority=0, ShortTooltip = "Generate an arbitrary maze."))
	virtual void Randomize();

	virtual void CreateMazeOutline() const;

	virtual void EnableCollision(const bool bShouldEnable);

	// Clears all HISM instances.
	virtual void ClearMaze();

	virtual FVector2D GetMaxCellSize() const;


#if WITH_EDITOR
private:
	FTransform LastMazeTransform;
#endif
};
