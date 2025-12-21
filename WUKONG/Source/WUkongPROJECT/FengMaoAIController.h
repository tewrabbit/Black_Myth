
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FengMaoAIController.generated.h"

/**
 * AI Controller for FengMao Enemy
 * Controls the AI behavior of the ParagonFengMao enemy character
 */
UCLASS()
class WUKONGPROJECT_API AFengMaoAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFengMaoAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// 设置敌对目标
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetHostileTarget(AActor* HostileTargetActor);

	// AI感知更新
	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

	// 检查是否是友方单位
	bool IsFriendly(AActor* OtherActor) const;

protected:
	virtual void BeginPlay() override;

private:
	// 敌对目标
	UPROPERTY()
	AActor* HostileTarget;

	// AI感知组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	class UAIPerceptionComponent* AIPerception;

	// 感知配置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	class UAISenseConfig_Sight* SightConfig;
};