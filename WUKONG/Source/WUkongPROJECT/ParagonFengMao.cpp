// Fill out your copyright notice in the Description page of Project Settings.

#include "ParagonFengMao.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "FengMaoAIController.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/TargetPoint.h"

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
	LastAttackTime = -100.f;
	LastSkillTime = -100.f;
	LastDodgeTime = -100.f;  // 初始化闪避时间
	TargetPlayer = nullptr;
	AIController = nullptr;
	CurrentPatrolDestination = FVector::ZeroVector;
	bIsAttacking = false;
	bIsUsingSkill = false;
	bIsDodging = false;  // 初始化闪避状态
	bIsAutoTestingDamage = false; // 初始化自动测试状态

	UE_LOG(LogTemp, Warning, TEXT("ewolf ==========================="));
	UE_LOG(LogTemp, Warning, TEXT("ewolf 🏗️ ParagonFengMao构造函数执行中..."));
	UE_LOG(LogTemp, Warning, TEXT("ewolf AI Controller类: %s"), AIControllerClass ? *AIControllerClass->GetName() : TEXT("未设置"));
	UE_LOG(LogTemp, Warning, TEXT("ewolf ==========================="));

	// 设置角色移动
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);
		GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;

		// 禁用根运动，使用代码控制速度
		GetCharacterMovement()->bIgnoreClientMovementErrorChecksAndCorrection = false;

		UE_LOG(LogTemp, Warning, TEXT("ewolf CharacterMovement设置完成"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ewolf 错误: CharacterMovement组件不存在！"));
	}

	// 禁用Mesh的根运动
	if (GetMesh())
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		// 强制禁用根运动
		if (GetMesh()->GetAnimInstance())
		{
			GetMesh()->GetAnimInstance()->RootMotionMode = ERootMotionMode::IgnoreRootMotion;
		}
		UE_LOG(LogTemp, Warning, TEXT("ewolf Mesh动画设置完成，根运动已禁用"));
	}

	// 设置碰撞 - 确保可以被攻击
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // 阻止Pawn碰撞，这样才能被攻击到
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // 忽略相机
		GetCapsuleComponent()->SetGenerateOverlapEvents(true); // 生成重叠事件
		UE_LOG(LogTemp, Warning, TEXT("ewolf CapsuleComponent碰撞设置完成"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ewolf 错误: CapsuleComponent不存在！"));
	}

	// 设置AI Controller类 - 确保在构造函数中就设置好
	AIControllerClass = AFengMaoAIController::StaticClass();
	UE_LOG(LogTemp, Warning, TEXT("ewolf AI Controller类已设置为: %s"), *AIControllerClass->GetName());
	UE_LOG(LogTemp, Warning, TEXT("ewolf AI Controller类设置完成"));

	// 创建血条Widget组件
	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	if (HealthBarWidget)
	{
		HealthBarWidget->SetupAttachment(GetRootComponent());
		HealthBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f)); // 在头顶上方
		HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
		HealthBarWidget->SetDrawSize(FVector2D(150.f, 30.f));
		HealthBarWidget->SetVisibility(false); // 默认隐藏
		UE_LOG(LogTemp, Warning, TEXT("ewolf HealthBarWidget创建完成"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ewolf 错误: HealthBarWidget创建失败！"));
	}

	// 加载攻击动画
	static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageObj(
		TEXT("/Game/ParagonFengMao/Characters/Heroes/FengMao/Animations/Melee_B_Fast_Montage")
	);
	if (AttackMontageObj.Succeeded())
	{
		AttackMontage = AttackMontageObj.Object;
		UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 攻击动画加载成功: %s"), *GetNameSafe(AttackMontage));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf ⚠️ 攻击动画未找到，将不播放动画"));
		AttackMontage = nullptr;
	}

	// 加载重击动画
	static ConstructorHelpers::FObjectFinder<UAnimMontage> HeavyAttackMontageObj(
		TEXT("/Game/ParagonFengMao/Characters/Heroes/FengMao/Animations/Melee_A_Med_Montage")
	);
	if (HeavyAttackMontageObj.Succeeded())
	{
		HeavyAttackMontage = HeavyAttackMontageObj.Object;
		UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 重击动画加载成功: %s"), *GetNameSafe(HeavyAttackMontage));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf ⚠️ 重击动画未找到，将不播放动画"));
		HeavyAttackMontage = nullptr;
	}

	// 加载技能动画（冲刺攻击）
	static ConstructorHelpers::FObjectFinder<UAnimMontage> SkillMontageObj(
		TEXT("/Game/ParagonFengMao/Characters/Heroes/FengMao/Animations/Melee_C_Med_Montage")
	);
	if (SkillMontageObj.Succeeded())
	{
		SkillMontage = SkillMontageObj.Object;
		UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 技能动画加载成功: %s"), *GetNameSafe(SkillMontage));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf ⚠️ 技能动画未找到，将不播放动画"));
		SkillMontage = nullptr;
	}

	// 加载死亡动画
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DeathMontageObj(
		TEXT("/Game/ParagonFengMao/Characters/Heroes/FengMao/Animations/Death_Montage")
	);
	if (DeathMontageObj.Succeeded())
	{
		DeathMontage = DeathMontageObj.Object;
		UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 死亡动画加载成功: %s"), *GetNameSafe(DeathMontage));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf ⚠️ 死亡动画未找到，将不播放动画"));
		DeathMontage = nullptr;
	}
}

