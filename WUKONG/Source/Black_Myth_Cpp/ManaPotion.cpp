// Fill out your copyright notice in the Description page of Project Settings.


// ManaPotion.cpp
#include "ManaPotion.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Engine.h"
#include "WukongCharacter.h"
#include "InventoryWidget.h"

AManaPotion::AManaPotion()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    RootComponent = MeshComp;

    SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    SphereComp->SetupAttachment(RootComponent);
    SphereComp->SetSphereRadius(100.f);
    SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SphereComp->SetCollisionResponseToAllChannels(ECR_Overlap);
    SphereComp->OnComponentBeginOverlap.AddDynamic(this, &AManaPotion::OnSphereBeginOverlap);

    // 球模型
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        MeshComp->SetStaticMesh(SphereMesh.Object);
        MeshComp->SetWorldScale3D(FVector(0.3f));
    }

    // 可变色材质
    static ConstructorHelpers::FObjectFinder<UMaterial> MatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
    if (MatFinder.Succeeded())
    {
        UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(MatFinder.Object, this);
        DynMat->SetVectorParameterValue("Color", FLinearColor::Blue);   // 蓝
        MeshComp->SetMaterial(0, DynMat);
    }
}


void AManaPotion::BeginPlay()
{
    Super::BeginPlay();
}

void AManaPotion::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this)
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue,
                FString::Printf(TEXT("Mana potion picked by %s"), *OtherActor->GetName()));
        Destroy();
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            AWukongCharacter* Wukong = Cast<AWukongCharacter>(PC->GetPawn());
            if (Wukong)
            {
                UMyPlayerWidget* HUD = Wukong->getMyPlayerHUD();
                if (HUD)
                {
                    if (UInventoryWidget* Inv = HUD->GetInventoryWidget())
                    {
                        Inv->AddItem(EItemType::ManaPotion, TEXT("Mana Potion"), 1);
                    }
                }
            }
        }
    }
}


// Called every frame
void AManaPotion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

