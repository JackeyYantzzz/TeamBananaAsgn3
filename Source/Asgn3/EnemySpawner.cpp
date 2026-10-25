#include "EnemySpawner.h"
#include "EnemyTankBase.h"
#include "TankBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

// Key for the on-screen kill counter so it updates in place
static const uint64 KillsMessageKey = 0x7A4E1003;

AEnemySpawner::AEnemySpawner()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AEnemySpawner::BeginPlay()
{
    Super::BeginPlay();

    // The player uses the same map bounds as the enemies, so it cannot drive off the map either
    if (ATankBase* Player = Cast<ATankBase>(UGameplayStatics::GetActorOfClass(this, ATankBase::StaticClass())))
    {
        const FVector Origin = GetActorLocation();
        const FVector2D AreaMin(Origin.X - SpawnAreaRange.X, Origin.Z - SpawnAreaRange.Z);
        const FVector2D AreaMax(Origin.X + SpawnAreaRange.X, Origin.Z + SpawnAreaRange.Z);
        // Keep the tank centre half a tile inside the edge (same as the enemies)
        const double Half = SpawnGridSize > 0.0f ? SpawnGridSize * 0.5 : 20.0;
        Player->SetPlayArea(AreaMin + FVector2D(Half, Half), AreaMax - FVector2D(Half, Half));
    }

    ShowKills();

    // Spawn one right away, then keep spawning every SpawnInterval seconds
    SpawnEnemy();
    GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &AEnemySpawner::SpawnEnemy, SpawnInterval, true);
}