// Called after components are initialized
void AParagonFengMao::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	UE_LOG(LogTemp, Warning, TEXT("ewolf PostInitializeComponents: 组件初始化完成"));
	UE_LOG(LogTemp, Warning, TEXT("ewolf AI Controller类: %s"), AIControllerClass ? *AIControllerClass->GetName() : TEXT("未设置"));

	// 确保AI Controller类已设置
	if (!AIControllerClass)
	{
		AIControllerClass = AFengMaoAIController::StaticClass();
		UE_LOG(LogTemp, Warning, TEXT("ewolf 自动设置AI Controller类"));
	}

	// 确保根运动被禁用
	if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->RootMotionMode = ERootMotionMode::IgnoreRootMotion;
		UE_LOG(LogTemp, Warning, TEXT("ewolf 根运动已禁用"));
	}

	// 确保移动组件设置正确
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
		UE_LOG(LogTemp, Warning, TEXT("ewolf 初始速度设置为巡逻速度: %.1f"), PatrolSpeed);
	}

	// 如果没有设置巡逻点，自动生成一些
	if (PatrolPoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 未设置巡逻点，自动生成巡逻路径"));
		GenerateDefaultPatrolPoints();
	}

	// 初始化到巡逻状态
	SetAIState(EFengMaoAIState::Patrol);
}

