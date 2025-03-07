#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"

#include "GuardsCharacterMovementComponent.generated.h"

UCLASS(Config = Game)
class GUARDS_API UGuardsCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UGuardsCharacterMovementComponent();

	virtual float GetMaxSpeed() const override;

	virtual void Crouch(bool bClientSimulation /*= false*/) override;
	virtual void UnCrouch(bool bClientSimulation /*= false*/) override;
	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode /*= 0*/) override;

private:
	FGameplayTag CrouchingTag;
	FGameplayTag FallingTag;
};
