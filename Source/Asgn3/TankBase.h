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

    // --- Facing sprites ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteUp = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteDown = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteLeft = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteRight = nullptr;

    // --- Movement ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 200.0f;

    // Size of one map tile. 0 disables turn assist.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    float GridSize = 0.0f;

    // X and Z coordinates of the tile grid origin.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    FVector2D GridOrigin = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    float TurnAssistDistance = 12.0f;

    // --- Combat ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TSubclassOf<class AActor> BulletClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float FireCooldown = 0.5f;

    // --- Health ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    int32 MaxHealth = 3;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
    int32 Health = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float HitInvincibleTime = 1.0f;

    // Retained for compatibility.
    // Death no longer schedules an automatic restart.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float RestartDelay = 3.0f;

    UFUNCTION(BlueprintCallable, Category = "Health")
    void TakeHit(AActor* DamageSource);

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsDead() const { return bIsDead; }

    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetPlayArea(FVector2D InMinXZ, FVector2D InMaxXZ);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void Heal(int32 Amount);

    // --- Double shot power-up ---
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ActivateDoubleShot(float Duration);

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool HasDoubleShot() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float DoubleShotSpacing = 24.0f;

    // Implement this event in the player tank Blueprint.
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void ShowGameOverUI();

private:
    void Fire();
    void Die();
    void RestartLevel();
    void ShowHealth() const;
    void TryTurnAssist(const FVector& MoveDirection, float DeltaTime);
    void UpdateFacingSprite(const FVector& Direction);

    // The most recently pressed axis wins when both axes are held.
    bool bPreferVertical = true;

    bool bHasPlayArea = false;
    FVector2D PlayAreaMin = FVector2D::ZeroVector;
    FVector2D PlayAreaMax = FVector2D::ZeroVector;

    bool IsInvincible() const;

    float LastFireTime = 0.0f;
    float LastHitTime = -1000.0f;
    float DoubleShotEndTime = -1000.0f;

    void ShowDoubleShot() const;

    bool bIsDead = false;
    FTimerHandle RestartTimerHandle;

    FVector LastMoveDirection = FVector(0.0f, 0.0f, 1.0f);
};