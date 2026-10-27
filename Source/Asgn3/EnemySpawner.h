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
    // Enemy Blueprint class to spawn.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    TSubclassOf<class AEnemyTankBase> EnemyClass;

    // Time between spawns in seconds.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    float SpawnInterval = 2.0f;

    // Half size of the play area on the X-Z plane.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    FVector SpawnAreaRange = FVector(300.0f, 0.0f, 300.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    int32 MaxAliveEnemies = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    float MinDistanceToPlayer = 120.0f;

    // 0 disables grid-based spawning.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    float SpawnGridSize = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
    int32 MaxSpawnAttempts = 15;

    // --- Pickups ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups")
    int32 KillsPerPickup = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups")
    TArray<TSubclassOf<AActor>> PickupClasses;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickups")
    int32 KillCount = 0;

    // --- Game rules ---
    // 0 disables victory.
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Game Rules",
        meta = (ClampMin = "0")
    )
    int32 KillsToWin = 20;

    // Retained for compatibility; automatic restart is disabled.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Rules")
    float WinRestartDelay = 5.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Rules")
    bool bGameWon = false;

    UFUNCTION(BlueprintCallable, Category = "Pickups")
    void NotifyEnemyKilled(class AEnemyTankBase* Enemy);

    // Implement this event in the spawner Blueprint.
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void ShowVictoryUI();

private:
    void SpawnPickup();
    void WinGame();
    void RestartLevel();
    void ShowKills() const;

    bool FindFreeTile(float HalfSize, FVector& OutLocation) const;

    FTimerHandle WinRestartTimerHandle;
    FTimerHandle SpawnTimerHandle;

    TArray<TWeakObjectPtr<class AEnemyTankBase>> AliveEnemies;

    void SpawnEnemy();
};