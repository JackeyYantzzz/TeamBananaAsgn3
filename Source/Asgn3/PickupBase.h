#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupBase.generated.h"

UENUM(BlueprintType)
enum class EPickupType : uint8
{
    Health      UMETA(DisplayName = "Health"),
    DoubleShot  UMETA(DisplayName = "Double Shot")
};

UCLASS()
class ASGN3_API APickupBase : public AActor
{
    GENERATED_BODY()

public:
    APickupBase();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPaperSpriteComponent* PickupSprite;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    EPickupType PickupType = EPickupType::Health;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    int32 HealAmount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    float DoubleShotDuration = 5.0f;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);
};
