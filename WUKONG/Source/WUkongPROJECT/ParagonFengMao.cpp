// Fill out your copyright notice in the Description page of Project Settings.

#include "ParagonFengMao.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "FengMaoAIController.h"

// Sets default values
AParagonFengMao::AParagonFengMao()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 初始化属性
	CurrentHealth = MaxHealth;
	bIsDead = false;
	CurrentAIState = EFengMaoAIState::Idle;
	CurrentPatrolIndex = 0;
	LastAttackTime = 0.f;
	TargetPlayer = nullptr;
	AIController = nullptr;

	UE_LOG(LogTemp, Warning, TEXT("🐺 ==========================="));
	UE_LOG(LogTemp, Warning, TEXT("🐺 🏗️ ParagonFengMao构造函数执行中..."));
	UE_LOG(LogTemp, Warning, TEXT("🐺 AI Controller类: %s"), AIControllerClass ? *AIControllerClass->GetName() : TEXT("未设置"));
	UE_LOG(LogTemp, Warning, TEXT("🐺 ==========================="));

	// 设置角色移动
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.f, 600.f, 0.f);
		GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
		UE_LOG(LogTemp, Warning, TEXT("🐺 CharacterMovement设置完成"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("🐺 错误: CharacterMovement组件不存在！"));
	}

	// 设置碰撞 - 确保可以被攻击
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 忽略其他Pawn的碰撞
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // 忽略相机
		GetCapsuleComponent()->SetGenerateOverlapEvents(true); // 生成重叠事件
		UE_LOG(LogTemp, Warning, TEXT("🐺 CapsuleComponent碰撞设置完成"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("🐺 错误: CapsuleComponent不存在！"));
	}

	// 设置AI Controller类
	AIControllerClass = AFengMaoAIController::StaticClass();
	UE_LOG(LogTemp, Warning, TEXT("🐺 AI Controller类已设置为: %s"), *AIControllerClass->GetName());
	UE_LOG(LogTemp, Warning, TEXT("🐺 AI Controller类设置完成"));

	// 创建血条Widget组件
	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	if (HealthBarWidget)
	{
		HealthBarWidget->SetupAttachment(GetRootComponent());
		HealthBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f)); // 在头顶上方
		HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
		HealthBarWidget->SetDrawSize(FVector2D(150.f, 30.f));
		HealthBarWidget->SetVisibility(false); // 默认隐藏
		UE_LOG(LogTemp, Warning, TEXT("🐺 HealthBarWidget创建完成"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("🐺 错误: HealthBarWidget创建失败！"));
	}
}

// Called after components are initialized
void AParagonFengMao::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	UE_LOG(LogTemp, Warning, TEXT("🐺 PostInitializeComponents: 组件初始化完成"));
	UE_LOG(LogTemp, Warning, TEXT("🐺 AI Controller类: %s"), AIControllerClass ? *AIControllerClass->GetName() : TEXT("未设置"));
	
	// 确保AI Controller类已设置
	if (!AIControllerClass)
	{
		AIControllerClass = AFengMaoAIController::StaticClass();
		UE_LOG(LogTemp, Warning, TEXT("🐺 自动设置AI Controller类"));
	}
}

