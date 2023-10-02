// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "ClimbingSystem/DebugHelper.h"

void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//TraceClimbableSurfaces();
	//TraceFromEyeHeight(100.f);
}

#pragma region ClimbTraces

TArray<FHitResult> UCustomMovementComponent::DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebugShape, bool bDrawPersistantShapes)
{
	TArray<FHitResult> outCapsuleTraceHitResults;
	EDrawDebugTrace::Type debugTraceType = EDrawDebugTrace::None;

	if (bShowDebugShape)
	{
		debugTraceType = EDrawDebugTrace::ForOneFrame;

		if (bDrawPersistantShapes)
		{
			debugTraceType = EDrawDebugTrace::Persistent;
		}
	}

	UKismetSystemLibrary::CapsuleTraceMultiForObjects(
		this,
		Start,
		End,
		ClimbCapsuleTraceRadius,
		ClimbCapsuleTraceHalfHeight,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		debugTraceType,
		outCapsuleTraceHitResults,
		false
	);

	return outCapsuleTraceHitResults;
}

FHitResult UCustomMovementComponent::DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebugShape, bool bDrawPersistantShapes)
{
	FHitResult outHitResult;
	EDrawDebugTrace::Type debugTraceType = EDrawDebugTrace::None;

	if (bShowDebugShape)
	{
		debugTraceType = EDrawDebugTrace::ForOneFrame;

		if (bDrawPersistantShapes)
		{
			debugTraceType = EDrawDebugTrace::Persistent;
		}
	}

	UKismetSystemLibrary::LineTraceSingleForObjects(
		this,
		Start,
		End,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		debugTraceType,
		outHitResult,
		false
	);

	return outHitResult;
}

#pragma endregion ClimbTraces

#pragma region ClimbCore

void UCustomMovementComponent::ToggleClimbing(bool bEnableClimb)
{
	if (bEnableClimb)
	{
		if (CanStartClimbing())
		{
			Debug::Print(TEXT("Can Start Climbing"));
		}
		else
		{
			Debug::Print(TEXT("Can NOT Start Climbing"));
		}
	}
	else
	{
		// stop climbing
	}
}

bool UCustomMovementComponent::CanStartClimbing()
{
	if (IsFalling())
		return false;

	if (!TraceClimbableSurfaces())
		return false;

	if (!TraceFromEyeHeight(100.f).bBlockingHit)
		return false;

	return true;
}

bool UCustomMovementComponent::IsClimbing() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::MOVE_Climb;
}

bool UCustomMovementComponent::TraceClimbableSurfaces()
{
	// UpdatedComponent 實際管理位置的Component，MovementComponent會移動、更新它
	const FVector startOffset = UpdatedComponent->GetForwardVector() * 30.f;
	const FVector start = UpdatedComponent->GetComponentLocation() + startOffset;
	const FVector end = start + UpdatedComponent->GetForwardVector();

	ClimbableSurfacesTracedResults = DoCapsuleTraceMultiByObject(start, end, true, true);

	return !ClimbableSurfacesTracedResults.IsEmpty();
}

FHitResult UCustomMovementComponent::TraceFromEyeHeight(float TraceDistance, float TraceStartOffset/* = 0.f*/)
{
	const FVector componentLocation = UpdatedComponent->GetComponentLocation();
	const FVector eyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);
	const FVector start = componentLocation + eyeHeightOffset;
	const FVector end = start + UpdatedComponent->GetForwardVector() * TraceDistance;

	return DoLineTraceSingleByObject(start, end, true, true);
}

#pragma endregion ClimbCore