void AEnemySpawner::SpawnEnemy()
{
    if (!EnemyClass || bGameWon)
    {
        return;
    }

    // Forget enemies that have been destroyed, then respect the alive limit
    AliveEnemies.RemoveAll([](const TWeakObjectPtr<AEnemyTankBase>& Enemy) { return !Enemy.IsValid(); });
    if (AliveEnemies.Num() >= MaxAliveEnemies)
    {
        return;
    }

    // Stop spawning once the player is dead
    ATankBase* Player = Cast<ATankBase>(UGameplayStatics::GetActorOfClass(this, ATankBase::StaticClass()));
    if (Player && Player->IsDead())
    {
        return;
    }

    const FVector Origin = GetActorLocation();
    const FVector2D AreaMin(Origin.X - SpawnAreaRange.X, Origin.Z - SpawnAreaRange.Z);
    const FVector2D AreaMax(Origin.X + SpawnAreaRange.X, Origin.Z + SpawnAreaRange.Z);

    FActorSpawnParameters SpawnParams;
    // Skip spots that overlap walls, crates or other tanks; we simply try another random spot
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

    const bool bUseGrid = SpawnGridSize > 0.0f;
    const int32 TilesX = bUseGrid ? FMath::Max(1, FMath::FloorToInt((AreaMax.X - AreaMin.X) / SpawnGridSize)) : 0;
    const int32 TilesZ = bUseGrid ? FMath::Max(1, FMath::FloorToInt((AreaMax.Y - AreaMin.Y) / SpawnGridSize)) : 0;

    for (int32 Attempt = 0; Attempt < MaxSpawnAttempts; ++Attempt)
    {
        FVector SpawnLocation;
        if (bUseGrid)
        {
            // Random free tile centre
            SpawnLocation = FVector(
                AreaMin.X + (FMath::RandRange(0, TilesX - 1) + 0.5) * SpawnGridSize,
                Origin.Y,
                AreaMin.Y + (FMath::RandRange(0, TilesZ - 1) + 0.5) * SpawnGridSize);
        }
        else
        {
            SpawnLocation = FVector(
                FMath::RandRange(AreaMin.X, AreaMax.X),
                Origin.Y,
                FMath::RandRange(AreaMin.Y, AreaMax.Y));
        }

        if (Player && FVector::Dist(SpawnLocation, Player->GetActorLocation()) < MinDistanceToPlayer)
        {
            continue;
        }

        SpawnParams.Owner = this; // lets the enemy report its death back to this spawner
        AEnemyTankBase* Enemy = GetWorld()->SpawnActor<AEnemyTankBase>(EnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        if (Enemy)
        {
            if (bUseGrid)
            {
                // Keep tank centres between the first and last tile centre so tanks stay inside the map
                const FVector2D Half(SpawnGridSize * 0.5, SpawnGridSize * 0.5);
                Enemy->SetPlayArea(AreaMin + Half, AreaMax - Half);
                Enemy->SetGrid(AreaMin, SpawnGridSize);
            }
            else
            {
                Enemy->SetPlayArea(AreaMin, AreaMax);
            }
            AliveEnemies.Add(Enemy);
            return;
        }
    }
}

void AEnemySpawner::NotifyEnemyKilled(AEnemyTankBase* Enemy)
{
    if (bGameWon)
    {
        return;
    }

    KillCount++;
    ShowKills();

    if (KillsToWin > 0 && KillCount >= KillsToWin)
    {
        WinGame();
        return;
    }

    if (KillsPerPickup > 0 && KillCount % KillsPerPickup == 0)
    {
        SpawnPickup();
    }
}

void AEnemySpawner::ShowKills() const
{
    if (!GEngine)
    {
        return;
    }
    const FString Text = KillsToWin > 0
        ? FString::Printf(TEXT("KILLS: %d / %d"), KillCount, KillsToWin)
        : FString::Printf(TEXT("KILLS: %d"), KillCount);
    GEngine->AddOnScreenDebugMessage(KillsMessageKey, 100000.0f, FColor::Cyan, Text, true, FVector2D(2.0f, 2.0f));
}

void AEnemySpawner::WinGame()
{
    bGameWon = true;
    GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

    // Clear the remaining enemies (they no longer count)
    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsOfClass(this, AEnemyTankBase::StaticClass(), Enemies);
    for (AActor* Enemy : Enemies)
    {
        if (Enemy)
        {
            Enemy->Destroy();
        }
    }
    AliveEnemies.Reset();

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(KillsMessageKey + 1, WinRestartDelay + 1.0f, FColor::Yellow,
            FString::Printf(TEXT("YOU WIN!  %d enemies destroyed"), KillCount), true, FVector2D(3.0f, 3.0f));
    }

    GetWorld()->GetTimerManager().SetTimer(WinRestartTimerHandle, this, &AEnemySpawner::RestartLevel, WinRestartDelay, false);
}