// Called when the game starts or when spawned
void AParagonFengMao::BeginPlay()
{
	Super::BeginPlay();

	// 获取AI Controller
	AIController = Cast<AFengMaoAIController>(GetController());

	UE_LOG(LogTemp, Warning, TEXT("ewolf ==========================="));
	UE_LOG(LogTemp, Warning, TEXT("ewolf 🐺🦇 封魔敌人出生！🦇🐺"));
	UE_LOG(LogTemp, Warning, TEXT("ewolf ID: %s"), *GetName());
	UE_LOG(LogTemp, Warning, TEXT("ewolf 位置: %s"), *GetActorLocation().ToString());
	UE_LOG(LogTemp, Warning, TEXT("ewolf 生命值: %.1f/%.1f"), CurrentHealth, MaxHealth);
	UE_LOG(LogTemp, Warning, TEXT("ewolf 当前Controller: %s"), GetController() ? *GetController()->GetClass()->GetName() : TEXT("无"));
	UE_LOG(LogTemp, Warning, TEXT("ewolf AI控制器: %s"), AIController ? TEXT("✅ 已连接") : TEXT("❌ 未找到"));

	// 如果没有AI Controller，尝试手动生成
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("ewolf 错误: AI Controller未找到！尝试手动生成..."));

		// 检查AIControllerClass是否设置
		if (AIControllerClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf AI Controller类已设置: %s"), *AIControllerClass->GetName());

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
					UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 手动生成AI Controller成功"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("ewolf ❌ 手动生成AI Controller失败"));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ewolf ❌ AI Controller类未设置！"));
		}
	}

	// 验证移动组件
	if (!GetCharacterMovement())
	{
		UE_LOG(LogTemp, Error, TEXT("ewolf ❌ 错误: CharacterMovement组件不存在！"));
	}
	else
	{
		// 确保移动组件没有被禁用
		if (!GetCharacterMovement()->IsActive())
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf ⚠️ 移动组件未激活，尝试激活..."));
			GetCharacterMovement()->SetActive(true);
		}

		// 确保可以移动
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		// 显示配置的速度值
		UE_LOG(LogTemp, Warning, TEXT("ewolf ⚙️ 速度配置: 巡逻=%.1f, 追逐=%.1f"), PatrolSpeed, ChaseSpeed);

		// 强制设置巡逻速度
		GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;

		UE_LOG(LogTemp, Warning, TEXT("ewolf 移动组件状态: 激活=%d, 模式=%d, 当前速度=%.1f"),
			GetCharacterMovement()->IsActive(),
			(int32)GetCharacterMovement()->MovementMode,
			GetCharacterMovement()->MaxWalkSpeed);
	}

	// 再次确认禁用根运动
	if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->RootMotionMode = ERootMotionMode::IgnoreRootMotion;
		UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 根运动已禁用，使用代码控制速度"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf ⚠️ 动画实例未就绪，稍后会禁用根运动"));
	}

	UE_LOG(LogTemp, Warning, TEXT("ewolf ==========================="));

	// 验证关键设置
	if (!GetCharacterMovement())
	{
		UE_LOG(LogTemp, Error, TEXT("ewolf 错误: CharacterMovement组件不存在！"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf CharacterMovement组件正常"));
	}

	if (!GetCapsuleComponent())
	{
		UE_LOG(LogTemp, Error, TEXT("ewolf 错误: CapsuleComponent不存在！"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf CapsuleComponent正常"));
	}

	// 如果没有设置巡逻点，自动生成一些
	if (PatrolPoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 未设置巡逻点，自动生成巡逻路径"));
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

		// 使用ATargetPoint作为巡逻点（它有根组件，可以正确设置位置）
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 直接在SpawnActor时设置正确的位置，而不是先(0,0,0)再SetActorLocation
		ATargetPoint* PatrolPoint = GetWorld()->SpawnActor<ATargetPoint>(
			ATargetPoint::StaticClass(),
			PatrolPointLocation,  // 直接使用目标位置
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (PatrolPoint)
		{
			// 验证位置是否正确
			FVector ActualLocation = PatrolPoint->GetActorLocation();
			PatrolPoints.Add(PatrolPoint);

			UE_LOG(LogTemp, Warning, TEXT("ewolf 生成巡逻点 %d: 目标=%s, 实际=%s"),
				i, *PatrolPointLocation.ToString(), *ActualLocation.ToString());

			// 如果位置仍然不对，再强制设置一次
			if (!ActualLocation.Equals(PatrolPointLocation, 10.f))
			{
				PatrolPoint->SetActorLocation(PatrolPointLocation);
				UE_LOG(LogTemp, Warning, TEXT("ewolf 强制修正巡逻点位置"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ewolf 失败：无法生成巡逻点 %d"), i);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ewolf 自动生成 %d 个巡逻点完成"), PatrolPoints.Num());
}

// Called every frame
void AParagonFengMao::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 安全检查
	if (!GetWorld() || !IsValid(this))
	{
		UE_LOG(LogTemp, Error, TEXT("ewolf Tick: 世界或对象无效"));
		return;
	}

	// 每5秒输出一次Tick确认信息
	static float LastTickLogTime = 0.f;
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastTickLogTime >= 10.f) // 改为10秒一次
	{
		LastTickLogTime = CurrentTime;
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
		UE_LOG(LogTemp, Warning, TEXT("ewolf 封魔[%s] 状态:%s | 生命:%.1f/%.1f | 目标:%s"),
			*GetName(), *StateText, CurrentHealth, MaxHealth, *TargetInfo);
	}

	if (bIsDead)
	{
		return; // 死亡后不执行AI逻辑
	}

	// 定期检查AI状态的合理性
	static float LastStateCheckTime = 0.f;
	if (CurrentTime - LastStateCheckTime >= 8.f) // 改为8秒一次
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

	case EFengMaoAIState::Dodge:
		// 闪避状态下不需要特殊处理，闪避完成后会自动切换状态
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
			FVector(1, 0, 0), FVector(0, 1, 0), false);

		// 绘制攻击范围
		DrawDebugCircle(GetWorld(), GetActorLocation(), AttackRange, 16, FColor::Red, false, -1.f, 0, 1.f,
			FVector(1, 0, 0), FVector(0, 1, 0), false);

		// 绘制到玩家的连线
		DrawDebugLine(GetWorld(), GetActorLocation(), TargetPlayer->GetActorLocation(), FColor::Cyan, false, -1.f, 0, 1.f);
	}

	// 调试显示状态 - 删除，已经在前面显示了
	// 不再需要额外的调试信息
}

// 验证AI状态的合理性
void AParagonFengMao::ValidateAIState()
{
	// 检查目标是否仍然有效
	if (TargetPlayer && (!TargetPlayer->IsValidLowLevel() || TargetPlayer->IsPendingKillPending()))
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 目标无效，清空目标"));
		TargetPlayer = nullptr;
		SetAIState(EFengMaoAIState::Patrol);
	}

	// 检查状态转换的合理性
	if (CurrentAIState == EFengMaoAIState::Chase && !TargetPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 追逐状态但没有目标，回到巡逻"));
		SetAIState(EFengMaoAIState::Patrol);
	}

	if (CurrentAIState == EFengMaoAIState::Attack && (!TargetPlayer || FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation()) > AttackRange * 2.f))
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 攻击状态但目标过远，回到巡逻"));
		SetAIState(EFengMaoAIState::Patrol);
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
	UE_LOG(LogTemp, Warning, TEXT("ewolf TakeDamage调用: 伤害=%.1f, 来源=%s"),
		DamageAmount, DamageCauser ? *DamageCauser->GetName() : TEXT("未知"));

	if (bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 已死亡，忽略伤害"));
		return 0.f;
	}

	// 有机会触发闪避，不受到伤害
	TryDodge();

	// 如果正在闪避，则不受到伤害
	if (bIsDodging)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 闪避中，忽略伤害"));
		return 0.f;
	}

	float ActualDamage = FMath::Max(0.f, DamageAmount); // 确保伤害不为负数
	CurrentHealth -= ActualDamage;

	UE_LOG(LogTemp, Warning, TEXT("ewolf 封魔受伤: %.1f, 剩余生命: %.1f/%.1f"), ActualDamage, CurrentHealth, MaxHealth);

	// 检查是否死亡
	if (CurrentHealth <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 生命值降至0，触发死亡"));
		Die();
	}
	else
	{
		// 受伤后进入追逐状态
		if (DamageCauser && DamageCauser->IsA(ACharacter::StaticClass()))
		{
			TargetPlayer = DamageCauser;
			UE_LOG(LogTemp, Warning, TEXT("ewolf 锁定攻击者: %s，进入追逐状态"), *DamageCauser->GetName());
			SetAIState(EFengMaoAIState::Chase);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf 伤害来源无效或不是角色"));
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

	UE_LOG(LogTemp, Warning, TEXT("ewolf AI状态改变: %d → %d"), (int32)CurrentAIState, (int32)NewState);
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
		case EFengMaoAIState::Dodge:
			// 闪避时保持当前速度
			break;
		default:
			GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
			break;
		}
	}
}

