// Fill out your copyright notice in the Description page of Project Settings.

#include "FengMaoAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ParagonFengMao.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"

AFengMaoAIController::AFengMaoAIController()
{
	// 创建AI感知组件
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerception);

	// 创建视野感知配置
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1000.f;          // 检测范围
	SightConfig->LoseSightRadius = 1200.f;      // 丢失视野范围
	SightConfig->PeripheralVisionAngleDegrees = 90.f;  // 视野角度
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;

	// 设置主要感知为视野
	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());

	// 启用行为树
	bSetControlRotationFromPawnOrientation = false;
}

void AFengMaoAIController::BeginPlay()
{
	Super::BeginPlay();

	// 绑定感知更新事件
	if (AIPerception)
	{
		AIPerception->OnPerceptionUpdated.AddDynamic(this, &AFengMaoAIController::OnPerceptionUpdated);
	}

	UE_LOG(LogTemp, Warning, TEXT("🤖 FengMao AI Controller initialized"));
}

void AFengMaoAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AParagonFengMao* FengMao = Cast<AParagonFengMao>(InPawn);
	if (FengMao)
	{
		UE_LOG(LogTemp, Warning, TEXT("🤖 ==========================="));
		UE_LOG(LogTemp, Warning, TEXT("🤖 🤖🧠 AI控制器已连接封魔敌人！🧠🤖"));
		UE_LOG(LogTemp, Warning, TEXT("🤖 敌人ID: %s"), *FengMao->GetName());
		UE_LOG(LogTemp, Warning, TEXT("🤖 感知组件: %s"), AIPerception ? TEXT("✅ 已初始化") : TEXT("❌ 未初始化"));
		UE_LOG(LogTemp, Warning, TEXT("🤖 ==========================="));

		// 可以在这里运行行为树
		// if (BehaviorTree)
		// {
		//     RunBehaviorTree(BehaviorTree);
		// }
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("🤖 错误: 连接的Pawn不是FengMao类型！类型: %s"), *InPawn->GetClass()->GetName());
	}
}

void AFengMaoAIController::OnUnPossess()
{
	Super::OnUnPossess();
	UE_LOG(LogTemp, Warning, TEXT("🤖 AI Controller unpossessed"));
}

bool AFengMaoAIController::IsFriendly(AActor* OtherActor) const
{
	// 检查参数有效性
	if (!OtherActor || !GetPawn())
		return false;

	// 自己总是友方
	if (OtherActor == GetPawn())
		return true;

	// 检查是否是敌对目标
	if (OtherActor == HostileTarget)
		return false;

	// 获取当前控制的FengMao和对方FengMao
	AParagonFengMao* ThisFengMao = Cast<AParagonFengMao>(GetPawn());
	AParagonFengMao* OtherFengMao = Cast<AParagonFengMao>(OtherActor);

	// 如果两者都是FengMao类型的敌人
	if (ThisFengMao && OtherFengMao)
	{
		// 检查是否由同一个召唤者召唤
		if (ThisFengMao->Summoner && OtherFengMao->Summoner &&
			ThisFengMao->Summoner == OtherFengMao->Summoner)
		{
			return true; // 同一召唤者召唤的单位是友方
		}

		// 检查对方是否是召唤者本身
		if (OtherActor == ThisFengMao->Summoner)
		{
			return true; // 召唤者是友方
		}
	}

	// 默认情况下，不是友方
	return false;
}

void AFengMaoAIController::SetHostileTarget(AActor* HostileTargetActor)
{
	HostileTarget = HostileTargetActor;
	UE_LOG(LogTemp, Warning, TEXT("🤖 设置敌对目标: %s"), HostileTargetActor ? *HostileTargetActor->GetName() : TEXT("None"));
}

void AFengMaoAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	for (AActor* Actor : UpdatedActors)
	{
		UE_LOG(LogTemp, Warning, TEXT("🤖 AI感知更新: 检测到Actor: %s"), *Actor->GetName());

		// 检查是否是玩家并且不是友方单位
		if (Actor && Actor->IsA(APawn::StaticClass()) && !IsFriendly(Actor))
		{
			AParagonFengMao* FengMao = Cast<AParagonFengMao>(GetPawn());
			if (FengMao)
			{
				UE_LOG(LogTemp, Warning, TEXT("🤖 AI更新目标: %s"), *Actor->GetName());

				// 更新敌人的目标
				FengMao->TargetPlayer = Actor;

				// 检查是否能看到目标
				FActorPerceptionBlueprintInfo Info;
				if (GetPerceptionComponent()->GetActorsPerception(Actor, Info))
				{
					bool bCanSee = false;
					for (const FAIStimulus& Stimulus : Info.LastSensedStimuli)
					{
						if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>() && Stimulus.WasSuccessfullySensed())
						{
							bCanSee = true;
							UE_LOG(LogTemp, Warning, TEXT("🤖 成功感知到玩家: %s"), *Actor->GetName());
							break;
						}
					}

					// 根据是否能看到玩家来改变AI状态
					if (bCanSee)
					{
						float Distance = FVector::Distance(FengMao->GetActorLocation(), Actor->GetActorLocation());
						UE_LOG(LogTemp, Warning, TEXT("🤖 玩家距离: %.1f米, 攻击范围: %.1f米"), Distance, FengMao->AttackRange);

						if (Distance <= FengMao->AttackRange)
						{
							UE_LOG(LogTemp, Warning, TEXT("🤖 切换到攻击状态"));
							FengMao->SetAIState(EFengMaoAIState::Attack);
						}
						else if (Distance <= FengMao->DetectionRange)
						{
							UE_LOG(LogTemp, Warning, TEXT("🤖 切换到追逐状态"));
							FengMao->SetAIState(EFengMaoAIState::Chase);
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("🤖 失去玩家视野，返回巡逻"));
						FengMao->TargetPlayer = nullptr;
						FengMao->SetAIState(EFengMaoAIState::Patrol);
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("🤖 获取感知信息失败"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("🤖 无法获取FengMao引用"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("🤖 忽略的Actor: %s (不是Pawn、自身或敌对目标)"), *Actor->GetName());
		}
	}
}
