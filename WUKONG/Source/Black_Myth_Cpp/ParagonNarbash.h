// ParagonNarbash.h
#pragma once

#include "CoreMinimal.h"
#include "ParagonFengMao.h"
#include "ParagonNarbash.generated.h"

/**
 * Narbash敌人类，继承自ParagonFengMao以复用所有AI逻辑
 */
UCLASS()
class BLACK_MYTH_CPP_API AParagonNarbash : public AParagonFengMao
{
    GENERATED_BODY()

public:
    // 构造函数
    AParagonNarbash();

protected:
    // 可以重写父类函数来自定义特定行为
    virtual void BeginPlay() override;
};