// 巡逻行为 - 完全重写，简化逻辑
void AParagonFengMao::Patrol()
{
	if (!GetWorld() || bIsDead)
	{
		return;
	}

	// 如果没有AI控制器，使用简单的随机游走
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 没有AI控制器，使用随机游走"));
		// 随机转向
		if (FMath::FRand() < 0.02f)
		{
			FRotator NewRotation = GetActorRotation();
			NewRotation.Yaw += FMath::FRandRange(-45.f, 45.f);
			SetActorRotation(NewRotation);
		}
		// 向前移动
		AddMovementInput(GetActorForwardVector(), 1.f);
		return;
	}

	// 使用AI控制器进行巡逻
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		UE_LOG(LogTemp, Error, TEXT("ewolf 错误: 没有导航系统！请在关卡中添加NavMeshBoundsVolume"));
		return;
	}

	// 检查是否需要新的巡逻目标
	bool bNeedNewDestination = false;

	if (CurrentPatrolDestination.IsZero())
	{
		bNeedNewDestination = true;
		UE_LOG(LogTemp, Warning, TEXT("ewolf 需要新目标：当前目标为零"));
	}
	else
	{
		float DistanceToDestination = FVector::Distance(GetActorLocation(), CurrentPatrolDestination);
		if (DistanceToDestination < AcceptanceRadiusPatrol)
		{
			bNeedNewDestination = true;
			UE_LOG(LogTemp, Warning, TEXT("ewolf 已抵达巡逻点，寻找下一个目标"));
		}
	}

	// 生成新的巡逻目标
	if (bNeedNewDestination)
	{
		CurrentPatrolDestination = GetRandomPatrolLocation();
		if (!CurrentPatrolDestination.IsZero())
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf 新巡逻目标: %s"), *CurrentPatrolDestination.ToString());

			FAIMoveRequest MoveRequest;
			MoveRequest.SetGoalLocation(CurrentPatrolDestination);
			MoveRequest.SetAcceptanceRadius(AcceptanceRadiusPatrol);
			MoveRequest.SetUsePathfinding(true);
			MoveRequest.SetAllowPartialPath(true);  // 允许部分路径
			MoveRequest.SetProjectGoalLocation(true); // 投影目标到导航网格

			FPathFollowingRequestResult Result = AIController->MoveTo(MoveRequest);

			if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
			{
				UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 移动请求成功"));
			}
			else if (Result.Code == EPathFollowingRequestResult::Failed)
			{
				UE_LOG(LogTemp, Error, TEXT("ewolf ❌ 移动请求失败！NavMesh不可用，切换到简单移动模式"));
				// 如果导航失败，使用简单的AddMovementInput方式
				// 不设置CurrentPatrolDestination为零，让下面的简单移动逻辑处理
			}
			else if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
			{
				UE_LOG(LogTemp, Warning, TEXT("ewolf 已在目标位置"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ewolf 错误：无法获取有效的巡逻位置！"));
		}
	}
	else
	{
		// 检查移动状态，如果停止了就使用简单移动
		// 使用更新的API检查移动状态而不是EPathFollowingStatus::Moving
		bool bIsMoving = false;
		if (AIController && AIController->GetPathFollowingComponent())
		{
			// 在UE5.7中使用正确的API检查移动状态
			bIsMoving = AIController->GetPathFollowingComponent()->GetStatus() != EPathFollowingStatus::Idle;
		}

		if (!bIsMoving)
		{
			// 使用简单的移动方式（不依赖NavMesh）
			// 确保速度被设置
			if (GetCharacterMovement())
			{
				GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
			}

			FVector Direction = (CurrentPatrolDestination - GetActorLocation()).GetSafeNormal();
			float DistanceToTarget = FVector::Distance(GetActorLocation(), CurrentPatrolDestination);

			if (DistanceToTarget > AcceptanceRadiusPatrol)
			{
				// 直接添加移动输入
				AddMovementInput(Direction, 1.0f);

				// 每秒只打印一次日志，避免刷屏
				static float LastSimpleMoveLogTime = 0.f;
				float CurrentTime = GetWorld()->GetTimeSeconds();
				if (CurrentTime - LastSimpleMoveLogTime >= 2.0f)
				{
					LastSimpleMoveLogTime = CurrentTime;
					UE_LOG(LogTemp, Warning, TEXT("ewolf 使用简单移动模式，距离目标: %.1f米, 当前速度: %.1f"),
						DistanceToTarget, GetCharacterMovement() ? GetCharacterMovement()->MaxWalkSpeed : 0.f);
				}
			}
			else
			{
				// 到达目标，重置以获取新目标
				CurrentPatrolDestination = FVector::ZeroVector;
				UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 到达巡逻点（简单移动模式）"));
			}
		}
	}
}

void AParagonFengMao::ChasePlayer()
{
	if (!TargetPlayer || bIsDead)
	{
		SetAIState(EFengMaoAIState::Patrol);
		return;
	}

	float Distance = FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation());

	// 目标过远，放弃追逐 (保持原有逻辑)
	if (Distance > DetectionRange * 1.5f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 目标过远 (%.1f米)，放弃追逐"), Distance);
		TargetPlayer = nullptr;
		SetAIState(EFengMaoAIState::Patrol);
		return;
	}

	// 进入攻击范围 (保持原有逻辑)
	if (Distance <= AttackRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 进入攻击范围 (%.1f米)"), Distance);
		SetAIState(EFengMaoAIState::Attack);
		return;
	}

	// 尝试使用技能 (保持原有逻辑)
	if (!bIsUsingSkill)
	{
		TryUseSkill();
	}

	// ----------------------------------------------------
	// 👇 核心修改：统一使用简单移动逻辑 (因为没有NavMesh)
	// ----------------------------------------------------
	if (!bIsUsingSkill && GetCharacterMovement())
	{
		// 1. 确保设置追逐速度 (速度应为 400)
		GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;

		// 2. 计算方向并进行移动
		FVector Direction = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();

		// 停止AIController正在进行的MoveTo操作（以防万一）
		if (AIController)
		{
			AIController->StopMovement();
		}

		// 直接使用AddMovementInput驱动移动
		AddMovementInput(Direction, 1.0f);

		// 调试日志 (避免刷屏)
		static float LastSimpleMoveLogTime = 0.f;
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastSimpleMoveLogTime >= 2.0f)
		{
			LastSimpleMoveLogTime = CurrentTime;
			UE_LOG(LogTemp, Warning, TEXT("ewolf 🚀 追逐：使用简单移动模式，距离目标: %.1f米, 当前速度: %.1f"),
				Distance, GetCharacterMovement()->MaxWalkSpeed);
		}
	}
	// ----------------------------------------------------

	// 面向目标 (保持原有逻辑)
	FaceTarget(TargetPlayer, GetWorld()->GetDeltaSeconds());
}

