#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MychuansongActor.generated.h"

UCLASS()
class BLACK_MYTH_CPP_API AMychuansongActor : public AActor
{
    GENERATED_BODY()

public:
    AMychuansongActor();

protected:
    virtual void BeginPlay() override;

    // 传送逻辑
    void TeleportToNextLevel();

public:
    // 可视化方块
    UPROPERTY(VisibleAnywhere)
    class UStaticMeshComponent* Mesh;
};
