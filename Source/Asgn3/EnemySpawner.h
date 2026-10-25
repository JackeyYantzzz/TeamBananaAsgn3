#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

UCLASS()
class ASGN3_API AEnemySpawner : public AActor
{
    GENERATED_BODY()

public:
    AEnemySpawner();

protected:
    virtual void BeginPlay() override;

public:
    // Enemy blueprint class to spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    TSubclassOf<class AEnemyTankBase> EnemyClass;

    // Time interval between each spawn (seconds)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    float SpawnInterval = 2.0f;

    // Half size of the map area on X and Z, centred on this spawner.
    // Enemies spawn at random free spots inside it and never leave it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    FVector SpawnAreaRange = FVector(300.0f, 0.0f, 300.0f);

    // Maximum number of enemies alive at the same time
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    int32 MaxAliveEnemies = 4;

    // Enemies never spawn closer than this to the player
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    float MinDistanceToPlayer = 120.0f;

    // Tile size of the map grid. When > 0 enemies spawn on tile centres and move along the grid. 0 = free.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    float SpawnGridSize = 0.0f;

    // How many random spots to try per spawn before giving up until the next interval
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    int32 MaxSpawnAttempts = 15;

    // --- Pickups ---
    // A pickup appears after every this many enemies destroyed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups")
    int32 KillsPerPickup = 5;

    // Pickup blueprints to choose from at random
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups")
    TArray<TSubclassOf<AActor>> PickupClasses;

    // Enemies destroyed so far
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickups")
    int32 KillCount = 0;

    // --- Game rules ---
    // Destroy this many enemies to win. Change it any time in the Details panel. 0 = never ends.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Rules", meta = (ClampMin = "0"))
    int32 KillsToWin = 20;

    // Seconds the victory message stays before the level restarts
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Rules")
    float WinRestartDelay = 5.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Rules")
    bool bGameWon = false;

    // Called by an enemy when a bullet destroys it
    UFUNCTION(BlueprintCallable, Category = "Pickups")
    void NotifyEnemyKilled(class AEnemyTankBase* Enemy);

private:
    void SpawnPickup();
    void WinGame();
    void RestartLevel();
    void ShowKills() const;
    FTimerHandle WinRestartTimerHandle;
    bool FindFreeTile(float HalfSize, FVector& OutLocation) const;
    FTimerHandle SpawnTimerHandle;
    TArray<TWeakObjectPtr<class AEnemyTankBase>> AliveEnemies;
    void SpawnEnemy();
};
