// ParagonNarbash.h
#pragma once

#include "CoreMinimal.h"
#include "ParagonFengMao.h"
#include "ParagonNarbash.generated.h"

UCLASS()
class BLACK_MYTH_CPP_API AParagonNarbash : public AParagonFengMao
{
    GENERATED_BODY()

public:
    // ¹¹Ôìº¯Êý
    AParagonNarbash();

protected:
    virtual void BeginPlay() override;
};