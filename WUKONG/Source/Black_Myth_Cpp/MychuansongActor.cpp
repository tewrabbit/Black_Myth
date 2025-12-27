#include "MychuansongActor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

AMychuansongActor::AMychuansongActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建 Mesh
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    // 使用 UE 自带 Cube
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube")
    );
    if (CubeMesh.Succeeded())
    {
        Mesh->SetStaticMesh(CubeMesh.Object);
    }

    // 缩放
    Mesh->SetWorldScale3D(FVector(1.f, 1.f, 1.f));
}

void AMychuansongActor::BeginPlay()
{
    Super::BeginPlay();

    // 启用输入
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        EnableInput(PC);
    }

    if (InputComponent)
    {
        InputComponent->BindKey(EKeys::K, IE_Pressed, this, &AMychuansongActor::TeleportToNextLevel);
        UE_LOG(LogTemp, Warning, TEXT("MychuansongActor: Press K to teleport."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MychuansongActor: InputComponent is NULL!"));
    }
}

void AMychuansongActor::TeleportToNextLevel()
{
    if (!GetWorld()) return;

    // 获取当前关卡名（可能带 PIE 前缀）
    FString CurrentLevelName = GetWorld()->GetMapName();

    // 去掉 UEDPIE_x_ 前缀
    FString CleanLevelName = CurrentLevelName;
    CleanLevelName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

    UE_LOG(LogTemp, Warning, TEXT("Current Level: %s"), *CleanLevelName);

    if (CleanLevelName == TEXT("LandscapeAutoMaterial_MountainRange_Example"))
    {
        UE_LOG(LogTemp, Warning, TEXT("Teleport to Desert"));
        UGameplayStatics::OpenLevel(
            GetWorld(),
            FName("LandscapeAutoMaterial_Desert_Example")
        );
    }
    else if (CleanLevelName == TEXT("LandscapeAutoMaterial_Desert_Example"))
    {
        UE_LOG(LogTemp, Warning, TEXT("Teleport to Island"));
        UGameplayStatics::OpenLevel(
            GetWorld(),
            FName("LandscapeAutoMaterial_Island_Example")
        );
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No teleport rule for this level"));
    }
}