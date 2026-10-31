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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPaperSpriteComponent* TankSpriteComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class UCameraComponent* FollowCamera;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteUp = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteDown = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteLeft = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteRight = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    float GridSize = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    FVector2D GridOrigin = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    float TurnAssistDistance = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TSubclassOf<class AActor> BulletClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float FireCooldown = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    int32 MaxHealth = 3;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
    int32 Health = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float HitInvincibleTime = 1.0f;

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

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ActivateDoubleShot(float Duration);

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool HasDoubleShot() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float DoubleShotSpacing = 24.0f;

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void ShowGameOverUI();

private:
    void Fire();
    void Die();
    void RestartLevel();
    void ShowHealth() const;
    void TryTurnAssist(const FVector& MoveDirection, float DeltaTime);
    void UpdateFacingSprite(const FVector& Direction);

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