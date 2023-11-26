// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimInstnace/CharacterAnimInstance.h"
#include "../ClimbingSystemCharacter.h"
#include "Components/CustomMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ClimbingSystemCharacter = Cast<AClimbingSystemCharacter>(TryGetPawnOwner());
	if (IsValid(ClimbingSystemCharacter))
	{
		CustomMovementComponent = ClimbingSystemCharacter->GetCustomMovementComponent();
	}
}

void UCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(ClimbingSystemCharacter) || !IsValid(CustomMovementComponent))
		return;

	GetGroundSpeed();
	GetAirSpeed();
	GetShouldMove();
	GetIsFalling();
}

void UCharacterAnimInstance::GetGroundSpeed()
{
	GroundSpeed = UKismetMathLibrary::VSizeXY(ClimbingSystemCharacter->GetVelocity());
}

void UCharacterAnimInstance::GetAirSpeed()
{
	AirSpeed = ClimbingSystemCharacter->GetVelocity().Z;
}

void UCharacterAnimInstance::GetShouldMove()
{
	bShouldMove = CustomMovementComponent->GetCurrentAcceleration().Size() > 0 && 
				  GroundSpeed > 5.f && 
				  !bIsFalling;
}

void UCharacterAnimInstance::GetIsFalling()
{
	bIsFalling = CustomMovementComponent->IsFalling();
}
