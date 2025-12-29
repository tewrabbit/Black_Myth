// ParagonNarbash.cpp
#include "ParagonNarbash.h"
#include "UObject/ConstructorHelpers.h"

AParagonNarbash::AParagonNarbash()
{
    // 高血量
    MaxHealth = 300.f; 

    // 低攻击力
    AttackDamage = 10.f;
    HeavyAttackDamage = 30.f; 

    // 低移动速度
    PatrolSpeed = 200.f;  
    ChaseSpeed = 400.f; 

    static ConstructorHelpers::FObjectFinder<UAnimMontage> NarbashAttackMontageObj(
        TEXT("/Game/ParagonNarbash/Characters/Heroes/Narbash/Animations/Primary_Swing1_Medium_Montage") // 替换为实际路径
    );
    if (NarbashAttackMontageObj.Succeeded())
    {
        AttackMontage = NarbashAttackMontageObj.Object;
    }


}

void AParagonNarbash::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("🐺 Narbash Enemy BeginPlay"));
}