#include "System/MainPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "System/StonekinSimSubSystem.h"
#include "DrawDebugHelpers.h"

void AMainPlayerController::Move()
{
	UE_LOG(LogTemp,Log,TEXT("Click"));
	FHitResult Hit;
	bool bHit = GetHitResultUnderCursor(ECC_Visibility,true,Hit);
	
	if (!bHit) return;
	
	const FVector ClickWorld = Hit.ImpactPoint;
	UE_LOG(LogTemp,Log,TEXT("Click Transform : %s"),*ClickWorld.ToString());
	
	StoneSub->SetClickPosition(ClickWorld);

	// 피보나치 슬롯 디버그
	const int32 Count = StoneSub->GetPositions().Num();
	TArray<FVector> Slots = StoneSub->GenerateFibonacciSlots(ClickWorld, Count, 30.f);

	FlushPersistentDebugLines(GetWorld());

	for (const FVector& Slot : Slots)
	{
		DrawDebugSphere(GetWorld(), Slot, 8.f, 6, FColor::Cyan, true, -1.f);
	}
}

void AMainPlayerController::Attack()
{
}

void AMainPlayerController::InteractCommand()
{
}

void AMainPlayerController::MakeDrag()
{
}

void AMainPlayerController::SetSelect()
{
}

void AMainPlayerController::ClearSelect()
{
}

void AMainPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;
	SetInputMode(FInputModeGameAndUI());
	
	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP) return;
	UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsys) return;
	Subsys->AddMappingContext(DefaultMappingContext,0);
	UE_LOG(LogTemp,Log,TEXT("Controller BeginPlay()"));
	
	StoneSub = GetWorld()->GetSubsystem<UStonekinSimSubSystem>();
	if (!StoneSub) return;
}

void AMainPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC) return;
	
	EIC->BindAction(ClickAction, ETriggerEvent::Started,this,&AMainPlayerController::Move);
}

void AMainPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
}
