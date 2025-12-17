#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Engine/EngineTypes.h" // 引擎类型定义
#include "Engine/DamageEvents.h" // 伤害事件类型定义
#include "ParagonFengMao.h" // 敌人角色类
#include "WukongCharacter.generated.h"

/*
===============================================================================
     悟空角色系统 - 头文件声明
    这个类实现了悟空的所有战斗和移动功能，包括连击、闪避、蓄力攻击等
===============================================================================
*/

//  动作状态枚举 - 角色的"心情"，告诉游戏"我现在在做什么"
UENUM(BlueprintType)
enum class EWukongActionState : uint8
{
    Idle UMETA(DisplayName = "Idle"),           //  站着不动，等待玩家输入
    Sprint UMETA(DisplayName = "Sprint"),       //  冲刺跑，消耗体力但速度快
    Dodge UMETA(DisplayName = "Dodge"),         //  闪避，短暂无敌并快速移动
    LightAttack UMETA(DisplayName = "LightAttack"), //  轻攻击连击中
    HeavyCharge UMETA(DisplayName = "HeavyCharge"), //  重攻击蓄力中（暂时保留但不再使用）
    HeavyAttack UMETA(DisplayName = "HeavyAttack"), //  重攻击释放
    Stun UMETA(DisplayName = "Stun"),           //  定身状态
    DrinkingPotion UMETA(DisplayName = "DrinkingPotion"), //  喝药状态
};

