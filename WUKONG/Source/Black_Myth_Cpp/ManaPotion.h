// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ManaPotion.generated.h"

UCLASS()
class BLACK_MYTH_CPP_API AManaPotion : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AManaPotion();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* SphereComp;

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
