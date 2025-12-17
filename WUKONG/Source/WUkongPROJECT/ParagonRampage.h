// ParagonRampage.h
#pragma once

#include "CoreMinimal.h"
#include "ParagonFengMao.h"
#include "ParagonRampage.generated.h"

/**
 * Rampage敌人类，继承自ParagonFengMao以复用所有AI逻辑
 */
UCLASS()
class WUKONGPROJECT_API AParagonRampage : public AParagonFengMao
{
    GENERATED_BODY()

public:
    // 构造函数
    AParagonRampage();

protected:
    // 可以重写父类函数来自定义特定行为
    virtual void BeginPlay() override;
};