// Called when the game starts or when spawned
void AParagonFengMao::BeginPlay()
{
	Super::BeginPlay();

	// 获取AI Controller
	AIController = Cast<AFengMaoAIController>(GetController());

	UE_LOG(LogTemp, Warning, TEXT("🐺 ==========================="));
	UE_LOG(LogTemp, Warning, TEXT("🐺 🐺🦇 封魔敌人出生！🦇🐺"));
	UE_LOG(LogTemp, Warning, TEXT("🐺 ID: %s"), *GetName());
	UE_LOG(LogTemp, Warning, TEXT("🐺 位置: %s"), *GetActorLocation().ToString());
	UE_LOG(LogTemp, Warning, TEXT("🐺 生命值: %.1f/%.1f"), CurrentHealth, MaxHealth);
	UE_LOG(LogTemp, Warning, TEXT("🐺 巡逻点数量: %d"), PatrolPoints.Num());
	UE_LOG(LogTemp, Warning, TEXT("🐺 当前Controller: %s"), GetController() ? *GetController()->GetClass()->GetName() : TEXT("无"));
	UE_LOG(LogTemp, Warning, TEXT("🐺 AI控制器: %s"), AIController ? TEXT("✅ 已连接") : TEXT("❌ 未找到"));

	// 如果没有AI Controller，尝试手动生成
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("🐺 错误: AI Controller未找到！尝试手动生成..."));
		
		// 检查AIControllerClass是否设置
		if (AIControllerClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("🐺 AI Controller类已设置: %s"), *AIControllerClass->GetName());
			
			// 尝试手动生成AI Controller
			if (GetWorld())
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				AFengMaoAIController* NewAIController = GetWorld()->SpawnActor<AFengMaoAIController>(AIControllerClass, SpawnParams);
				if (NewAIController)
				{
					NewAIController->Possess(this);
					AIController = NewAIController;
					UE_LOG(LogTemp, Warning, TEXT("🐺 ✅ 手动生成AI Controller成功"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("🐺 ❌ 手动生成AI Controller失败"));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("🐺 ❌ AI Controller类未设置！"));
		}
	}

	// 验证移动组件
	if (!GetCharacterMovement())
	{
		UE_LOG(LogTemp, Error, TEXT("🐺 ❌ 错误: CharacterMovement组件不存在！"));
	}
	else
	{
		// 确保移动组件没有被禁用
		if (!GetCharacterMovement()->IsActive())
		{
			UE_LOG(LogTemp, Warning, TEXT("🐺 ⚠️ 移动组件未激活，尝试激活..."));
			GetCharacterMovement()->SetActive(true);
		}
		
		// 确保可以移动
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		UE_LOG(LogTemp, Warning, TEXT("🐺 移动组件状态: 激活=%d, 模式=%d, 速度=%.1f"),
			GetCharacterMovement()->IsActive(),
			(int32)GetCharacterMovement()->MovementMode,
			GetCharacterMovement()->MaxWalkSpeed);
	}

	UE_LOG(LogTemp, Warning, TEXT("🐺 ==========================="));

	// 验证关键设置
	if (!GetCharacterMovement())
	{
		UE_LOG(LogTemp, Error, TEXT("🐺 错误: CharacterMovement组件不存在！"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 CharacterMovement组件正常"));
	}

	if (!GetCapsuleComponent())
	{
		UE_LOG(LogTemp, Error, TEXT("🐺 错误: CapsuleComponent不存在！"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 CapsuleComponent正常"));
	}

	// 如果没有设置巡逻点，自动生成一些
	if (PatrolPoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 未设置巡逻点，自动生成巡逻路径"));
		GenerateDefaultPatrolPoints();
	}

	// 初始化到巡逻状态
	SetAIState(EFengMaoAIState::Patrol);
}

// 自动生成默认巡逻点
void AParagonFengMao::GenerateDefaultPatrolPoints()
{
	FVector BaseLocation = GetActorLocation();

	// 在出生点周围生成4个巡逻点
	for (int32 i = 0; i < 4; i++)
	{
		float Angle = i * 90.f; // 每个点间隔90度
		float Radius = 500.f;   // 500单位半径

		FVector Offset = FVector(
			FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
			0.f
		);

		FVector PatrolPointLocation = BaseLocation + Offset;

		// 创建一个空的Actor作为巡逻点
		AActor* PatrolPoint = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), PatrolPointLocation, FRotator::ZeroRotator);
		if (PatrolPoint)
		{
			PatrolPoint->SetActorLabel(FString::Printf(TEXT("FengMao_PatrolPoint_%d"), i));
			PatrolPoints.Add(PatrolPoint);
			UE_LOG(LogTemp, Warning, TEXT("🐺 生成巡逻点 %d: %s"), i, *PatrolPointLocation.ToString());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("🐺 自动生成 %d 个巡逻点完成"), PatrolPoints.Num());
}

// Called every frame
void AParagonFengMao::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 安全检查
	if (!GetWorld() || !IsValid(this))
	{
		UE_LOG(LogTemp, Error, TEXT("🐺 Tick: 世界或对象无效"));
		return;
	}

	// 每5秒输出一次Tick确认信息
	static float LastTickLogTime = 0.f;
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastTickLogTime >= 5.f)
	{
		LastTickLogTime = CurrentTime;
		UE_LOG(LogTemp, Warning, TEXT("🐺 Tick确认: %s 正在运行，移动组件: %s, 速度: %.1f"),
			*GetName(),
			GetCharacterMovement() ? TEXT("存在") : TEXT("不存在"),
			GetCharacterMovement() ? GetCharacterMovement()->MaxWalkSpeed : 0.f);
	}

	if (bIsDead)
	{
		return; // 死亡后不执行AI逻辑
	}

	// 定期检查AI状态的合理性
	static float LastStateCheckTime = 0.f;
	// 重用上面的CurrentTime变量
	if (CurrentTime - LastStateCheckTime >= 5.f) // 每5秒检查一次
	{
		LastStateCheckTime = CurrentTime;
		ValidateAIState();
	}

	// 更新AI状态
	CheckForPlayer();

	switch (CurrentAIState)
	{
	case EFengMaoAIState::Patrol:
		Patrol();
		break;

	case EFengMaoAIState::Chase:
		ChasePlayer();
		break;

	case EFengMaoAIState::Attack:
		AttackPlayer();
		break;

	case EFengMaoAIState::Idle:
	default:
		// 待机状态
		break;
	}

	// 调试可视化 - 显示检测范围和状态
	if (TargetPlayer)
	{
		// 绘制检测范围
		DrawDebugCircle(GetWorld(), GetActorLocation(), DetectionRange, 32, FColor::Yellow, false, -1.f, 0, 2.f,
			FVector(1,0,0), FVector(0,1,0), false);

		// 绘制攻击范围
		DrawDebugCircle(GetWorld(), GetActorLocation(), AttackRange, 16, FColor::Red, false, -1.f, 0, 1.f,
			FVector(1,0,0), FVector(0,1,0), false);

		// 绘制到玩家的连线
		DrawDebugLine(GetWorld(), GetActorLocation(), TargetPlayer->GetActorLocation(), FColor::Cyan, false, -1.f, 0, 1.f);
	}

	// 调试显示状态 - 每秒显示一次
	static float LastDebugTime = 0.f;
	if (CurrentTime - LastDebugTime >= 1.f)
	{
		LastDebugTime = CurrentTime;

		FString StateText = TEXT("未知");
		switch (CurrentAIState)
		{
		case EFengMaoAIState::Idle: StateText = TEXT("待机"); break;
		case EFengMaoAIState::Patrol: StateText = TEXT("巡逻"); break;
		case EFengMaoAIState::Chase: StateText = TEXT("追逐"); break;
		case EFengMaoAIState::Attack: StateText = TEXT("攻击"); break;
		case EFengMaoAIState::Dead: StateText = TEXT("死亡"); break;
		}

		FString TargetInfo = TargetPlayer ? FString::Printf(TEXT("有目标(%s)"), *TargetPlayer->GetName()) : TEXT("无目标");
		FString ControllerInfo = AIController ? TEXT("✅已连接") : TEXT("❌未连接");

		UE_LOG(LogTemp, Warning, TEXT("🐺 封魔[%s] 状态:%s | 生命:%.1f/%.1f | 目标:%s | AI控制器:%s | 位置:%s"),
			*GetName(), *StateText, CurrentHealth, MaxHealth, *TargetInfo, *ControllerInfo,
			*GetActorLocation().ToString());
	}
}

