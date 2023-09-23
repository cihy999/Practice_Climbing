// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TraceClimbableSurfaces();
}

#pragma region ClimbTraces

TArray<FHitResult> UCustomMovementComponent::DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebugShape)
{
	TArray<FHitResult> outCapsuleTraceHitResults;

	UKismetSystemLibrary::CapsuleTraceMultiForObjects(
		this,
		Start,
		End,
		ClimbCapsuleTraceRadius,
		ClimbCapsuleTraceHalfHeight,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		bShowDebugShape ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
		outCapsuleTraceHitResults,
		false
	);

	return outCapsuleTraceHitResults;
}

#pragma endregion ClimbTraces

#pragma region ClimbCore

void UCustomMovementComponent::TraceClimbableSurfaces()
{
	// UpdatedComponent 實際管理位置的Component，MovementComponent會移動、更新它
	const FVector startOffset = UpdatedComponent->GetForwardVector() * 30.f;
	const FVector start = UpdatedComponent->GetComponentLocation() + startOffset;
	const FVector end = start + UpdatedComponent->GetForwardVector();

	DoCapsuleTraceMultiByObject(start, end, true);
}

#pragma endregion ClimbCore