// 注意: FaceTarget() 函数未在提供的代码中，假设它用于旋转角色。
// 攻击玩家 - 添加重击逻辑
void AParagonFengMao::AttackPlayer()
{
	if (!TargetPlayer || bIsDead)
	{
		SetAIState(EFengMaoAIState::Patrol);
		return;
	}

	float Distance = FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation());

	// 如果玩家远离，切换到追逐
	if (Distance > AttackRange * 1.8f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 玩家远离 (%.1f米)，切换到追逐"), Distance);
		bIsAttacking = false;
		SetAIState(EFengMaoAIState::Chase);
		return;
	}

	// 面向玩家
	FaceTarget(TargetPlayer, GetWorld()->GetDeltaSeconds());

	// 停止移动（攻击时站立）
	if (AIController)
	{
		AIController->StopMovement();
	}

	// 检查攻击冷却
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float TimeSinceLastAttack = CurrentTime - LastAttackTime;

	if (TimeSinceLastAttack >= AttackCooldown)
	{
		bIsAttacking = true;
		LastAttackTime = CurrentTime;

		// 有一定概率执行重击
		if (FMath::FRand() < HeavyAttackChance)
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf 准备执行重击！"));
			SetAIState(EFengMaoAIState::HeavyAttack);
			PerformHeavyAttack();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf 执行普通攻击！伤害: %.1f"), AttackDamage);
			PerformAttack();
		}

		// 0.5秒后重置攻击状态
		FTimerHandle AttackResetHandle;
		GetWorldTimerManager().SetTimer(AttackResetHandle, [this]()
			{
				bIsAttacking = false;
			}, 0.5f, false);
	}
}

// 尝试使用技能 - 优化
void AParagonFengMao::TryUseSkill()
{
	if (!TargetPlayer || bIsDead || bIsUsingSkill)
	{
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastSkillTime < SkillCooldown)
	{
		return;
	}

	float Distance = FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation());

	// 只有在中等距离时才使用技能
	if (Distance > AttackRange * 1.2f && Distance <= SkillRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 使用技能：冲刺！距离: %.1f米"), Distance);
		DashToTargetAndStrike();
		LastSkillTime = CurrentTime;
	}
}

// 冲刺技能 - 优化
void AParagonFengMao::DashToTargetAndStrike()
{
	if (!TargetPlayer)
	{
		return;
	}

	bIsUsingSkill = true;

	// 停止当前移动
	if (AIController)
	{
		AIController->StopMovement();
	}

	// 面向目标
	FRotator LookAtRotation = (TargetPlayer->GetActorLocation() - GetActorLocation()).Rotation();
	LookAtRotation.Pitch = 0.f;
	SetActorRotation(LookAtRotation);

	// 播放技能动画
	if (PlaySkillMontage())
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 播放冲刺技能动画"));
	}

	// 加速冲刺
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed * DashSpeedMultiplier;
	}

	// 开始冲刺
	if (AIController)
	{
		AIController->MoveToActor(TargetPlayer, AttackRange * 0.8f, true, true, true, 0, true);
	}

	UE_LOG(LogTemp, Warning, TEXT("ewolf 开始冲刺，速度: %.1f"), ChaseSpeed * DashSpeedMultiplier);

	// 设置定时器结束冲刺
	GetWorldTimerManager().SetTimer(SkillTimerHandle, this, &AParagonFengMao::FinishDash, DashDuration, false);
}

// 结束冲刺 - 优化
void AParagonFengMao::FinishDash()
{
	bIsUsingSkill = false;

	// 恢复速度
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
	}

	// 如果移动到了目标范围内，造成伤害
	if (TargetPlayer)
	{
		float Distance = FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation());
		UE_LOG(LogTemp, Warning, TEXT("ewolf 冲刺结束，距离: %.1f米"), Distance);

		if (Distance <= AttackRange * 1.5f)
		{
			FDamageEvent DamageEvent;
			FHitResult HitResult;
			FPointDamageEvent PointDamageEvent(SkillDamage, HitResult, GetActorForwardVector(), nullptr);
			TargetPlayer->TakeDamage(SkillDamage, PointDamageEvent, nullptr, this);

			UE_LOG(LogTemp, Warning, TEXT("ewolf 冲刺命中！造成 %.1f 伤害"), SkillDamage);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf 冲刺未命中"));
		}
	}
}

