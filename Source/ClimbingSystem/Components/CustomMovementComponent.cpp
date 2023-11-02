// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "ClimbingSystem/DebugHelper.h"

#pragma region OverridenFunctions

void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//TraceClimbableSurfaces();
	//TraceFromEyeHeight(100.f);
}

void UCustomMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (IsClimbing())
	{
		bOrientRotationToMovement = false;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(48.f);
	}

	// 離開攀爬狀態
	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == ECustomMovementMode::MOVE_Climb)
	{
		bOrientRotationToMovement = true;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);

		// 恢復角度
		const FRotator dirtyRotation = UpdatedComponent->GetComponentRotation();
		const FRotator cleanStandRotation = FRotator(0.f, dirtyRotation.Yaw, 0.f);
		UpdatedComponent->SetRelativeRotation(cleanStandRotation);

		StopMovementImmediately();
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

#pragma endregion OverridenFunctions

void UCustomMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	if (IsClimbing())
	{
		PhysClimb(deltaTime, Iterations);
	}

	Super::PhysCustom(deltaTime, Iterations);
}

float UCustomMovementComponent::GetMaxSpeed() const
{
	if (IsClimbing())
	{
		return MaxClimbSpeed;
	}

	return Super::GetMaxSpeed();
}

float UCustomMovementComponent::GetMaxAcceleration() const
{
	if (IsClimbing())
	{
		return MaxClimbAcceleration;
	}

	return Super::GetMaxAcceleration();
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

bool UCustomMovementComponent::TraceClimbableSurfaces()
{
	// UpdatedComponent 實際管理位置的Component，MovementComponent會移動、更新它
	const FVector startOffset = UpdatedComponent->GetForwardVector() * 30.f;
	const FVector start = UpdatedComponent->GetComponentLocation() + startOffset;
	const FVector end = start + UpdatedComponent->GetForwardVector();

	ClimbableSurfacesTracedResults = DoCapsuleTraceMultiByObject(start, end, true);

	return !ClimbableSurfacesTracedResults.IsEmpty();
}

FHitResult UCustomMovementComponent::TraceFromEyeHeight(float TraceDistance, float TraceStartOffset/* = 0.f*/)
{
	const FVector componentLocation = UpdatedComponent->GetComponentLocation();
	const FVector eyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);
	const FVector start = componentLocation + eyeHeightOffset;
	const FVector end = start + UpdatedComponent->GetForwardVector() * TraceDistance;

	return DoLineTraceSingleByObject(start, end);
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

void UCustomMovementComponent::StartClimbing()
{
	SetMovementMode(MOVE_Custom, ECustomMovementMode::MOVE_Climb);
}

void UCustomMovementComponent::StopClimbing()
{
	SetMovementMode(MOVE_Falling);
}

void UCustomMovementComponent::PhysClimb(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// Process all the climbable surfaces info
	TraceClimbableSurfaces();
	ProcessClimbableSurfaceInfo();
	
	// Check if we should stop climbing

	RestorePreAdditiveRootMotionVelocity();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		// Define the max climb speed and acceleration
		// 覆寫 GetMaxSpeed() & GetMaxAcceleration() 來調整攀爬速度

		CalcVelocity(deltaTime, 0.f, true, MaxBrakingClimbDeceleration);
	}

	ApplyRootMotionToVelocity(deltaTime);

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);

	// Handle climb rotation
	SafeMoveUpdatedComponent(Adjusted, GetClimbRotation(deltaTime), true, Hit);

	if (Hit.Time < 1.f)
	{
		//adjust and try again
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}

	// Snap movement to climbable surfaces
	SnapMovementToClimbaleSurface(deltaTime);
}

void UCustomMovementComponent::ProcessClimbableSurfaceInfo()
{
	CurrentClimbableSurfacesLocation = FVector::ZeroVector;
	CurrentClimbableSurfacesNormal = FVector::ZeroVector;

	if (ClimbableSurfacesTracedResults.IsEmpty())
		return;

	for (const FHitResult& tracedHitResult : ClimbableSurfacesTracedResults)
	{
		CurrentClimbableSurfacesLocation += tracedHitResult.ImpactPoint;
		CurrentClimbableSurfacesNormal += tracedHitResult.ImpactNormal;
	}

	CurrentClimbableSurfacesLocation /= ClimbableSurfacesTracedResults.Num();
	CurrentClimbableSurfacesNormal = CurrentClimbableSurfacesNormal.GetSafeNormal();
}

FQuat UCustomMovementComponent::GetClimbRotation(float deltaTime)
{
	const FQuat currentQuat = UpdatedComponent->GetComponentQuat();

	// 由 Root Motion 控制就return當前角度
	if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
	{
		return currentQuat;
	}

	// 找到面相 X軸 的角度
	const FQuat targetQuat = FRotationMatrix::MakeFromX(-CurrentClimbableSurfacesNormal).ToQuat();

	return FMath::QInterpTo(currentQuat, targetQuat, deltaTime, 5.f);
}

void UCustomMovementComponent::SnapMovementToClimbaleSurface(float deltaTime)
{
	// 以世界座標標示該角色的前方位置
	const FVector componentForward = UpdatedComponent->GetForwardVector();
	const FVector componentLocation = UpdatedComponent->GetComponentLocation();

	// 計算"可攀爬位置"與"當前位置"之間的距離，方向要與ForwardVector一致
	const FVector projectedCharacterToSurface = (CurrentClimbableSurfacesLocation - componentLocation).ProjectOnTo(componentForward);
	const FVector snapVector = -CurrentClimbableSurfacesNormal * projectedCharacterToSurface.Length();

	// 維持角色貼在牆面上
	UpdatedComponent->MoveComponent(
		snapVector * deltaTime * MaxClimbSpeed,
		UpdatedComponent->GetComponentQuat(),
		true);
}

#pragma endregion ClimbCore

void UCustomMovementComponent::ToggleClimbing(bool bEnableClimb)
{
	if (bEnableClimb)
	{
		if (CanStartClimbing())
		{
			Debug::Print(TEXT("Can Start Climbing"));
			StartClimbing();
		}
		else
		{
			Debug::Print(TEXT("Can NOT Start Climbing"));
		}
	}
	else
	{
		// stop climbing
		StopClimbing();
	}
}

bool UCustomMovementComponent::IsClimbing() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::MOVE_Climb;
}