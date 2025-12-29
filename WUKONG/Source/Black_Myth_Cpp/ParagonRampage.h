// ParagonRampage.h
#pragma once

#include "CoreMinimal.h"
#include "ParagonFengMao.h"
#include "ParagonRampage.generated.h"

UCLASS()
class BLACK_MYTH_CPP_API AParagonRampage : public AParagonFengMao
{
    GENERATED_BODY()

public:
    AParagonRampage();

protected:
    virtual void BeginPlay() override;
};
