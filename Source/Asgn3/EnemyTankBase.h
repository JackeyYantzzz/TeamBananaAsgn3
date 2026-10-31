#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyTankBase.generated.h"

UCLASS()
class ASGN3_API AEnemyTankBase : public AActor
{
    GENERATED_BODY()

public:
    AEnemyTankBase();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPaperSpriteComponent* TankSpriteComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 Health = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteUp = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteDown = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteLeft = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites")
    class UPaperSprite* SpriteRight = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MinDirectionTime = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MaxDirectionTime = 4.0f;

    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetPlayArea(FVector2D InMinXZ, FVector2D InMaxXZ);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    float GridSize = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    FVector2D GridOrigin = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grid")
    float TurnAssistDistance = 12.0f;

    UFUNCTION(BlueprintCallable, Category = "Movement|Grid")
    void SetGrid(FVector2D InOriginXZ, float InGridSize);

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

private:
    void ChooseNewRandomDirection();
    void UpdateFacingSprite();
    bool IsInsidePlayArea(const FVector& Location) const;
    bool TryTurnAssist(float DeltaTime);
    bool SnapToTileCenterIfClose(float DeltaTime);

    float DirectionChangeTimer = 0.0f;
    FVector CurrentMoveDirection = FVector::ZeroVector;

    bool bHasPlayArea = false;
    FVector2D PlayAreaMin = FVector2D::ZeroVector;
    FVector2D PlayAreaMax = FVector2D::ZeroVector;
};
