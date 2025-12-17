// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/EngineTypes.h"
#include "Engine/DamageEvents.h"
#include "FengMaoAIController.h"  // 修改这里
#include "Components/WidgetComponent.h"
#include "ParagonFengMao.generated.h"

// AI状态枚举
UENUM(BlueprintType)
enum class EFengMaoAIState : uint8
{
	Idle UMETA(DisplayName = "Idle"),           // 待机
	Patrol UMETA(DisplayName = "Patrol"),       // 巡逻
	Chase UMETA(DisplayName = "Chase"),         // 追逐玩家
	Attack UMETA(DisplayName = "Attack"),       // 攻击
	HeavyAttack UMETA(DisplayName = "HeavyAttack"), // 重击
	Dodge UMETA(DisplayName = "Dodge"),         // 闪避
	Dead UMETA(DisplayName = "Dead")            // 死亡
};

UCLASS()
class WUKONGPROJECT_API AParagonFengMao : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AParagonFengMao();

	// 战斗系统
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// AI状态
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetAIState(EFengMaoAIState NewState);

	UFUNCTION(BlueprintPure, Category = "AI")
	EFengMaoAIState GetAIState() const { return CurrentAIState; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called after components are initialized
	virtual void PostInitializeComponents() override;

	// 控制台命令处理
	UFUNCTION(exec)
	void ApplyTestDamage();

	UFUNCTION(exec)
	void StartAutoDamageTest();

	UFUNCTION(exec)
	void StopAutoDamageTest();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// AI行为函数
	void Patrol();
	void ChasePlayer();
	void AttackPlayer();
	void Die();
	void TryUseSkill();
	void DashToTargetAndStrike();
	void FinishDash();
	void PlayAttackAnimation();
	void PerformHeavyAttack(); // 新增重击函数

	// 闪避函数
	void TryDodge();
	void PerformDodge();
	void FinishDodge();

	// 测试函数
	void TestTakeDamage();

	// 感知和检测
	void CheckForPlayer();
	bool CanSeePlayer();
	FVector GetPlayerLocation();

	// 战斗相关
	void PerformAttack();
	void ResetAttack();
	bool PlayAttackMontage(); // 播放攻击动画
	bool PlaySkillMontage();  // 播放技能动画
	bool PlayHeavyAttackMontage(); // 播放重击动画

	// 工具函数
	void GenerateDefaultPatrolPoints();
	void ValidateAIState();
	void FaceTarget(AActor* Target, float DeltaTime);
	FVector GetRandomPatrolLocation();

public:
	// AI属性
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectionRange = 800.f;              // 检测范围

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange = 150.f;                 // 攻击范围

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float PatrolSpeed = 300.f;                 // 巡逻速度（提高3倍）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ChaseSpeed = 400.f;                  // 追逐速度（提高3倍）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackCooldown = 1.5f;                // 攻击冷却时间

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float HeavyAttackChance = 0.3f;             // 重击概率

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AcceptanceRadiusPatrol = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AcceptanceRadiusChase = 75.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SkillCooldown = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SkillRange = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SkillDamage = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DashSpeedMultiplier = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DashDuration = 0.35f;

	// 闪避属性
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	float DodgeChance = 0.3f;                  // 闪避概率

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	float DodgeDistance = 300.f;               // 闪避距离

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	float DodgeCooldown = 2.0f;                // 闪避冷却时间

	// 生命值系统
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 150.f;                   // 最大生命值

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;                       // 当前生命值

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	bool bIsDead;                              // 是否死亡

	// 攻击伤害
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackDamage = 25.f;                 // 普通攻击伤害

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float HeavyAttackDamage = 50.f;            // 重击伤害

	// 动画资源
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* AttackMontage;              // 普通攻击动画

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* HeavyAttackMontage;         // 重击动画

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* SkillMontage;               // 技能攻击动画

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* DeathMontage;               // 死亡动画

	// AI状态
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	EFengMaoAIState CurrentAIState;

	// 巡逻点
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TArray<AActor*> PatrolPoints;              // 巡逻路径点

	// 血条Widget组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	class UWidgetComponent* HealthBarWidget;

private:
	// 内部变量
	int32 CurrentPatrolIndex;                  // 当前巡逻点索引
	float LastAttackTime;                      // 上次攻击时间
	float LastSkillTime;
	float LastDodgeTime;                       // 上次闪避时间
	FVector CurrentPatrolDestination;          // 当前巡逻目标位置
	bool bIsAttacking;                         // 是否正在攻击
	bool bIsUsingSkill;                        // 是否正在使用技能
	bool bIsDodging;                           // 是否正在闪避
	bool bIsAutoTestingDamage;                 // 是否自动测试伤害

	// AI控制器引用
	class AFengMaoAIController* AIController;  // AI控制器

	// Timer handles
	FTimerHandle DeathTimerHandle;            // 死亡定时器
	FTimerHandle SkillTimerHandle;
	FTimerHandle DodgeTimerHandle;            // 闪避定时器
	FTimerHandle AutoDamageTestTimerHandle;   // 自动测试伤害定时器

public:
	// 允许AI控制器访问的目标玩家
	AActor* TargetPlayer;                      // 目标玩家
};