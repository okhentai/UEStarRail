// Fill out your copyright notice in the Description page of Project Settings.


#include "StarRailAnimInstance.h"
#include "StarRailCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "UEStarRail/Weapon/Weapon.h"
#include "UEStarRail/StarRailTypes/CombatState.h"

void UStarRailAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	StarRailCharacter = Cast<AStarRailCharacter>(TryGetPawnOwner());
}

void UStarRailAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (StarRailCharacter == nullptr)
	{
		StarRailCharacter = Cast<AStarRailCharacter>(TryGetPawnOwner());
	}
	if (StarRailCharacter == nullptr) return;

	FVector Velocity = StarRailCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = StarRailCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = StarRailCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = StarRailCharacter->IsWeaponEquipped();
	EquippedWeapon = StarRailCharacter->GetEquippedWeapon();
	bIsCrouched = StarRailCharacter->bIsCrouched;
	bAiming = StarRailCharacter->IsAiming();
	TurningInPlace = StarRailCharacter->GetTurningInPlace();
	bRotateRootBone = StarRailCharacter->ShouldRotateRootBone();
	bElimmed = StarRailCharacter->IsElimmed();

	// Offset Yaw for Strafing
	FRotator AimRotation = StarRailCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(StarRailCharacter->GetVelocity());
	FRotator DeltaRor = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRor, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = StarRailCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = StarRailCharacter->GetAO_Yaw();
	AO_Pitch = StarRailCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && StarRailCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		StarRailCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (StarRailCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
		
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - StarRailCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
	}
	bUseFABRIK = StarRailCharacter->GetCombatState() != ECombatState::ECS_Reloading;
	bUseAimOffsets = StarRailCharacter->GetCombatState() != ECombatState::ECS_Reloading && !StarRailCharacter->GetDisableGameplay();
	bTransformRightHand = StarRailCharacter->GetCombatState() != ECombatState::ECS_Reloading && !StarRailCharacter->GetDisableGameplay();
}
