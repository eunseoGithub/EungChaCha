// Fill out your copyright notice in the Description page of Project Settings.


#include "System/MainPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "System/StonekinSimSubSystem.h"

void AMainPlayerController::Move()
{
	UE_LOG(LogTemp,Log,TEXT("Click"));
	FHitResult Hit;
	bool bHit = GetHitResultUnderCursor(ECC_Visibility,true,Hit);
	
	if (!bHit) return;
	
	const FVector ClickWorld = Hit.ImpactPoint;
	UE_LOG(LogTemp,Log,TEXT("Click Transform : %s"),*ClickWorld.ToString());
	
	StoneSub->SetClickPosition(ClickWorld);
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
