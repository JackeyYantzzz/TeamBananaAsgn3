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

static const uint64 HealthMessageKey = 0x7A4E1001;
static const uint64 DoubleShotMessageKey = 0x7A4E1002;

ATankBase::ATankBase()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(
        TEXT("CollisionBox")
    );

    RootComponent = CollisionBox;
    CollisionBox->SetBoxExtent(FVector(20.0f, 20.0f, 20.0f));
    CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    CollisionBox->SetGenerateOverlapEvents(true);

    TankSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(
        TEXT("TankSpriteComponent")
    );

    TankSpriteComponent->SetupAttachment(RootComponent);

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(
        TEXT("CameraBoom")
    );

    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->SetUsingAbsoluteRotation(true);
    CameraBoom->TargetArmLength = 2000.0f;
    CameraBoom->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
    CameraBoom->bDoCollisionTest = false;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(
        TEXT("FollowCamera")
    );

    FollowCamera->SetupAttachment(
        CameraBoom,
        USpringArmComponent::SocketName
    );

    FollowCamera->bUsePawnControlRotation = false;
}

void ATankBase::BeginPlay()
{
    Super::BeginPlay();

    Health = MaxHealth;
    bIsDead = false;

    ShowHealth();
    UpdateFacingSprite(LastMoveDirection);

    APlayerController* PC = UGameplayStatics::GetPlayerController(
        this,
        0
    );

    if (PC)
    {
        EnableInput(PC);
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

    if (TankSpriteComponent)
    {
        const bool bVisible =
            !IsInvincible() ||
            FMath::Fmod(GetWorld()->GetTimeSeconds(), 0.2f) < 0.1f;

        TankSpriteComponent->SetVisibility(bVisible);
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(
        this,
        0
    );

    if (PC)
    {
        float ForwardValue = 0.0f;

        if (PC->IsInputKeyDown(EKeys::W) ||
            PC->IsInputKeyDown(EKeys::Up))
        {
            ForwardValue += 1.0f;
        }

        if (PC->IsInputKeyDown(EKeys::S) ||
            PC->IsInputKeyDown(EKeys::Down))
        {
            ForwardValue -= 1.0f;
        }

        float RightValue = 0.0f;

        if (PC->IsInputKeyDown(EKeys::D) ||
            PC->IsInputKeyDown(EKeys::Right))
        {
            RightValue += 1.0f;
        }

        if (PC->IsInputKeyDown(EKeys::A) ||
            PC->IsInputKeyDown(EKeys::Left))
        {
            RightValue -= 1.0f;
        }

        if (PC->WasInputKeyJustPressed(EKeys::W) ||
            PC->WasInputKeyJustPressed(EKeys::S) ||
            PC->WasInputKeyJustPressed(EKeys::Up) ||
            PC->WasInputKeyJustPressed(EKeys::Down))
        {
            bPreferVertical = true;
        }

        if (PC->WasInputKeyJustPressed(EKeys::A) ||
            PC->WasInputKeyJustPressed(EKeys::D) ||
            PC->WasInputKeyJustPressed(EKeys::Left) ||
            PC->WasInputKeyJustPressed(EKeys::Right))
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

        FVector MoveDirection = FVector(
            RightValue,
            0.0f,
            ForwardValue
        ).GetSafeNormal();

        if (!MoveDirection.IsZero())
        {
            LastMoveDirection = MoveDirection;
            UpdateFacingSprite(MoveDirection);

            FVector NewLocation =
                GetActorLocation() +
                (MoveDirection * MoveSpeed * DeltaTime);

            if (bHasPlayArea)
            {
                NewLocation.X = FMath::Clamp(
                    NewLocation.X,
                    PlayAreaMin.X,
                    PlayAreaMax.X
                );

                NewLocation.Z = FMath::Clamp(
                    NewLocation.Z,
                    PlayAreaMin.Y,
                    PlayAreaMax.Y
                );
            }

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
                    TryTurnAssist(MoveDirection, DeltaTime);
                }
            }
        }

        if (PC->IsInputKeyDown(EKeys::SpaceBar))
        {
            Fire();
        }
    }
}