// 执行攻击 - 优化版本
void AParagonFengMao::PerformAttack()
{
	if (!TargetPlayer)
	{
		return;
	}

	// 确保目标仍然有效
	if (!TargetPlayer->IsValidLowLevel() || TargetPlayer->IsPendingKillPending())
	{
		return;
	}

	// 播放攻击动画
	if (PlayAttackMontage())
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 播放攻击动画"));
	}

	// 创建伤害事件并应用伤害
	FDamageEvent DamageEvent;
	FHitResult HitResult;
	FPointDamageEvent PointDamageEvent(AttackDamage, HitResult, GetActorForwardVector(), nullptr);

	float ActualDamage = TargetPlayer->TakeDamage(AttackDamage, PointDamageEvent, nullptr, this);

	UE_LOG(LogTemp, Warning, TEXT("ewolf 攻击成功！对 %s 造成 %.1f 伤害 (实际: %.1f)"),
		*TargetPlayer->GetName(), AttackDamage, ActualDamage);
}

// 执行重击
void AParagonFengMao::PerformHeavyAttack()
{
	if (!TargetPlayer)
	{
		return;
	}

	// 确保目标仍然有效
	if (!TargetPlayer->IsValidLowLevel() || TargetPlayer->IsPendingKillPending())
	{
		return;
	}

	// 播放重击动画
	if (PlayHeavyAttackMontage())
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 播放重击动画"));
	}

	// 创建伤害事件并应用伤害
	FDamageEvent DamageEvent;
	FHitResult HitResult;
	FPointDamageEvent PointDamageEvent(HeavyAttackDamage, HitResult, GetActorForwardVector(), nullptr);

	float ActualDamage = TargetPlayer->TakeDamage(HeavyAttackDamage, PointDamageEvent, nullptr, this);

	UE_LOG(LogTemp, Warning, TEXT("ewolf 重击成功！对 %s 造成 %.1f 伤害 (实际: %.1f)"),
		*TargetPlayer->GetName(), HeavyAttackDamage, ActualDamage);
}

// 播放重击动画
bool AParagonFengMao::PlayHeavyAttackMontage()
{
	if (!HeavyAttackMontage || !GetMesh() || !GetMesh()->GetAnimInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 重击动画资源缺失"));
		return false;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(HeavyAttackMontage, 1.0f);
		UE_LOG(LogTemp, Warning, TEXT("ewolf 开始播放重击动画"));
		return true;
	}

	return false;
}

// 死亡处理 - 添加死亡动画
void AParagonFengMao::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	SetAIState(EFengMaoAIState::Dead);

	UE_LOG(LogTemp, Warning, TEXT("ewolf 封魔死亡！"));

	// 停止移动
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}

	// 播放死亡动画
	if (DeathMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(DeathMontage, 1.0f);
			UE_LOG(LogTemp, Warning, TEXT("ewolf 播放死亡动画"));
		}
	}
	else
	{
		// 如果没有死亡动画，设置布娃娃效果
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

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

// 检查玩家 - 主动检测方法
void AParagonFengMao::CheckForPlayer()
{
	// 主动寻找玩家（无论是否有AI Controller）
	if (!TargetPlayer)
	{
		TargetPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (TargetPlayer)
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf 发现玩家：%s"), *TargetPlayer->GetName());
		}
	}

	// 如果有玩家目标，检查是否应该攻击
	if (TargetPlayer && !bIsDead)
	{
		float Distance = FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation());

		// 每5秒输出一次检测信息
		static float LastCheckLogTime = 0.f;
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastCheckLogTime >= 5.0f)
		{
			LastCheckLogTime = CurrentTime;
			UE_LOG(LogTemp, Warning, TEXT("ewolf 检测玩家距离: %.1f米, 攻击范围: %.1f米, 检测范围: %.1f米"),
				Distance, AttackRange, DetectionRange);
		}

		// 在攻击范围内
		if (Distance <= AttackRange && CurrentAIState != EFengMaoAIState::Attack)
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 玩家进入攻击范围！距离: %.1f米"), Distance);
			SetAIState(EFengMaoAIState::Attack);
		}
		// 在检测范围内但不在攻击范围
		else if (Distance <= DetectionRange && CurrentAIState == EFengMaoAIState::Patrol)
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 玩家进入检测范围！开始追逐，距离: %.1f米"), Distance);
			SetAIState(EFengMaoAIState::Chase);
		}
		// 超出检测范围
		else if (Distance > DetectionRange * 1.5f && (CurrentAIState == EFengMaoAIState::Chase || CurrentAIState == EFengMaoAIState::Attack))
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf 玩家过远，返回巡逻，距离: %.1f米"), Distance);
			TargetPlayer = nullptr;
			SetAIState(EFengMaoAIState::Patrol);
		}
	}
}

// 是否能看到玩家
bool AParagonFengMao::CanSeePlayer()
{
	if (!TargetPlayer)
	{
		return false;
	}

	float Distance = FVector::Distance(GetActorLocation(), TargetPlayer->GetActorLocation());

	// 检查距离
	if (Distance > DetectionRange)
	{
		return false;
	}

	// 检查视野角度
	FVector DirectionToPlayer = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector ForwardVector = GetActorForwardVector();

	float DotProduct = FVector::DotProduct(ForwardVector, DirectionToPlayer);
	float Angle = FMath::Acos(DotProduct) * (180.f / PI);

	// 视野角度90度 (左右各45度)
	if (Angle > 90.f)
	{
		return false;
	}

	// 射线检测是否有障碍物
	FVector StartLocation = GetActorLocation() + FVector(0, 0, 50);
	FVector EndLocation = TargetPlayer->GetActorLocation() + FVector(0, 0, 50);

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
		return (HitActor == TargetPlayer);
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
	LastAttackTime = -100.f;
}

