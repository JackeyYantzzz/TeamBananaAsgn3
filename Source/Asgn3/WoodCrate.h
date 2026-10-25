#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WoodCrate.generated.h"

UCLASS()
class ASGN3_API AWoodCrate : public AActor
{
    GENERATED_BODY()

public:
    AWoodCrate();

    // --- Components ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPaperSpriteComponent* CrateSpriteComponent;

    // --- Combat Properties ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 Health = 1;

    // Handle overlap event when hit by bullets (while blocking tanks)
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);
};