// 验证AI状态的合理性
void AParagonFengMao::ValidateAIState()
{
	UE_LOG(LogTemp, Warning, TEXT("🐺 验证AI状态..."));

	// 检查目标是否仍然有效
	if (TargetPlayer && (!TargetPlayer->IsValidLowLevel() || TargetPlayer->IsPendingKillPending()))
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 目标无效，清空目标"));
		TargetPlayer = nullptr;
		SetAIState(EFengMaoAIState::Patrol);
	}

	// 检查状态转换的合理性
	if (CurrentAIState == EFengMaoAIState::Chase && !TargetPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 追逐状态但没有目标，回到巡逻"));
		SetAIState(EFengMaoAIState::Patrol);
	}

	if (CurrentAIState == EFengMaoAIState::Attack && (!TargetPlayer || FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation()) > AttackRange + 100.f))
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 攻击状态但目标过远，回到巡逻"));
		SetAIState(EFengMaoAIState::Patrol);
	}

	// 检查生命值合理性
	if (CurrentHealth < 0.f)
	{
		CurrentHealth = 0.f;
		UE_LOG(LogTemp, Warning, TEXT("🐺 修正生命值为0"));
	}

	if (CurrentHealth > MaxHealth)
	{
		CurrentHealth = MaxHealth;
		UE_LOG(LogTemp, Warning, TEXT("🐺 修正生命值为最大值"));
	}
}