// 面向目标
void AParagonFengMao::FaceTarget(AActor* Target, float DeltaTime)
{
	if (!Target)
	{
		return;
	}

	FRotator LookAtRotation = (Target->GetActorLocation() - GetActorLocation()).Rotation();
	LookAtRotation.Pitch = 0.f; // 保持水平

	// 平滑旋转
	FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), LookAtRotation, DeltaTime, 8.f);
	SetActorRotation(NewRotation);
}

// 获取随机巡逻位置
FVector AParagonFengMao::GetRandomPatrolLocation()
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("ewolf GetRandomPatrolLocation: 没有World"));
		return FVector::ZeroVector;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		UE_LOG(LogTemp, Error, TEXT("ewolf GetRandomPatrolLocation: 没有导航系统"));
	}

	// 先检查是否有巡逻点
	if (PatrolPoints.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 有 %d 个巡逻点，当前索引: %d"), PatrolPoints.Num(), CurrentPatrolIndex);

		// 使用设定的巡逻点
		AActor* PatrolPoint = PatrolPoints[CurrentPatrolIndex];
		if (PatrolPoint && IsValid(PatrolPoint))
		{
			FVector Location = PatrolPoint->GetActorLocation();
			UE_LOG(LogTemp, Warning, TEXT("ewolf 使用巡逻点 %d: %s"), CurrentPatrolIndex, *Location.ToString());

			// 检查位置是否有效（不是零向量）
			if (!Location.IsZero())
			{
				// 如果有导航系统，验证这个位置是否可达
				if (NavSys)
				{
					FNavLocation NavLoc;
					if (NavSys->ProjectPointToNavigation(Location, NavLoc, FVector(500.f, 500.f, 500.f)))
					{
						UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 巡逻点 %d 位置有效且可达"), CurrentPatrolIndex);
						CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
						return NavLoc.Location;
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("ewolf ⚠️ 巡逻点 %d 不在NavMesh上，但仍然使用（简单移动模式）"), CurrentPatrolIndex);
						// 即使不在NavMesh上，也返回这个位置，用简单移动模式
						CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
						return Location;
					}
				}
				else
				{
					// 没有导航系统时直接返回位置
					UE_LOG(LogTemp, Warning, TEXT("ewolf ⚠️ 无导航系统，直接使用巡逻点位置（简单移动模式）"));
					CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
					return Location;
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ewolf ❌ 巡逻点 %d 位置是零向量！这是生成巡逻点时的BUG！"), CurrentPatrolIndex);
			}

			// 尝试下一个巡逻点
			CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ewolf 巡逻点 %d 为空"), CurrentPatrolIndex);
			CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
		}
	}

	// 如果有导航系统，尝试随机生成位置
	if (NavSys)
	{
		FNavLocation RandomLoc;
		float SearchRadius = 800.f;

		UE_LOG(LogTemp, Warning, TEXT("ewolf 尝试从 %s 搜索半径 %.1f 的可达位置"), *GetActorLocation().ToString(), SearchRadius);

		if (NavSys->GetRandomReachablePointInRadius(GetActorLocation(), SearchRadius, RandomLoc))
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf ✅ 找到随机可达位置: %s"), *RandomLoc.Location.ToString());
			return RandomLoc.Location;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ewolf ⚠️ NavMesh找不到可达位置，将使用后备位置（简单移动模式）"));
		}
	}

	// 最后的后备方案：直接返回附近的位置（不检查NavMesh）
	FVector RandomOffset = FVector(
		FMath::FRandRange(-500.f, 500.f),
		FMath::FRandRange(-500.f, 500.f),
		0.f
	);

	FVector FallbackLocation = GetActorLocation() + RandomOffset;
	UE_LOG(LogTemp, Warning, TEXT("ewolf ⚠️ 使用后备位置（无导航验证）: %s"), *FallbackLocation.ToString());
	return FallbackLocation;
}

// 播放攻击动画
bool AParagonFengMao::PlayAttackMontage()
{
	if (!AttackMontage)
	{
		return false;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return false;
	}

	// 检查是否已经在播放政击动画
	if (AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		return false;
	}

	// 播放动画
	float PlayLength = AnimInstance->Montage_Play(AttackMontage, 1.5f);
	if (PlayLength > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 攻击动画播放成功，长度: %.2f秒"), PlayLength);
		return true;
	}

	return false;
}

// 播放技能动画
bool AParagonFengMao::PlaySkillMontage()
{
	if (!SkillMontage)
	{
		return false;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return false;
	}

	// 检查是否已经在播放技能动画
	if (AnimInstance->Montage_IsPlaying(SkillMontage))
	{
		return false;
	}

	// 播放动画
	float PlayLength = AnimInstance->Montage_Play(SkillMontage, 1.2f);
	if (PlayLength > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 技能动画播放成功，长度: %.2f秒"), PlayLength);
		return true;
	}

	return false;
}

