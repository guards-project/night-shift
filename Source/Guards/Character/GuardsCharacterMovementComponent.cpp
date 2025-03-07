#include "GuardsCharacterMovementComponent.h"
#include "GAS/GuardsMovementAttributeSet.h"

#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UGuardsCharacterMovementComponent::UGuardsCharacterMovementComponent()
{
	CrouchingTag = FGameplayTag::RequestGameplayTag("State.Crouching");
	FallingTag = FGameplayTag::RequestGameplayTag("State.Falling");
}

float UGuardsCharacterMovementComponent::GetMaxSpeed() const
{
	float OrigSpeed = Super::GetMaxSpeed();
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		bool Found = false;
		float Multiplier = ASC->GetGameplayAttributeValue(UGuardsMovementAttributeSet::GetSpeedMultiplierAttribute(), Found);
		if (!Found)
		{
			Multiplier = 1.f;
		}
		OrigSpeed *= Multiplier;
	}
	return OrigSpeed;
}

void UGuardsCharacterMovementComponent::Crouch(bool bClientSimulation)
{
	// Super early outs with this as well
	if (!HasValidData())
	{
		return;
	}

	// Replicated loose gameplay tags are bugged in 5.0, needs 5.1
	/*if (bClientSimulation)
	{
		Super::Crouch(bClientSimulation);
	}
	else
	{
		bool bWasCrouched = CharacterOwner->bIsCrouched;
		Super::Crouch(bClientSimulation);
		// weren't crouching, now are
		if (!bWasCrouched && CharacterOwner->bIsCrouched)
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
			{
				ASC->AddReplicatedLooseGameplayTag(CrouchingTag);
			}
		}
	}*/

	// 5.0 version - may have late replication issues
	bool bWasCrouched = CharacterOwner->bIsCrouched;
	Super::Crouch(bClientSimulation);
	// weren't crouching, now are
	if (!bWasCrouched && CharacterOwner->bIsCrouched)
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
		{
			ASC->AddLooseGameplayTag(CrouchingTag);
		}
	}
}

void UGuardsCharacterMovementComponent::UnCrouch(bool bClientSimulation)
{
	// Super early outs with this as well
	if (!HasValidData())
	{
		return;
	}

	// Replicated loose gameplay tags are bugged in 5.0, needs 5.1
	/*if (bClientSimulation)
	{
		Super::UnCrouch(bClientSimulation);
	}
	else
	{
		bool bWasCrouched = CharacterOwner->bIsCrouched;
		Super::UnCrouch(bClientSimulation);
		// were crouching, no longer are
		if (bWasCrouched && !CharacterOwner->bIsCrouched)
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
			{
				ASC->RemoveReplicatedLooseGameplayTag(CrouchingTag);
			}
		}
	}*/

	bool bWasCrouched = CharacterOwner->bIsCrouched;
	Super::UnCrouch(bClientSimulation);
	// were crouching, now aren't
	if (bWasCrouched && !CharacterOwner->bIsCrouched)
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
		{
			ASC->RemoveLooseGameplayTag(CrouchingTag);
		}
	}
}

void UGuardsCharacterMovementComponent::SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode)
{
	const EMovementMode PrevMovementMode = MovementMode;
	const uint8 PrevCustomMode = CustomMovementMode;
	Super::SetMovementMode(NewMovementMode, NewCustomMode);
	if (HasValidData()) // Super also checks this
	{
		// Falling is used for jumping as well
		if (NewMovementMode == MOVE_Falling)
		{
			// now falling
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
			{
				ASC->AddLooseGameplayTag(FallingTag);
			}
		}
		else if (PrevMovementMode == MOVE_Falling)
		{
			// no longer falling
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
			{
				ASC->RemoveLooseGameplayTag(FallingTag);
			}
		}
	}
}