// Called to bind functionality to input
void AParagonFengMao::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// 受伤函数
float AParagonFengMao::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("🐺 TakeDamage调用: 伤害=%.1f, 来源=%s"),
		DamageAmount, DamageCauser ? *DamageCauser->GetName() : TEXT("未知"));

	if (bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 已死亡，忽略伤害"));
		return 0.f;
	}

	float ActualDamage = FMath::Max(0.f, DamageAmount); // 确保伤害不为负数
	CurrentHealth -= ActualDamage;

	UE_LOG(LogTemp, Warning, TEXT("🐺 封魔受伤: %.1f, 剩余生命: %.1f/%.1f"), ActualDamage, CurrentHealth, MaxHealth);

	// 检查是否死亡
	if (CurrentHealth <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 生命值降至0，触发死亡"));
		Die();
	}
	else
	{
		// 受伤后进入追逐状态
		if (DamageCauser && DamageCauser->IsA(ACharacter::StaticClass()))
		{
			TargetPlayer = DamageCauser;
			UE_LOG(LogTemp, Warning, TEXT("🐺 锁定攻击者: %s，进入追逐状态"), *DamageCauser->GetName());
			SetAIState(EFengMaoAIState::Chase);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("🐺 伤害来源无效或不是角色"));
		}
	}

	return ActualDamage;
}

// 设置AI状态
void AParagonFengMao::SetAIState(EFengMaoAIState NewState)
{
	if (CurrentAIState == NewState || bIsDead)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("🐺 AI状态改变: %d → %d"), (int32)CurrentAIState, (int32)NewState);
	CurrentAIState = NewState;

	// 根据新状态调整移动速度
	if (GetCharacterMovement())
	{
		switch (NewState)
		{
		case EFengMaoAIState::Patrol:
			GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
			break;
		case EFengMaoAIState::Chase:
			GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
			break;
		case EFengMaoAIState::Attack:
			GetCharacterMovement()->MaxWalkSpeed = 0.f; // 攻击时停止移动
			break;
		default:
			GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
			break;
		}
	}
}

