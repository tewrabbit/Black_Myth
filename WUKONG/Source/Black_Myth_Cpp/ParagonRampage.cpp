// ParagonRampage.cpp
#include "ParagonRampage.h"
#include "UObject/ConstructorHelpers.h"

AParagonRampage::AParagonRampage()
{

    // 中等血量
    MaxHealth = 225.f;  // ParagonFengMao为150.f, ParagonNarbash为300.f

    // 中等攻击力
    AttackDamage = 20.f; // ParagonFengMao为25.f, ParagonNarbash为15.f
    HeavyAttackDamage = 40.f; // ParagonFengMao为50.f, ParagonNarbash为30.f

    // 中等移动速度
    PatrolSpeed = 250.f;  // ParagonFengMao为300.f, ParagonNarbash为200.f
    ChaseSpeed = 500.f;   // ParagonFengMao为600.f, ParagonNarbash为400.f



    // 加载Rampage特有的动画资源
    static ConstructorHelpers::FObjectFinder<UAnimMontage> RampageAttackMontageObj(
        TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Animations/Attack_Biped_Melee_A_Montage")
    );
    if (RampageAttackMontageObj.Succeeded())
    {
        AttackMontage = RampageAttackMontageObj.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> RampageHeavyAttackMontageObj(
        TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Animations/Attack_Biped_Melee_B_Montage")
    );
    if (RampageHeavyAttackMontageObj.Succeeded())
    {
        HeavyAttackMontage = RampageHeavyAttackMontageObj.Object;
    }


}

void AParagonRampage::BeginPlay()
{
    // 调用父类的BeginPlay
    Super::BeginPlay();

    // 添加Rampage特有的初始化逻辑
    UE_LOG(LogTemp, Warning, TEXT("🐺 Rampage Enemy BeginPlay"));
}