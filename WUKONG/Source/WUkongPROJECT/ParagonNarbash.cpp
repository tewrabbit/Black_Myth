// ParagonNarbash.cpp
#include "ParagonNarbash.h"
#include "UObject/ConstructorHelpers.h"

AParagonNarbash::AParagonNarbash()
{
    // 高血量（比父类高）
    MaxHealth = 300.f;  // 父类为150.f
    
    // 低攻击力（比父类低）
    AttackDamage = 10.f; // 父类为25.f
    HeavyAttackDamage = 30.f; // 父类为50.f
    
    // 低移动速度（比父类低）
    PatrolSpeed = 200.f;  // 父类为300.f
    ChaseSpeed = 400.f;   // 父类为600.f
  
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
    // 调用父类的BeginPlay
    Super::BeginPlay();
    
    // 添加Narbash特有的初始化逻辑
    UE_LOG(LogTemp, Warning, TEXT("🐺 Narbash Enemy BeginPlay"));
}