#include "TankBase.h"
#include "EnemyTankBase.h"
#include "TankGridUtils.h"
#include "Components/BoxComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

// Key used for the on-screen health message so it is updated in place instead of stacking
static const uint64 HealthMessageKey = 0x7A4E1001;
static const uint64 DoubleShotMessageKey = 0x7A4E1002;

ATankBase::ATankBase()
{
    PrimaryActorTick.bCanEverTick = true;

    // Setup root collision box
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetBoxExtent(FVector(20.0f, 20.0f, 20.0f));
    // Block other solid actors (e.g. wood crates) so the tank cannot drive through them.
    // Bullets use OverlapAllDynamic, so they still only overlap the tank.
    CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    CollisionBox->SetGenerateOverlapEvents(true);

    // Setup sprite component
    TankSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("TankSpriteComponent"));
    TankSpriteComponent->SetupAttachment(RootComponent);

    // Setup Camera Boom (Spring Arm) for a fixed full-map view
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->SetUsingAbsoluteRotation(true); // Prevent arm from rotating when tank rotates
    CameraBoom->TargetArmLength = 2000.0f; // Distance from the map to embrace the entire view
    CameraBoom->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f)); // Face the vertical map directly
    CameraBoom->bDoCollisionTest = false; // Disable collision test

    // Setup Follow Camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void ATankBase::BeginPlay()
{
    Super::BeginPlay();

    Health = MaxHealth;
    bIsDead = false;
    ShowHealth();
    UpdateFacingSprite(LastMoveDirection);

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        EnableInput(PC);
        // Automatically set view target to this tank (which carries the fixed camera)
        PC->SetViewTarget(this);
    }
}

void ATankBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDead)
    {
        return;
    }

    ShowDoubleShot();

    // Blink while invincible after a hit
    if (TankSpriteComponent)
    {
        const bool bVisible = !IsInvincible() || FMath::Fmod(GetWorld()->GetTimeSeconds(), 0.2f) < 0.1f;
        TankSpriteComponent->SetVisibility(bVisible);
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        // 1. Get keyboard inputs for W/S and A/D
        float ForwardValue = 0.0f;
        if (PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up))   ForwardValue += 1.0f;
        if (PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down)) ForwardValue -= 1.0f;

        float RightValue = 0.0f;
        if (PC->IsInputKeyDown(EKeys::D) || PC->IsInputKeyDown(EKeys::Right)) RightValue += 1.0f;
        if (PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Left))  RightValue -= 1.0f;

        // Battle City style: only up / down / left / right. If keys on both axes are held,
        // the axis of the most recently pressed key wins.
        if (PC->WasInputKeyJustPressed(EKeys::W) || PC->WasInputKeyJustPressed(EKeys::S) ||
            PC->WasInputKeyJustPressed(EKeys::Up) || PC->WasInputKeyJustPressed(EKeys::Down))
        {
            bPreferVertical = true;
        }
        if (PC->WasInputKeyJustPressed(EKeys::A) || PC->WasInputKeyJustPressed(EKeys::D) ||
            PC->WasInputKeyJustPressed(EKeys::Left) || PC->WasInputKeyJustPressed(EKeys::Right))
        {
            bPreferVertical = false;
        }
        if (RightValue != 0.0f && ForwardValue != 0.0f)
        {
            if (bPreferVertical)
            {
                RightValue = 0.0f;
            }
            else
            {
                ForwardValue = 0.0f;
            }
        }

        // 2. Map D/A to Red Arrow (X-axis) and W/S to Blue Arrow (Z-axis)
        FVector MoveDirection = FVector(RightValue, 0.0f, ForwardValue).GetSafeNormal();

        if (!MoveDirection.IsZero())
        {
            LastMoveDirection = MoveDirection; // Cache direction for bullet spawning
            UpdateFacingSprite(MoveDirection);
            FVector NewLocation = GetActorLocation() + (MoveDirection * MoveSpeed * DeltaTime);

            // Never leave the map: stop at its edge
            if (bHasPlayArea)
            {
                NewLocation.X = FMath::Clamp(NewLocation.X, PlayAreaMin.X, PlayAreaMax.X);
                NewLocation.Z = FMath::Clamp(NewLocation.Z, PlayAreaMin.Y, PlayAreaMax.Y);
            }

            // Sweep so walls / crates / enemies block the tank; driving into an enemy also hurts
            FHitResult Hit;
            SetActorLocation(NewLocation, true, &Hit);
            if (Hit.bBlockingHit)
            {
                if (Cast<AEnemyTankBase>(Hit.GetActor()))
                {
                    TakeHit(Hit.GetActor());
                }
                else
                {
                    // Blocked by a wall: help the player line up with the corridor
                    TryTurnAssist(MoveDirection, DeltaTime);
                }
            }
        }

        // 3. Detect Spacebar input to fire bullets
        if (PC->IsInputKeyDown(EKeys::SpaceBar))
        {
            Fire();
        }
    }
}

