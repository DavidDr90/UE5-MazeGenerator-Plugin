# UE5 Maze Generator Pro

<img src="https://media3.giphy.com/media/8FDhJzbSLQvhmDVeTW/giphy.gif?cid=790b76114036e4ee69b9f731981f1a7704870fdb8ff19adc&rid=giphy.gif&ct=g" height="222"/>

**Version 2.0.0** - Advanced procedural maze generation for Unreal Engine 5

## Table of Contents

- [UE5 Maze Generator Pro](#ue5-maze-generator-pro)
  - [Table of Contents](#table-of-contents)
  - [About](#about)
  - [What's New in 2.0](#whats-new-in-20)
  - [Enable Plugin](#enable-plugin)
  - [Quick Start](#quick-start)
  - [Advanced Features](#advanced-features)
    - [Configurable Floor Width](#configurable-floor-width)
    - [Endpoint System](#endpoint-system)
    - [Collision-Free Spawning](#collision-free-spawning)
  - [Blueprint API Reference](#blueprint-api-reference)
  - [Generation Algorithms](#generation-algorithms)
  - [Game Integration Example](#game-integration-example)
  - [Limitations](#limitations)
  - [Notes](#notes)

---

## About

A comprehensive plugin for UE5 that allows you to create procedural mazes with:

- **7 Generation Algorithms** - Different maze styles and characteristics
- **Configurable Floor Width** - Create wider hallways (1-99 units)
- **Automatic Endpoint System** - Random edge-based goal spawning
- **Collision-Free Spawn System** - Smart player/enemy placement
- **Pathfinding Support** - Built-in A* pathfinding
- **Blueprint-Friendly** - All features exposed to Blueprints
- **High Performance** - Instanced static mesh rendering

Perfect for roguelikes, puzzle games, and procedural level design!

---

## What's New in 2.0

### 🎯 Configurable Floor Width
- Adjust hallway width from 1 to 99 units
- Intelligent wall scaling maintains proper thickness
- Perfect for accessibility or difficulty tuning

### 🏁 Endpoint System
- Automatic random edge selection for maze goal
- Spawn custom endpoint actors (flags, portals, treasures)
- Auto-creates exit door at endpoint

### 🎮 Smart Spawning
- Collision-free player spawning
- Enemy spawn system with exclusion zones
- Prevent overlapping spawns automatically

### 📊 Enhanced API
- `GetRandomSpawnTransform()` - Player spawn point
- `SpawnEndpointActor()` - Spawn goal actor
- `GetRandomFloorLocationsExcluding()` - Spawn enemies without collisions
- `GetAllFloorLocations()` - All floor positions (accounts for width)

---

## Enable Plugin

Before enabling plugin ensure you have installed it in engine of appropriate version inside Epic Games launcher:

![](./Images/install_to_engine.png)

---

Open plugins list **Edit->Plugins**:

![](./Images/click_plugins.png)

---

Search for MazeGenerator in search box and check the checkbox against MazeGenerator plugin. Then restart your project.

![](./Images/enable_plugin.png)

---

Make sure plugins are shown in content browser. You can toggle visibility as follows:

![](./Images/show_plugin.png)

## Quick Start

To create your first maze create Blueprint class based on `Maze` class:

![](./Images/create_maze.png)

---

In the newly created Blueprint class select _Class Defaults_ and play with available parameters:

![](./Images/class_defaults.png)

_Note: size can be specified only on instances(i.e. on actors placed on level)_

Set `Floor` and `Wall` Static Meshes under _Details_ panel.

---

Instantiate Maze by dragging Blueprint into level and play with parameters:

![](./Images/place_on_level.png)

---

## Advanced Features

### Configurable Floor Width

Make your maze hallways wider for easier navigation or gameplay variety:

```cpp
// In Blueprint or C++
Maze->FloorWidth = 3;  // Creates 3x3 unit wide hallways
Maze->UpdateMaze();
```

**Properties:**
- `FloorWidth` (int32, 1-99) - Width of hallways in grid units
- Walls automatically scale to maintain proper barriers
- Perfect for different difficulty levels or accessibility

### Endpoint System

Automatically create a goal/exit point in your maze:

```cpp
// Enable endpoint
Maze->bHasEndpoint = true;
Maze->EndpointActorClass = BP_GoalFlag::StaticClass();
Maze->bCreateDoors = true;  // Creates exit door

// Spawn the endpoint actor
AActor* Endpoint = Maze->SpawnEndpointActor();

// Get endpoint transform
FTransform EndpointTransform = Maze->GetMazeEndpointTransform();
```

**Features:**
- Automatically selects random edge floor position
- Spawns custom actor (flag, portal, treasure, etc.)
- Optional exit door creation
- Perfect for win conditions

### Collision-Free Spawning

Prevent player, enemies, and objectives from spawning on top of each other:

```cpp
// 1. Spawn player
FTransform PlayerSpawn = Maze->GetRandomSpawnTransform();
Player->SetActorTransform(PlayerSpawn);

// 2. Spawn endpoint
AActor* Endpoint = Maze->SpawnEndpointActor();

// 3. Build exclusion list
TArray<FVector> ExcludedPositions;
ExcludedPositions.Add(PlayerSpawn.GetLocation());
ExcludedPositions.Add(Endpoint->GetActorLocation());

// 4. Spawn enemies (avoiding player and endpoint)
TArray<FVector> EnemySpawns = Maze->GetRandomFloorLocationsExcluding(
    10,                    // Number of enemies
    ExcludedPositions,     // Positions to avoid
    300.0f                 // Exclusion radius
);

// 5. Spawn enemies
for (const FVector& Location : EnemySpawns)
{
    GetWorld()->SpawnActor<AEnemy>(EnemyClass, Location, FRotator::ZeroRotator);
}
```

---

## Blueprint API Reference

### Maze Generation
- `UpdateMaze()` - Regenerate maze with current settings
- `Randomize()` - Generate random maze with random settings

### Spawn Functions
- `GetRandomSpawnTransform()` - Get random floor position for player
- `SpawnEndpointActor()` - Spawn endpoint actor at goal location
- `GetMazeEndpointTransform()` - Get endpoint position and rotation
- `GetRandomFloorLocations(int32 Count)` - Get N random floor positions
- `GetRandomFloorLocationsExcluding(int32 Count, TArray<FVector> Exclude, float Radius)` - Get N positions avoiding others
- `GetAllFloorLocations()` - Get all floor positions (accounts for FloorWidth)

### Path Functions
- `GetPathStartTransform()` - Get path entrance transform
- `GetPathEndTransform()` - Get path exit transform

### Properties
- `MazeSize` (X, Y) - Maze dimensions
- `Seed` - Random seed for generation
- `GenerationAlgorithm` - Algorithm to use
- `FloorWidth` - Hallway width (1-99)
- `bHasEndpoint` - Enable endpoint system
- `EndpointActorClass` - Actor class to spawn at endpoint
- `bCreateDoors` - Create entrance/exit doors
- `bGeneratePath` - Generate visual path
- `FloorStaticMesh` - Floor mesh
- `WallStaticMesh` - Wall mesh
- `OutlineStaticMesh` - Outline wall mesh
- `PathStaticMesh` - Path floor mesh

---

## Generation Algorithms

- [Recursive Backtracker](http://weblog.jamisbuck.org/2010/12/27/maze-generation-recursive-backtracking.html)
- [Recursive Division](http://weblog.jamisbuck.org/2011/1/12/maze-generation-recursive-division-algorithm)
- [Hunt-and-Kill](http://weblog.jamisbuck.org/2011/1/24/maze-generation-hunt-and-kill-algorithm.html)
- [Sidewinder](http://weblog.jamisbuck.org/2011/2/3/maze-generation-sidewinder-algorithm.html)
- [Kruskal's](http://weblog.jamisbuck.org/2011/1/3/maze-generation-kruskal-s-algorithm.html)
- [Eller's](http://weblog.jamisbuck.org/2010/12/29/maze-generation-eller-s-algorithm.html)
- [Prim's](http://weblog.jamisbuck.org/2011/1/10/maze-generation-prim-s-algorithm.html)

Each algorithm produces different maze characteristics and solving difficulties!

---

## Game Integration Example

Complete example of setting up a maze-based game:

```cpp
// In your GameMode or Level Blueprint

void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Find or spawn maze actor
    AMaze* Maze = GetWorld()->SpawnActor<AMaze>(MazeClass);

    // Configure maze
    Maze->MazeSize = FMazeSize{25, 25};
    Maze->FloorWidth = 3;
    Maze->bHasEndpoint = true;
    Maze->EndpointActorClass = BP_GoalFlag;
    Maze->bCreateDoors = true;
    Maze->UpdateMaze();

    // Spawn player at random location
    FTransform PlayerSpawn = Maze->GetRandomSpawnTransform();
    APlayerCharacter* Player = GetWorld()->SpawnActor<APlayerCharacter>(
        PlayerClass,
        PlayerSpawn
    );

    // Spawn endpoint
    AActor* Endpoint = Maze->SpawnEndpointActor();

    // Spawn enemies avoiding player and endpoint
    TArray<FVector> Excluded;
    Excluded.Add(PlayerSpawn.GetLocation());
    Excluded.Add(Endpoint->GetActorLocation());

    TArray<FVector> EnemySpawns = Maze->GetRandomFloorLocationsExcluding(
        10,      // 10 enemies
        Excluded,
        400.0f   // 4 meter exclusion radius
    );

    for (const FVector& SpawnLoc : EnemySpawns)
    {
        GetWorld()->SpawnActor<AEnemy>(EnemyClass, SpawnLoc, FRotator::ZeroRotator);
    }

    // Store endpoint for win condition
    GoalLocation = Endpoint->GetActorLocation();
}

void AMyGameMode::CheckWinCondition()
{
    float Distance = FVector::Dist(Player->GetActorLocation(), GoalLocation);
    if (Distance < 100.0f)
    {
        // Player wins!
        OnPlayerWin();
    }
}
```

---

## Limitations

- Unreal Engine Reflection System doesn't support 2D arrays, so `MazeGrid` and `MazePathGrid` can't be exposed to Blueprints
- FloorWidth > 9 may impact performance on very large mazes
- Maximum maze size: 9999x9999 (not recommended for performance)
- Endpoint always spawns on edge positions (by design)

If you need direct access to the maze grid or path, inherit from the `Maze` class and extend functionality in C++.

---

## Notes

- All generated mazes are **perfect mazes** (one solution between any two points)
- Plugin content folder includes example `Maze` Blueprint
- Instances of `Maze` have a **Randomize** button in the editor
- Source code: `Plugins/MazeGenerator/Source/MazeGenerator`
- Supports **Win64, Mac, and Linux** platforms
- High performance using **Hierarchical Instanced Static Meshes**

---

## Version History

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.

**Current Version: 2.0.0**
- Configurable floor width
- Endpoint system
- Collision-free spawning
- Enhanced API
- Linux support

---

## Credits

Original plugin by **LowkeyMe**
Enhanced version by **DavidDr90**

---

## License

See [LICENSE.txt](LICENSE.txt) for license information.