// 尝试闪避 - 随机决定是否闪避
void AParagonFengMao::TryDodge()
{
	// 检查是否可以闪避
	if (bIsDead || bIsDodging)
	{
		return;
	}

	// 检查闪避冷却时间
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastDodgeTime < DodgeCooldown)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 闪避冷却中，剩余时间: %.2f秒"), DodgeCooldown - (CurrentTime - LastDodgeTime));
		return;
	}

	// 随机决定是否闪避
	if (FMath::FRand() < DodgeChance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 决定闪避！概率: %.2f"), DodgeChance);
		PerformDodge();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 未触发闪避，概率: %.2f"), DodgeChance);
	}
}

// 执行闪避 - 瞬移跑开
void AParagonFengMao::PerformDodge()
{
	if (bIsDead || !GetWorld())
	{
		return;
	}

	// 设置闪避状态
	bIsDodging = true;
	LastDodgeTime = GetWorld()->GetTimeSeconds();
	SetAIState(EFengMaoAIState::Dodge);

	UE_LOG(LogTemp, Warning, TEXT("ewolf 开始闪避动作"));

	// 停止当前移动
	if (AIController)
	{
		AIController->StopMovement();
	}

	// 确保角色可以移动
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	// 计算闪避方向 - 随机选择一个方向
	FVector DodgeDirection = FVector::ZeroVector;

	// 70%概率向侧面或后面闪避，30%概率向任意方向闪避
	if (FMath::FRand() < 0.7f && TargetPlayer)
	{
		// 向侧面或后面闪避，远离玩家
		FVector ToPlayer = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		FVector RightVector = GetActorRightVector();

		// 随机选择向左或向右闪避
		if (FMath::FRand() < 0.5f)
		{
			DodgeDirection = RightVector;  // 向右闪避
		}
		else
		{
			DodgeDirection = -RightVector; // 向左闪避
		}

		// 30%概率向后闪避
		if (FMath::FRand() < 0.3f)
		{
			DodgeDirection = -ToPlayer;  // 向后闪避
		}

		UE_LOG(LogTemp, Warning, TEXT("ewolf 向侧方或后方闪避"));
	}
	else
	{
		// 随机方向闪避
		DodgeDirection = FVector(FMath::RandRange(-1.0f, 1.0f), FMath::RandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal();
		UE_LOG(LogTemp, Warning, TEXT("ewolf 向随机方向闪避"));
	}

	// 确保方向有效
	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = GetActorForwardVector();
	}

	// 计算目标位置
	FVector CurrentLocation = GetActorLocation();
	FVector DodgeTargetLocation = CurrentLocation + (DodgeDirection * DodgeDistance);

	// 使用简单移动方式瞬移
	// 先瞬移到目标位置
	SetActorLocation(DodgeTargetLocation);

	UE_LOG(LogTemp, Warning, TEXT("ewolf 执行闪避！方向: %s, 距离: %.1f, 新位置: %s"),
		*DodgeDirection.ToString(), DodgeDistance, *DodgeTargetLocation.ToString());

	// 设置定时器结束闪避
	GetWorldTimerManager().SetTimer(DodgeTimerHandle, this, &AParagonFengMao::FinishDodge, 0.5f, false);
}

// 结束闪避
void AParagonFengMao::FinishDodge()
{
	bIsDodging = false;

	// 如果不是死亡状态，恢复到之前的AI状态
	if (!bIsDead)
	{
		// 默认回到巡逻状态
		SetAIState(EFengMaoAIState::Patrol);
	}

	UE_LOG(LogTemp, Warning, TEXT("ewolf 闪避结束，恢复巡逻状态"));
}

// 测试受到伤害函数
void AParagonFengMao::TestTakeDamage()
{
	UE_LOG(LogTemp, Warning, TEXT("ewolf 测试受到伤害！"));

	// 创建一个伤害事件
	FDamageEvent DamageEvent;

	// 对自己造成20点伤害（用于测试闪避）
	float TestDamage = 20.0f;

	// 调用受伤函数
	TakeDamage(TestDamage, DamageEvent, nullptr, nullptr);

	UE_LOG(LogTemp, Warning, TEXT("ewolf 测试伤害已施加: %.1f"), TestDamage);
}

// 控制台命令处理函数
void AParagonFengMao::ApplyTestDamage()
{
	UE_LOG(LogTemp, Warning, TEXT("ewolf 控制台命令：应用测试伤害"));
	TestTakeDamage();
}

// 启动自动测试伤害
void AParagonFengMao::StartAutoDamageTest()
{
	if (bIsAutoTestingDamage)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 自动测试伤害已在运行中"));
		return;
	}

	bIsAutoTestingDamage = true;
	UE_LOG(LogTemp, Warning, TEXT("ewolf 启动自动测试伤害，每2秒受到一次伤害"));

	// 设置定时器，每2秒调用一次TestTakeDamage
	GetWorldTimerManager().SetTimer(
		AutoDamageTestTimerHandle,
		this,
		&AParagonFengMao::TestTakeDamage,
		2.0f,
		true // 循环执行
	);
}

// 停止自动测试伤害
void AParagonFengMao::StopAutoDamageTest()
{
	if (!bIsAutoTestingDamage)
	{
		UE_LOG(LogTemp, Warning, TEXT("ewolf 自动测试伤害未在运行"));
		return;
	}

	bIsAutoTestingDamage = false;
	GetWorldTimerManager().ClearTimer(AutoDamageTestTimerHandle);
	UE_LOG(LogTemp, Warning, TEXT("ewolf 停止自动测试伤害"));
}
