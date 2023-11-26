// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"

UENUM(BlueprintType)
namespace ECustomMovementMode
{
	enum Type
	{
		MOVE_Climb	UMETA(DisplayName = "Climb Mode"),
	};
}

/**
 * 
 */
UCLASS()
class CLIMBINGSYSTEM_API UCustomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

#pragma region OverridenFunctions

protected:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;

#pragma endregion OverridenFunctions

#pragma region ClimbTraces

private:
	TArray<FHitResult> DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebugShape = false, bool bDrawPersistantShapes = false);
	FHitResult DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebugShape = false, bool bDrawPersistantShapes = false);

#pragma endregion ClimbTraces

#pragma region ClimbCore

private:
	/// <summary>
	/// 追蹤可以攀爬的表面
	/// </summary>
	/// <return>true=可以</return>
	bool TraceClimbableSurfaces();
	FHitResult TraceFromEyeHeight(float TraceDistance, float TraceStartOffset = 0.f);

	bool CanStartClimbing();

	void StartClimbing();
	void StopClimbing();

	void PhysClimb(float deltaTime, int32 Iterations);

	void ProcessClimbableSurfaceInfo();

	bool CheckShouldStopClimbing();

	FQuat GetClimbRotation(float deltaTime);

	void SnapMovementToClimbaleSurface(float deltaTime);

#pragma endregion ClimbCore

#pragma region ClimbVariables

private:
	TArray<FHitResult> ClimbableSurfacesTracedResults;
	FVector CurrentClimbableSurfacesLocation;
	FVector CurrentClimbableSurfacesNormal;

#pragma endregion ClimbVariables

#pragma region BP_ClimbVariables

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	TArray<TEnumAsByte<EObjectTypeQuery>> ClimbableSurfaceTraceTypes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceHalfHeight = 72.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float MaxBrakingClimbDeceleration = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float MaxClimbSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float MaxClimbAcceleration = 300.f;

#pragma endregion BP_ClimbVariables

public:
	void ToggleClimbing(bool bEnableClimb);
	bool IsClimbing() const;
	FORCEINLINE FVector GetClimbableSurfaceNormal() const { return CurrentClimbableSurfacesNormal; }
};