// 巡逻行为
void AParagonFengMao::Patrol()
{
	UE_LOG(LogTemp, Warning, TEXT("🐺 巡逻行为执行中..."));

	if (PatrolPoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 无巡逻点，执行随机游荡"));

		// 没有巡逻点，随机游荡
		if (FMath::FRand() < 0.01f) // 1%概率改变方向
		{
			FRotator NewRotation = GetActorRotation();
			NewRotation.Yaw += FMath::FRandRange(-90.f, 90.f);
			SetActorRotation(NewRotation);
			UE_LOG(LogTemp, Warning, TEXT("🐺 改变游荡方向: %.1f度"), NewRotation.Yaw);
		}
		AddMovementInput(GetActorForwardVector(), 1.f);
		return;
	}

	// 移动到当前巡逻点
	AActor* CurrentPatrolPoint = PatrolPoints[CurrentPatrolIndex];
	if (CurrentPatrolPoint)
	{
		FVector Direction = (CurrentPatrolPoint->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		float Distance = FVector::Distance(GetActorLocation(), CurrentPatrolPoint->GetActorLocation());

		UE_LOG(LogTemp, Warning, TEXT("🐺 移动到巡逻点 %d，距离: %.1f米"), CurrentPatrolIndex, Distance);

		AddMovementInput(Direction, 1.f);

		// 检查是否到达巡逻点
		if (Distance < 100.f)
		{
			// 到达巡逻点，移动到下一个
			CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
			UE_LOG(LogTemp, Warning, TEXT("🐺 到达巡逻点，切换到下一个: %d"), CurrentPatrolIndex);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("🐺 错误: 巡逻点 %d 为空！"), CurrentPatrolIndex);
		CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
	}
}

// 追逐玩家
void AParagonFengMao::ChasePlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("🐺 追逐玩家行为执行中..."));

	if (!TargetPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 目标丢失，返回巡逻"));
		SetAIState(EFengMaoAIState::Patrol);
		return;
	}

	float Distance = FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation());
	UE_LOG(LogTemp, Warning, TEXT("🐺 追逐距离: %.1f米，攻击范围: %.1f米"), Distance, AttackRange);

	// 检查距离，如果太远就返回巡逻
	if (Distance > DetectionRange * 1.2f)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 距离过远，返回巡逻"));
		SetAIState(EFengMaoAIState::Patrol);
		return;
	}

	// 如果足够近就攻击
	if (Distance <= AttackRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 进入攻击范围，开始攻击"));
		SetAIState(EFengMaoAIState::Attack);
		return;
	}

	// 移动向玩家
	FVector Direction = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	AddMovementInput(Direction, 1.f);

	// 转向玩家
	FRotator LookAtRotation = (TargetPlayer->GetActorLocation() - GetActorLocation()).Rotation();
	LookAtRotation.Pitch = 0.f; // 不上下看
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), LookAtRotation, GetWorld()->GetDeltaSeconds(), 5.f));

	UE_LOG(LogTemp, Warning, TEXT("🐺 正在追逐玩家，当前位置: %s"), *GetActorLocation().ToString());
}

// 攻击玩家
void AParagonFengMao::AttackPlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("🐺 攻击玩家行为执行中..."));

	if (!TargetPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 攻击目标丢失，返回巡逻"));
		SetAIState(EFengMaoAIState::Patrol);
		return;
	}

	float Distance = FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation());
	UE_LOG(LogTemp, Warning, TEXT("🐺 攻击距离: %.1f米，攻击范围: %.1f米"), Distance, AttackRange);

	// 如果玩家远离，切换到追逐
	if (Distance > AttackRange + 50.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 玩家远离，切换到追逐"));
		SetAIState(EFengMaoAIState::Chase);
		return;
	}

	// 检查攻击冷却
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float TimeSinceLastAttack = CurrentTime - LastAttackTime;
	UE_LOG(LogTemp, Warning, TEXT("🐺 攻击冷却: %.1f秒 / %.1f秒"), TimeSinceLastAttack, AttackCooldown);

	if (TimeSinceLastAttack >= AttackCooldown)
	{
		// 执行攻击
		PerformAttack();
		LastAttackTime = CurrentTime;
		UE_LOG(LogTemp, Warning, TEXT("🐺 执行攻击！伤害: %.1f"), AttackDamage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 攻击冷却中，还需等待 %.1f秒"), AttackCooldown - TimeSinceLastAttack);
	}
}

// 执行攻击
void AParagonFengMao::PerformAttack()
{
	UE_LOG(LogTemp, Warning, TEXT("🐺 开始执行攻击..."));

	if (!TargetPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("🐺 错误: 没有攻击目标！"));
		return;
	}

	// 确保目标仍然有效
	if (!TargetPlayer->IsValidLowLevel() || TargetPlayer->IsPendingKillPending())
	{
		UE_LOG(LogTemp, Error, TEXT("🐺 错误: 攻击目标无效！"));
		return;
	}

	// 创建伤害事件并应用伤害
	FDamageEvent DamageEvent;
	FHitResult HitResult;
	FPointDamageEvent PointDamageEvent(AttackDamage, HitResult, GetActorForwardVector(), nullptr);

	float ActualDamage = TargetPlayer->TakeDamage(AttackDamage, PointDamageEvent, nullptr, this);

	UE_LOG(LogTemp, Warning, TEXT("🐺 攻击成功！对 %s 造成 %.1f 伤害 (实际: %.1f)"),
		*TargetPlayer->GetName(), AttackDamage, ActualDamage);

	// 播放攻击动画（如果有的话）
	// 这里可以添加攻击动画播放逻辑

	// 添加攻击特效或音效
	// 这里可以添加粒子效果、音效等
}

