// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/EngineTypes.h"
#include "Engine/DamageEvents.h"
#include "AIController.h"
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

	// 感知和检测
	void CheckForPlayer();
	bool CanSeePlayer();
	FVector GetPlayerLocation();

	// 战斗相关
	void PerformAttack();
	void ResetAttack();

	// 工具函数
	void GenerateDefaultPatrolPoints();
	void ValidateAIState();

public:
	// AI属性
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectionRange = 800.f;              // 检测范围

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange = 150.f;                 // 攻击范围

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float PatrolSpeed = 200.f;                 // 巡逻速度

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ChaseSpeed = 400.f;                  // 追逐速度

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackCooldown = 2.f;                // 攻击冷却时间

	// 生命值系统
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 150.f;                   // 最大生命值

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;                       // 当前生命值

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	bool bIsDead;                              // 是否死亡

	// 攻击伤害
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackDamage = 25.f;                 // 攻击伤害

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

	// AI控制器引用
	class AFengMaoAIController* AIController;  // AI控制器

	// Timer handles
	FTimerHandle DeathTimerHandle;            // 死亡定时器

public:
	// 允许AI控制器访问的目标玩家
	AActor* TargetPlayer;                      // 目标玩家
};