//  悟空角色类 - 继承Unreal Engine的基础角色类，添加战斗功能
UCLASS()
class WUKONGPROJECT_API AWukongCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    // ==========================================
    //  PUBLIC 区域 - 外部可访问的函数和变量
    // ==========================================

    //  构造函数 - 角色出生时调用，创建所有组件和设置默认值
    AWukongCharacter();

    /*  当前动作状态 - 角色的实时状态（空闲、冲刺、闪避等）
       动画蓝图和AI逻辑都用这个判断角色在做什么 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    EWukongActionState CurrentActionState;

    //  每帧调用 - 处理体力恢复、蓄力计时、动画同步等持续性逻辑
    virtual void Tick(float DeltaTime) override;

    //  输入绑定 - 把玩家的按键操作连接到角色函数
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    //  控制器相关重载函数
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_Controller() override;

protected:
    // ==========================================
    //  PROTECTED 区域 - 仅类内部和子类可访问
    // ==========================================

    //  游戏开始时调用 - 初始化角色状态，设置输入系统
    virtual void BeginPlay() override;

    // ==========================================
    //  战斗系统核心 - 连击和动画播放
    // ==========================================
    /*  连击计数器 - 动画蓝图用这个变量决定播放哪段攻击动画
       比如：AttackCount=1播放"Attack1"段落，AttackCount=2播放"Attack2"段落 */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 AtttackMyCount;

    /* 🏹 攻击状态标志 - 动画蓝图用这个判断角色是否在攻击中
       true=正在攻击，false=空闲状态 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bMyIsAttacking;

    /*  轻攻击连击动画 - 包含4段连击动画的主蒙太奇文件
       必须在构造函数或编辑器中加载，否则无法播放攻击动画 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    UAnimMontage* PrimaryMeleeMontage;

    /*  连击窗口计时器 - 控制连击的缓冲时间
       比如攻击动画结束后0.5秒内按攻击键还能连击 */
    FTimerHandle ComboWindowTimerHandle;

    /*  执行攻击逻辑 - 替代蓝图中的Switch逻辑
       @param NewAttackCount - 新的连击数（1-4）
       @param SectionName - 要播放的动画段落名（如"Attack1"） */
    void ExecuteAttackLogic(int32 NewAttackCount, FName SectionName);

    /*  重置攻击状态 - 连击窗口结束后清理状态
       把AtttackMyCount和bMyIsAttacking重置为0和false */
    void ResetAttackState();

    /*  播放轻攻击动画 - 根据连击索引播放对应的段落
       @param ComboIndex - 连击索引（0=第一段，1=第二段，2=第三段，3=第四段） */
    void PlayLightAttackMontage(int32 ComboIndex);

    /*  获取段落名称 - 把连击索引转换为动画段落名
       @param ComboIndex - 连击索引
       @return FName - 对应的段落名（如"Attack1"） */
    FName GetSectionNameForComboIndex(int32 ComboIndex);

    /*  播放重攻击动画 - 执行蓄力后的重攻击 */
    void PlayHeavyAttackMontage();

    /*  安全播放蒙太奇 - 检查各种条件后再播放动画
       @param Montage - 要播放的动画
       @param InPlayRate - 播放速度（1.0=正常速度）
       @param StartSection - 开始播放的段落
       @return bool - 是否播放成功 */
    bool PlayMontageSafe(UAnimMontage* Montage, float InPlayRate = 1.0f, FName StartSection = NAME_None);

    // ==========================================
    //  输入系统和动作控制 - 处理玩家输入
    // ==========================================
    //  输入动作定义 - 玩家按键对应的动作对象
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveAction;        // 🏃 移世 移动（WASD）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* LookAction;        // 👀 视角控制（鼠标移动）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* JumpAction;        // 🦘 跳跃（空格键）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* LightAttackAction; // ⚔️ 轻攻击（鼠标左键）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* SprintAction;      // 🏃‍♂️ 冲刺（左Shift）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* DodgeAction;       // 💨 闪避（F键）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* HeavyAttackAction; // 💥 重攻击（鼠标右键）

    // 新增的输入动作
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* StunSkillAction;   // 💫 定身技能（Q键）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* DrinkPotionAction; // 🧪 喝药（E键）

    // 测试功能按键
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* TestDamageAction;   // 🩹 测试受伤（T键）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* TestDeathAction;    // 💀 测试死亡（Y键）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* TestRespawnAction;  // 🔄 测试重生（U键）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* TestDetectAction;   // 👁️ 测试前方检测（G键）

    /*  输入映射上下文 - 按键和动作的连接器
       定义"哪个按键触发哪个InputAction" */
    UPROPERTY()
    class UInputMappingContext* InputMappingContext;

    //  动作执行函数 - 玩家输入触发后实际执行的函数
    void Move(const FInputActionValue& Value);        // 处理移动输入
    void Look(const FInputActionValue& Value);        // 处理视角输入
    void SprintStart();     // 开始冲刺（按下Shift）
    void SprintStop();      // 结束冲刺（释放Shift）
    void Dodge();           // 执行闪避
    void StartHeavyCharge();    // 开始重攻击蓄力
    void ReleaseHeavyAttack();  // 释放蓄力重攻击
    void CancelHeavyCharge();   // 取消重攻击蓄力
    void ExecuteHeavyAttack(float DamageMultiplier); // 执行重攻击（蓄力后的实际攻击逻辑）
    void HeavyAttack();         // 执行重攻击（蓄力后的实际攻击逻辑）

    // 新增的功能函数
    void StunSkill();       // 执行定身技能
    void DrinkPotion();     // 执行喝药

    // 定身技能相关函数
    void ApplyStunToTarget(AActor* Target); // 对目标施加定身效果
    void WakeUpEnemy(class AParagonFengMao* Enemy); // 加快敌人苏醒

    // 喝药相关函数
    void StartDrinkingPotion(); // 开始喝药
    void FinishDrinkingPotion(); // 完成喝药
    void ApplyPotionEffect(); // 应用药水效果

    // 战斗系统函数 - 伤害、死亡、重生
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override; // 受伤函数
    void Die();                 // 死亡处理
    void Respawn();             // 重生处理
    void ApplyDamageToTarget(float Damage, AActor* Target); // 对目标造成伤害
    void PerformHeavyAttackDamageDetection(float Damage, FVector AttackDirection); // 执行重攻击伤害检测
    void PerformLightAttackDamageDetection(); // 执行轻攻击伤害检测

    // 测试功能函数
    void TestTakeDamage();    // 🩹 测试自己受伤（扣血）
    void TestDie();          // 💀 测试死亡
    void TestRespawn();      // 🔄 测试重生
    void TestFrontDetection(); // 👁️ 测试前方是否有目标

    //  连击系统函数 - 管理轻攻击的连击逻辑
    void ClearComboQueue();    // 清理连击队列（被打断时调用）
    void OnLightAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 攻击动画结束回调
    void LightAttack();        // 执行轻攻击（主入口函数）

    //  动画防护系统 - 防止动画蓝图出bug
    void PreventAnimationBlueprintDivisionByZero(); // 防止除零错误（目前为空函数）
    void FixAnimationBlueprintVariables();          // 同步变量到动画蓝图

    //  体力系统属性 - 控制角色的"蓝条"
    float CurrentStamina;           // 当前体力值
    float MaxStamina;               // 体力上限
    float StaminaRecoveryRate;      // 体力恢复速度（每秒恢复多少）
    float LightAttackStaminaCost;   // 轻攻击消耗的体力
    float DodgeStaminaCost;         // 闪避消耗的体力
    float HeavyAttackStaminaCost;   // 重攻击消耗的体力
    float HeavyAttackDistance;      // 重攻击突进距离
    float HeavyAttackDuration;      // 重攻击突进持续时间

    // 生命值系统属性 - 控制角色的"红条"
    float CurrentHealth;            // 当前生命值
    float MaxHealth;                // 生命值上限
    float HealthRecoveryRate;       // 生命值恢复速度（每秒恢复多少）
    bool bIsDead;                   // 是否死亡

    // 伤害系统属性
    float LightAttackDamage;        // 轻攻击伤害
    float HeavyAttackBaseDamage;    // 重攻击基础伤害

    // 蓄力系统属性
    float CurrentChargeTime;        // 当前蓄力时间
    float MaxChargeTime;            // 最大蓄力时间
    float MinChargeDamageMultiplier; // 最小蓄力伤害倍率
    float MaxChargeDamageMultiplier; // 最大蓄力伤害倍率
    bool bIsCharging;               // 是否正在蓄力
    float LightComboBufferWindow;   // 轻攻击连击缓冲窗口
    int32 CurrentLightComboIndex;   // 当前连击索引（0-3）
    bool bLightAttackQueued;        // 是否有排队的轻攻击
    bool bIsInvincible;             // 是否处于无敌状态

    /*  闪避计时器 - 控制闪避的持续时间和结束 */
    FTimerHandle DodgeTimerHandle;

    /*  重攻击动画 - 立即响应的重攻击动画 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    UAnimMontage* HeavyAttackMontage;

    // 开局 / 重生播放的动作蒙太奇
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")

    UAnimMontage* StartMontage = nullptr;

    /*  轻攻击动画 - 每个攻击使用独立的动画文件 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    UAnimMontage* Attack1Montage;  // 第一段攻击动画

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    UAnimMontage* Attack2Montage;  // 第二段攻击动画

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    UAnimMontage* Attack3Montage;  // 第三段攻击动画

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    UAnimMontage* Attack4Montage;  // 第四段攻击动画

    //  闪避系统属性
    UPROPERTY(EditDefaultsOnly, Category = "Dodge")
    UAnimMontage* DodgeMontage;     // 闪避动画（可选，没有就用程序化移动）

    UPROPERTY(EditDefaultsOnly, Category = "Dodge")
    float DodgeDistance;            // 闪避移动距离

    UPROPERTY(EditDefaultsOnly, Category = "Dodge")
    float DodgeSpeed;               // 闪避移动速度

    UPROPERTY(EditDefaultsOnly, Category = "Dodge")
    float DodgeDuration;            // 闪避持续时间（无敌时间）

    UPROPERTY(EditDefaultsOnly, Category = "Dodge")
    float DodgeCooldown;            // 闪避冷却时间

    FTimerHandle DodgeCooldownTimerHandle; // 冷却计时器
    bool bCanDodge;                 // 是否可以闪避（冷却检查用）

    /* 最后移动输入 - 记录玩家最后按的移动方向
       用于在不按移动键时仍能往面对方向闪避 */
    FVector2D LastMovementInput;

    /*  轻攻击连击数组 - 备用方案，如果需要分别的动画文件
       目前主要用PrimaryMeleeMontage的段落系统 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TArray<UAnimMontage*> LightComboMontages;

    // 死亡动画
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* DeathMontage;     // 死亡动画（可选，没有就用物理效果）

    // 新增的定身技能属性
    UPROPERTY(EditDefaultsOnly, Category = "StunSkill")
    float StunSkillDamage;          // 定身技能伤害
    UPROPERTY(EditDefaultsOnly, Category = "StunSkill")
    float StunDuration;             // 定身持续时间
    UPROPERTY(EditDefaultsOnly, Category = "StunSkill")
    float StunSkillStaminaCost;     // 定身技能消耗体力
    UPROPERTY(EditDefaultsOnly, Category = "StunSkill")
    float StunSkillRange;           // 定身技能范围
    UPROPERTY(EditDefaultsOnly, Category = "StunSkill")
    UAnimMontage* StunSkillMontage; // 定身技能动画

    // 新增的喝药属性
    UPROPERTY(EditDefaultsOnly, Category = "Potion")
    int32 PotionCount;              // 药水数量
    UPROPERTY(EditDefaultsOnly, Category = "Potion")
    float InstantHealAmount;        // 瞬间回复血量
    UPROPERTY(EditDefaultsOnly, Category = "Potion")
    float OverTimeHealAmount;       // 持续回复血量
    UPROPERTY(EditDefaultsOnly, Category = "Potion")
    float OverTimeHealDuration;     // 持续回复时间
    UPROPERTY(EditDefaultsOnly, Category = "Potion")
    float OverTimeHealInterval;     // 持续回复间隔
    UPROPERTY(EditDefaultsOnly, Category = "Potion")
    UAnimMontage* DrinkPotionMontage; // 喝药动画
    FTimerHandle PotionHealTimerHandle; // 药水回复计时器
};