// 死亡处理
void AParagonFengMao::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	SetAIState(EFengMaoAIState::Dead);

	UE_LOG(LogTemp, Warning, TEXT("🐺 封魔死亡！"));

	// 停止移动
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}

	// 设置死亡姿势
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 3秒后销毁
	GetWorldTimerManager().SetTimer(
		DeathTimerHandle,
		[this]() {
			Destroy();
		},
		3.f,
		false
	);
}

// 检查玩家 - 备用检测方法
void AParagonFengMao::CheckForPlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("🐺 检查玩家... AI控制器: %s"), AIController ? TEXT("有") : TEXT("无"));

	// 如果没有AI Controller，使用传统方法
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 使用传统检测方法"));

		// 寻找玩家
		if (!TargetPlayer)
		{
			TargetPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
			if (TargetPlayer)
			{
				UE_LOG(LogTemp, Warning, TEXT("🐺 找到玩家: %s"), *TargetPlayer->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("🐺 未找到玩家"));
			}
		}

		// 如果发现玩家，进入追逐状态
		if (TargetPlayer && CanSeePlayer() && CurrentAIState != EFengMaoAIState::Attack)
		{
			UE_LOG(LogTemp, Warning, TEXT("🐺 传统检测成功，看到玩家，进入追逐"));
			SetAIState(EFengMaoAIState::Chase);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 AI控制器负责检测，跳过传统检测"));
	}
}

// 是否能看到玩家
bool AParagonFengMao::CanSeePlayer()
{
	if (!TargetPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 CanSeePlayer: 没有目标"));
		return false;
	}

	float Distance = FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation());
	UE_LOG(LogTemp, Warning, TEXT("🐺 CanSeePlayer: 距离=%.1f米, 检测范围=%.1f米"), Distance, DetectionRange);

	// 检查距离
	if (Distance > DetectionRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 CanSeePlayer: 距离过远"));
		return false;
	}

	// 检查视野角度
	FVector DirectionToPlayer = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector ForwardVector = GetActorForwardVector();

	float DotProduct = FVector::DotProduct(ForwardVector, DirectionToPlayer);
	float Angle = FMath::Acos(DotProduct) * (180.f / PI);

	UE_LOG(LogTemp, Warning, TEXT("🐺 CanSeePlayer: 角度=%.1f度, 视野范围=45度"), Angle);

	// 视野角度45度 (左右各22.5度)
	if (Angle > 45.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 CanSeePlayer: 角度超出视野范围"));
		return false;
	}

	// 射线检测是否有障碍物
	FVector StartLocation = GetActorLocation() + FVector(0, 0, 50);
	FVector EndLocation = TargetPlayer->GetActorLocation() + FVector(0, 0, 50);

	UE_LOG(LogTemp, Warning, TEXT("🐺 CanSeePlayer: 执行射线检测 从 %s 到 %s"),
		*StartLocation.ToString(), *EndLocation.ToString());

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		QueryParams
	);

	if (bHit)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor == TargetPlayer)
		{
			UE_LOG(LogTemp, Warning, TEXT("🐺 CanSeePlayer: ✅ 成功看到玩家"));
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("🐺 CanSeePlayer: ❌ 被障碍物阻挡，击中: %s"),
				HitActor ? *HitActor->GetName() : TEXT("未知"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("🐺 CanSeePlayer: ❌ 射线未命中任何东西"));
	}

	return false;
}

// 获取玩家位置
FVector AParagonFengMao::GetPlayerLocation()
{
	if (TargetPlayer)
	{
		return TargetPlayer->GetActorLocation();
	}
	return FVector::ZeroVector;
}

// 重置攻击
void AParagonFengMao::ResetAttack()
{
	LastAttackTime = 0.f;
}

