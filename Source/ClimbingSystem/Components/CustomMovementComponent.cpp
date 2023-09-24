// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"

void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TraceClimbableSurfaces();
	TraceFromEyeHeight(100.f);
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

FHitResult UCustomMovementComponent::DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebugShape)
{
	FHitResult outHitResult;

	UKismetSystemLibrary::LineTraceSingleForObjects(
		this,
		Start,
		End,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		bShowDebugShape ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
		outHitResult,
		false
	);

	return outHitResult;
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

void UCustomMovementComponent::TraceFromEyeHeight(float TraceDistance, float TraceStartOffset/* = 0.f*/)
{
	const FVector componentLocation = UpdatedComponent->GetComponentLocation();
	const FVector eyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);
	const FVector start = componentLocation + eyeHeightOffset;
	const FVector end = start + UpdatedComponent->GetForwardVector() * TraceDistance;

	DoLineTraceSingleByObject(start, end, true);
}

#pragma endregion ClimbCore