void ATankBase::TryTurnAssist(const FVector& MoveDirection, float DeltaTime)
{
    if (GridSize <= 0.0f)
    {
        return;
    }

    // Only for straight up/down/left/right movement
    const bool bMovingX = FMath::Abs(MoveDirection.X) > 0.9f;
    const bool bMovingZ = FMath::Abs(MoveDirection.Z) > 0.9f;
    if (bMovingX == bMovingZ)
    {
        return;
    }

    const FVector Location = GetActorLocation();
    // The perpendicular axis is the one that has to line up with the corridor centre
    const double Coord  = bMovingX ? Location.Z : Location.X;
    const double Origin = bMovingX ? GridOrigin.Y : GridOrigin.X;
    const double Diff = TankGrid::TileCenter(Coord, Origin, GridSize) - Coord;

    if (FMath::Abs(Diff) > KINDA_SMALL_NUMBER && FMath::Abs(Diff) <= TurnAssistDistance)
    {
        const double Step = FMath::Sign(Diff) * FMath::Min(FMath::Abs(Diff), (double)(MoveSpeed * DeltaTime));
        const FVector Nudge = bMovingX ? FVector(0.0, 0.0, Step) : FVector(Step, 0.0, 0.0);
        SetActorLocation(Location + Nudge, true);
    }
}

void ATankBase::UpdateFacingSprite(const FVector& Direction)
{
    if (!TankSpriteComponent)
    {
        return;
    }

    UPaperSprite* NewSprite = nullptr;
    if (FMath::Abs(Direction.X) >= FMath::Abs(Direction.Z))
    {
        NewSprite = Direction.X >= 0.0f ? SpriteRight : SpriteLeft;
    }
    else
    {
        NewSprite = Direction.Z >= 0.0f ? SpriteUp : SpriteDown;
    }

    // Keep the current sprite if that direction has none assigned
    if (NewSprite && TankSpriteComponent->GetSprite() != NewSprite)
    {
        TankSpriteComponent->SetSprite(NewSprite);
    }
}

bool ATankBase::IsInvincible() const
{
    return GetWorld() && (GetWorld()->GetTimeSeconds() - LastHitTime) < HitInvincibleTime;
}

void ATankBase::TakeHit(AActor* DamageSource)
{
    if (bIsDead || IsInvincible())
    {
        return;
    }

    LastHitTime = GetWorld()->GetTimeSeconds();
    Health = FMath::Max(0, Health - 1);
    ShowHealth();

    if (Health <= 0)
    {
        Die();
    }
}

void ATankBase::SetPlayArea(FVector2D InMinXZ, FVector2D InMaxXZ)
{
    PlayAreaMin = InMinXZ;
    PlayAreaMax = InMaxXZ;
    bHasPlayArea = true;
}

void ATankBase::Heal(int32 Amount)
{
    if (bIsDead || Amount <= 0)
    {
        return;
    }
    Health = FMath::Min(MaxHealth, Health + Amount);
    ShowHealth();
}

void ATankBase::ActivateDoubleShot(float Duration)
{
    if (bIsDead || !GetWorld())
    {
        return;
    }
    // Picking up another one while active refreshes the timer
    DoubleShotEndTime = GetWorld()->GetTimeSeconds() + Duration;
}

bool ATankBase::HasDoubleShot() const
{
    return GetWorld() && GetWorld()->GetTimeSeconds() < DoubleShotEndTime;
}

void ATankBase::ShowDoubleShot() const
{
    if (!GEngine || !GetWorld())
    {
        return;
    }
    const float Remaining = DoubleShotEndTime - GetWorld()->GetTimeSeconds();
    if (Remaining > 0.0f)
    {
        GEngine->AddOnScreenDebugMessage(DoubleShotMessageKey, 0.1f, FColor::Yellow,
            FString::Printf(TEXT("DOUBLE SHOT %.1fs"), Remaining), true, FVector2D(2.0f, 2.0f));
    }
}

void ATankBase::Die()
{
    bIsDead = true;

    // Keep the actor (it carries the camera) but hide it and stop all collision
    if (TankSpriteComponent)
    {
        TankSpriteComponent->SetVisibility(false);
    }
    SetActorEnableCollision(false);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(HealthMessageKey, RestartDelay + 1.0f, FColor::Red,
            TEXT("GAME OVER"), true, FVector2D(3.0f, 3.0f));
    }

    GetWorld()->GetTimerManager().SetTimer(RestartTimerHandle, this, &ATankBase::RestartLevel, RestartDelay, false);
}

void ATankBase::RestartLevel()
{
    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void ATankBase::ShowHealth() const
{
    if (GEngine)
    {
        const FColor Color = Health > 1 ? FColor::Green : FColor::Red;
        GEngine->AddOnScreenDebugMessage(HealthMessageKey, 100000.0f, Color,
            FString::Printf(TEXT("HP: %d / %d"), Health, MaxHealth), true, FVector2D(2.0f, 2.0f));
    }
}

void ATankBase::Fire()
{
    // Check fire cooldown
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastFireTime < FireCooldown)
    {
        return;
    }
    LastFireTime = CurrentTime;

    // Spawn a bullet actor if BulletClass is assigned
    if (BulletClass)
    {
        FVector SpawnLocation = GetActorLocation() + (LastMoveDirection * 40.0f);
        FRotator SpawnRotation = LastMoveDirection.Rotation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        GetWorld()->SpawnActor<AActor>(BulletClass, SpawnLocation, SpawnRotation, SpawnParams);

        // Double shot: a second bullet just ahead of the first, in the same lane
        // (side by side would not fit in the one-tile corridors)
        if (HasDoubleShot())
        {
            GetWorld()->SpawnActor<AActor>(BulletClass, SpawnLocation + LastMoveDirection * DoubleShotSpacing,
                SpawnRotation, SpawnParams);
        }
    }
}