void AEnemySpawner::RestartLevel()
{
    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

bool AEnemySpawner::FindFreeTile(float HalfSize, FVector& OutLocation) const
{
    const FVector Origin = GetActorLocation();
    const FVector2D AreaMin(Origin.X - SpawnAreaRange.X, Origin.Z - SpawnAreaRange.Z);
    const FVector2D AreaMax(Origin.X + SpawnAreaRange.X, Origin.Z + SpawnAreaRange.Z);
    const float Grid = SpawnGridSize > 0.0f ? SpawnGridSize : 40.0f;
    const int32 TilesX = FMath::Max(1, FMath::FloorToInt((AreaMax.X - AreaMin.X) / Grid));
    const int32 TilesZ = FMath::Max(1, FMath::FloorToInt((AreaMax.Y - AreaMin.Y) / Grid));

    auto TileCenter = [&](int32 IX, int32 IZ)
    {
        return FVector(AreaMin.X + (IX + 0.5) * Grid, Origin.Y, AreaMin.Y + (IZ + 0.5) * Grid);
    };

    // Tanks move around, so they must not count as walls when deciding which tiles are open
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PickupTileQuery), false);
    TArray<AActor*> Tanks;
    UGameplayStatics::GetAllActorsOfClass(this, ATankBase::StaticClass(), Tanks);
    QueryParams.AddIgnoredActors(Tanks);
    Tanks.Reset();
    UGameplayStatics::GetAllActorsOfClass(this, AEnemyTankBase::StaticClass(), Tanks);
    QueryParams.AddIgnoredActors(Tanks);

    // 1. Which tiles have no crate / wall in them
    const FCollisionShape Box = FCollisionShape::MakeBox(FVector(HalfSize));
    TArray<bool> Free;
    Free.SetNumZeroed(TilesX * TilesZ);
    for (int32 IZ = 0; IZ < TilesZ; ++IZ)
    {
        for (int32 IX = 0; IX < TilesX; ++IX)
        {
            Free[IZ * TilesX + IX] = !GetWorld()->OverlapBlockingTestByChannel(
                TileCenter(IX, IZ), FQuat::Identity, ECC_WorldDynamic, Box, QueryParams);
        }
    }

    // 2. Only tiles the player can actually drive to (flood fill from the player's tile),
    //    so a pickup never appears inside a closed area such as the base
    ATankBase* Player = Cast<ATankBase>(UGameplayStatics::GetActorOfClass(this, ATankBase::StaticClass()));
    TArray<int32> Candidates;
    if (Player)
    {
        const FVector P = Player->GetActorLocation();
        const int32 StartX = FMath::Clamp(FMath::FloorToInt((P.X - AreaMin.X) / Grid), 0, TilesX - 1);
        const int32 StartZ = FMath::Clamp(FMath::FloorToInt((P.Z - AreaMin.Y) / Grid), 0, TilesZ - 1);
        const int32 StartIndex = StartZ * TilesX + StartX;

        TArray<bool> Visited;
        Visited.SetNumZeroed(TilesX * TilesZ);
        TArray<int32> Queue;
        Queue.Add(StartIndex);
        Visited[StartIndex] = true;
        for (int32 Head = 0; Head < Queue.Num(); ++Head)
        {
            const int32 Index = Queue[Head];
            const int32 IX = Index % TilesX;
            const int32 IZ = Index / TilesX;
            if (Index != StartIndex)
            {
                Candidates.Add(Index); // never on the player's own tile
            }
            const int32 Neighbours[4][2] = { {IX + 1, IZ}, {IX - 1, IZ}, {IX, IZ + 1}, {IX, IZ - 1} };
            for (const auto& N : Neighbours)
            {
                if (N[0] < 0 || N[0] >= TilesX || N[1] < 0 || N[1] >= TilesZ)
                {
                    continue;
                }
                const int32 NIndex = N[1] * TilesX + N[0];
                if (!Visited[NIndex] && Free[NIndex])
                {
                    Visited[NIndex] = true;
                    Queue.Add(NIndex);
                }
            }
        }
    }
    else
    {
        for (int32 Index = 0; Index < Free.Num(); ++Index)
        {
            if (Free[Index])
            {
                Candidates.Add(Index);
            }
        }
    }

    if (Candidates.Num() == 0)
    {
        return false;
    }

    const int32 Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
    OutLocation = TileCenter(Chosen % TilesX, Chosen / TilesX);
    return true;
}

void AEnemySpawner::SpawnPickup()
{
    // Pick one of the configured pickup types at random
    TArray<TSubclassOf<AActor>> Valid;
    for (const TSubclassOf<AActor>& PickupClass : PickupClasses)
    {
        if (PickupClass)
        {
            Valid.Add(PickupClass);
        }
    }
    if (Valid.Num() == 0)
    {
        return;
    }

    FVector Location;
    const float HalfSize = (SpawnGridSize > 0.0f ? SpawnGridSize : 40.0f) * 0.4f;
    if (!FindFreeTile(HalfSize, Location))
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    GetWorld()->SpawnActor<AActor>(Valid[FMath::RandRange(0, Valid.Num() - 1)], Location, FRotator::ZeroRotator, Params);
}
