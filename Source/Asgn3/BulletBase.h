#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletBase.generated.h"

UCLASS()
class ASGN3_API ABulletBase : public AActor
{
    GENERATED_BODY()

public:
    ABulletBase();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // --- Components ---
    // Collision box for bullet hit detection
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* CollisionBox;

    // Sprite component for 2D bullet appearance
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPaperSpriteComponent* BulletSpriteComponent;

    // --- Movement & Combat Properties ---
    // Speed of the bullet
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float BulletSpeed = 600.0f;

    // Lifetime in seconds before auto destruction
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float LifeSpanTimer = 3.0f;
};