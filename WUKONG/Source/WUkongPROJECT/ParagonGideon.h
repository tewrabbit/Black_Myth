// ParagonGideon.h
#pragma once

#include "CoreMinimal.h"
#include "ParagonFengMao.h"
// 包含小怪头文件
#include "ParagonNarbash.h"
#include "ParagonRampage.h"
#include "ParagonGideon.generated.h"

// Boss阶段枚举
UENUM(BlueprintType)
enum class EBossPhase : uint8
{
    Phase1 UMETA(DisplayName = "第一阶段"),
    Phase2 UMETA(DisplayName = "第二阶段"),
    Phase3 UMETA(DisplayName = "第三阶段")
};

/**
 * Gideon Boss敌人类，继承自ParagonFengMao以复用所有AI逻辑
 * 特点：三阶段战斗、特殊技能、高血量
 */
UCLASS()
class WUKONGPROJECT_API AParagonGideon : public AParagonFengMao
{
    GENERATED_BODY()

public:
    // 构造函数
    AParagonGideon();

protected:
    // 生命周期函数
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    
    // 重写关键函数实现Boss特有行为
    virtual void AttackPlayer();
    virtual void Die();
    
    // Boss特有函数
    void CheckPhaseTransition();
    void TransitionToPhase(EBossPhase NewPhase);
    void UseSpecialAbility();
    void PerformAreaAttack();
    void SummonMinions();
    void ActivateShield();
    void DeactivateShield();
    void EnterEnrageMode();
    
private:
    // 阶段系统
    UPROPERTY(VisibleAnywhere, Category = "Boss")
    EBossPhase CurrentPhase;
    
    UPROPERTY(EditAnywhere, Category = "Boss Stats")
    float Phase2Threshold = 0.7f; // 70%血量时进入二阶段
    
    UPROPERTY(EditAnywhere, Category = "Boss Stats")
    float Phase3Threshold = 0.3f; // 30%血量时进入三阶段
    
    // 特殊机制相关
    UPROPERTY(VisibleAnywhere, Category = "Boss Abilities")
    bool bHasShield;
    
    UPROPERTY(VisibleAnywhere, Category = "Boss Abilities")
    bool bIsEnraged;
    
    UPROPERTY(EditAnywhere, Category = "Boss Abilities")
    float SpecialAbilityCooldown = 15.f; // 特殊技能冷却时间
    
    UPROPERTY(EditAnywhere, Category = "Boss Abilities")
    float ShieldDuration = 10.f; // 护盾持续时间
    
    UPROPERTY(EditAnywhere, Category = "Boss Abilities")
    int32 MinionsToSummon = 3; // 每次召唤的小怪数量
    
    FTimerHandle SpecialAbilityTimerHandle;
    FTimerHandle ShieldTimerHandle;
    FTimerHandle EnrageTimerHandle;
};