void ATankBase::TryTurnAssist(
    const FVector& MoveDirection,
    float DeltaTime
)
{
    if (GridSize <= 0.0f)
    {
        return;
    }

    const bool bMovingX = FMath::Abs(MoveDirection.X) > 0.9f;
    const bool bMovingZ = FMath::Abs(MoveDirection.Z) > 0.9f;

    if (bMovingX == bMovingZ)
    {
        return;
    }

    const FVector Location = GetActorLocation();

    const double Coord = bMovingX ? Location.Z : Location.X;
    const double Origin = bMovingX ? GridOrigin.Y : GridOrigin.X;

    const double Diff =
        TankGrid::TileCenter(Coord, Origin, GridSize) - Coord;

    if (FMath::Abs(Diff) > KINDA_SMALL_NUMBER &&
        FMath::Abs(Diff) <= TurnAssistDistance)
    {
        const double Step =
            FMath::Sign(Diff) *
            FMath::Min(
                FMath::Abs(Diff),
                static_cast<double>(MoveSpeed * DeltaTime)
            );

        const FVector Nudge = bMovingX
            ? FVector(0.0, 0.0, Step)
            : FVector(Step, 0.0, 0.0);

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
        NewSprite = Direction.X >= 0.0f
            ? SpriteRight
            : SpriteLeft;
    }
    else
    {
        NewSprite = Direction.Z >= 0.0f
            ? SpriteUp
            : SpriteDown;
    }

    if (NewSprite &&
        TankSpriteComponent->GetSprite() != NewSprite)
    {
        TankSpriteComponent->SetSprite(NewSprite);
    }
}

bool ATankBase::IsInvincible() const
{
    return GetWorld() &&
        (GetWorld()->GetTimeSeconds() - LastHitTime) <
        HitInvincibleTime;
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

void ATankBase::SetPlayArea(
    FVector2D InMinXZ,
    FVector2D InMaxXZ
)
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

    DoubleShotEndTime =
        GetWorld()->GetTimeSeconds() + Duration;
}

bool ATankBase::HasDoubleShot() const
{
    return GetWorld() &&
        GetWorld()->GetTimeSeconds() < DoubleShotEndTime;
}

void ATankBase::ShowDoubleShot() const
{
    if (!GEngine || !GetWorld())
    {
        return;
    }

    const float Remaining =
        DoubleShotEndTime - GetWorld()->GetTimeSeconds();

    if (Remaining > 0.0f)
    {
        GEngine->AddOnScreenDebugMessage(
            DoubleShotMessageKey,
            0.1f,
            FColor::Yellow,
            FString::Printf(
                TEXT("DOUBLE SHOT %.1fs"),
                Remaining
            ),
            true,
            FVector2D(2.0f, 2.0f)
        );
    }
}

void ATankBase::Die()
{
    if (bIsDead)
    {
        return;
    }

    bIsDead = true;

    if (TankSpriteComponent)
    {
        TankSpriteComponent->SetVisibility(false);
    }

    SetActorEnableCollision(false);

    GetWorld()->GetTimerManager().ClearTimer(
        RestartTimerHandle
    );

    if (GEngine)
    {
        GEngine->RemoveOnScreenDebugMessage(
            HealthMessageKey
        );

        GEngine->RemoveOnScreenDebugMessage(
            DoubleShotMessageKey
        );
    }

    ShowGameOverUI();
}

void ATankBase::RestartLevel()
{
    UGameplayStatics::OpenLevel(
        this,
        FName(
            *UGameplayStatics::GetCurrentLevelName(
                this,
                true
            )
        )
    );
}

void ATankBase::ShowHealth() const
{
    if (GEngine)
    {
        const FColor Color = Health > 1
            ? FColor::Green
            : FColor::Red;

        GEngine->AddOnScreenDebugMessage(
            HealthMessageKey,
            100000.0f,
            Color,
            FString::Printf(
                TEXT("HP: %d / %d"),
                Health,
                MaxHealth
            ),
            true,
            FVector2D(2.0f, 2.0f)
        );
    }
}

void ATankBase::Fire()
{
    const float CurrentTime =
        GetWorld()->GetTimeSeconds();

    if (CurrentTime - LastFireTime < FireCooldown)
    {
        return;
    }

    LastFireTime = CurrentTime;

    if (BulletClass)
    {
        const FVector SpawnLocation =
            GetActorLocation() +
            (LastMoveDirection * 40.0f);

        const FRotator SpawnRotation =
            LastMoveDirection.Rotation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        GetWorld()->SpawnActor<AActor>(
            BulletClass,
            SpawnLocation,
            SpawnRotation,
            SpawnParams
        );

        if (HasDoubleShot())
        {
            GetWorld()->SpawnActor<AActor>(
                BulletClass,
                SpawnLocation +
                    LastMoveDirection * DoubleShotSpacing,
                SpawnRotation,
                SpawnParams
            );
        }
    }
}