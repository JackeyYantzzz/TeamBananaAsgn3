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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPaperSpriteComponent* BulletSpriteComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float BulletSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float LifeSpanTimer = 3.0f;
};