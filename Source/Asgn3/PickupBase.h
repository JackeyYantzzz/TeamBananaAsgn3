#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupBase.generated.h"

UENUM(BlueprintType)
enum class EPickupType : uint8
{
    // +1 health for the player (up to MaxHealth)
    Health      UMETA(DisplayName = "Health"),
    // Every shot fires two bullets for a few seconds
    DoubleShot  UMETA(DisplayName = "Double Shot")
};

UCLASS()
class ASGN3_API APickupBase : public AActor
{
    GENERATED_BODY()

public:
    APickupBase();

    // Overlap-only box: tanks drive into it to pick it up
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPaperSpriteComponent* PickupSprite;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    EPickupType PickupType = EPickupType::Health;

    // Health restored by a Health pickup
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    int32 HealAmount = 1;

    // Seconds of double shot given by a DoubleShot pickup
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    float DoubleShotDuration = 5.0f;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);
};
