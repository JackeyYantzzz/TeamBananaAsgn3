#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TankBase.generated.h"

UCLASS()
class ASGN3_API ATankBase : public AActor
{
    GENERATED_BODY()

public:
    ATankBase();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // --- Components ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPaperSpriteComponent* TankSpriteComponent;

    // --- Camera Components ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class UCameraComponent* FollowCamera;

    // --- Facing sprites (W / S / A / D) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteUp = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteDown = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteLeft = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteRight = nullptr;

    // --- Movement Properties ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 200.0f;

    // --- Battle City style turn assist ---
    // Size of one map tile (a corridor is one tile wide). 0 disables the assist.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    float GridSize = 0.0f;

    // (X, Z) of the map's bottom-left corner, i.e. where the tile grid starts
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    FVector2D GridOrigin = FVector2D::ZeroVector;

    // When blocked, slide sideways up to this distance to line up with a corridor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    float TurnAssistDistance = 12.0f;

    // --- Combat Properties (Bullet Spawning) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TSubclassOf<class AActor> BulletClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float FireCooldown = 0.5f;

    // --- Health ---
    // Number of enemy collisions the player can take before dying
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    int32 MaxHealth = 3;

    // Current health (reset to MaxHealth on BeginPlay)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
    int32 Health = 3;

    // Seconds of invincibility after being hit, so one collision only costs 1 health
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float HitInvincibleTime = 1.0f;

    // Seconds to wait after death before the level restarts
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float RestartDelay = 3.0f;

    // Called when an enemy tank collides with the player
    UFUNCTION(BlueprintCallable, Category = "Health")
    void TakeHit(AActor* DamageSource);

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsDead() const { return bIsDead; }

    // Keep the tank centre inside this rectangle on the X-Z plane (set by the spawner = the map)
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetPlayArea(FVector2D InMinXZ, FVector2D InMaxXZ);

    // Restore health (Health pickup), never above MaxHealth
    UFUNCTION(BlueprintCallable, Category = "Health")
    void Heal(int32 Amount);

    // --- Double shot power-up ---
    // Every shot fires two bullets in a line until the timer runs out (DoubleShot pickup)
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ActivateDoubleShot(float Duration);

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool HasDoubleShot() const;

    // Distance between the two bullets of a double shot (along the firing direction)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float DoubleShotSpacing = 24.0f;

private:
    void Fire();
    void Die();
    void RestartLevel();
    void ShowHealth() const;
    void TryTurnAssist(const FVector& MoveDirection, float DeltaTime);
    void UpdateFacingSprite(const FVector& Direction);

    // With two direction keys held, the axis of the most recently pressed key wins
    bool bPreferVertical = true;

    bool bHasPlayArea = false;
    FVector2D PlayAreaMin = FVector2D::ZeroVector; // (X, Z)
    FVector2D PlayAreaMax = FVector2D::ZeroVector; // (X, Z)
    bool IsInvincible() const;

    float LastFireTime = 0.0f;
    float LastHitTime = -1000.0f;
    float DoubleShotEndTime = -1000.0f;
    void ShowDoubleShot() const;
    bool bIsDead = false;
    FTimerHandle RestartTimerHandle;
    FVector LastMoveDirection = FVector(0.0f, 0.0f, 1.0f); // Default facing up along Z-axis
};
