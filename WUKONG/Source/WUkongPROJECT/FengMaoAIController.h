// Fill out your copyright notice in the Description page of Project Settings.

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

	// AI感知更新
	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

protected:
	virtual void BeginPlay() override;

private:
	// AI感知组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	class UAIPerceptionComponent* AIPerception;

	// 感知配置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	class UAISenseConfig_Sight* SightConfig;
};