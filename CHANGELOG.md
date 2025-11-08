# Changelog - Maze Generator Pro

## Version 2.0.0 (Production Release)

### 🎉 Major New Features

#### **Configurable Floor Width**
- Added `FloorWidth` property (1-99) to make hallways wider
- Intelligent wall scaling system that maintains 1-unit thickness
- Walls scale directionally based on orientation (horizontal vs vertical)
- Perfect for creating accessible mazes or varying difficulty

#### **Automatic Endpoint System**
- `bHasEndpoint` - Enable endpoint/goal system
- Automatically selects random edge floor position
- `EndpointActorClass` - Spawn custom actors (flags, portals, treasures)
- `SpawnEndpointActor()` - Blueprint-callable spawn function
- `GetMazeEndpointTransform()` - Get endpoint location and rotation
- Auto-creates exit door when enabled

#### **Collision-Free Spawn System**
- `GetRandomSpawnTransform()` - Spawn player at random location
- `GetRandomFloorLocationsExcluding()` - Spawn enemies without overlap
- Configurable exclusion radius
- Prevents player/endpoint/enemy collisions

#### **Enhanced Floor Location Functions**
- `GetAllFloorLocations()` - Returns ALL physical floor tiles (accounts for FloorWidth)
- `GetRandomFloorLocations()` - Get multiple random spawn points
- Properly scaled for wider floors
- Perfect for spawning pickups, enemies, etc.

### 🔧 Improvements

- Added proper null-checking and validation throughout
- Improved error messages with detailed logging
- Better memory management for spawned actors
- Added `IsValid()` checks for actor lifetime management
- Linux platform support added

### 🐛 Bug Fixes

- Fixed endpoint transform positioning with FloorWidth
- Fixed ClearMaze to properly destroy spawned actors
- Added World validation before spawning
- Fixed const-correctness issues

### 📝 API Changes

**New Properties:**
- `FloorWidth` (int32, default: 1)
- `bHasEndpoint` (bool)
- `MazeEndpoint` (FMazeCoordinates)
- `EndpointActorClass` (TSubclassOf<AActor>)

**New Functions:**
- `AActor* SpawnEndpointActor()`
- `FTransform GetMazeEndpointTransform()`
- `FTransform GetRandomSpawnTransform()`
- `TArray<FVector> GetRandomFloorLocationsExcluding(int32, TArray<FVector>, float)`

**Modified Functions:**
- `GetAllFloorLocations()` - Now accounts for FloorWidth
- `ClearMaze()` - No longer const, destroys endpoint actor

### 🎮 Usage Example

```cpp
// Setup
Maze->bHasEndpoint = true;
Maze->EndpointActorClass = BP_GoalFlag::StaticClass();
Maze->bCreateDoors = true;
Maze->FloorWidth = 3;

// Spawn player
FTransform PlayerSpawn = Maze->GetRandomSpawnTransform();
Player->SetActorTransform(PlayerSpawn);

// Spawn endpoint
AActor* Endpoint = Maze->SpawnEndpointActor();

// Spawn enemies (avoiding player and endpoint)
TArray<FVector> Excluded = {PlayerSpawn.GetLocation(), Endpoint->GetActorLocation()};
TArray<FVector> EnemySpawns = Maze->GetRandomFloorLocationsExcluding(10, Excluded, 300.0f);
```

---

## Version 1.0.0 (Original Release)

### Features
- 7 maze generation algorithms
- Pathfinding support
- Door creation system
- Randomize function
- Blueprint support

### Algorithms Supported
- Recursive Backtracker
- Recursive Division
- Hunt-and-Kill
- Sidewinder
- Kruskal's Algorithm
- Eller's Algorithm
- Prim's Algorithm
