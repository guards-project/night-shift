#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"

#include "GuardsContextEffectInterface.generated.h"

/**
 * Listens to Context Effect events, such as animation notifies.
 */
UINTERFACE(Blueprintable)
class GUARDS_API UGuardsContextEffectInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Listens to Context Effect events, such as animation notifies.
 */
class GUARDS_API IGuardsContextEffectInterface : public IInterface
{
	GENERATED_BODY()

public:
	/** Handle an animation notify */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void AnimMotionEffect(const FName Bone, const FGameplayTag MotionEffect, USceneComponent* StaticMeshComponent,
		const UAnimSequenceBase* AnimationSequence, const bool bHitSuccess, const FHitResult HitResult, FGameplayTagContainer Contexts);
};
