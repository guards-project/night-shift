#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "GuardsContextEffectInterface.h"
#include "GuardsContextEffectComponent.generated.h"

/**
 * Listens to context effects notifies and plays the effects
 */
UCLASS(ClassGroup = (Custom), hidecategories = (Variable, Tags, ComponentTick, ComponentReplication, Activation, Cooking, AssetUserData, Collision), CollapseCategories, meta = (BlueprintSpawnableComponent))
class GUARDS_API UGuardsContextEffectComponent : public UActorComponent, public IGuardsContextEffectInterface
{
	GENERATED_BODY()

public:
	UGuardsContextEffectComponent();

	UFUNCTION(BlueprintCallable)
	virtual void AnimMotionEffect_Implementation(const FName Bone, const FGameplayTag MotionEffect, USceneComponent* StaticMeshComponent,
		const UAnimSequenceBase* AnimationSequence, const bool bHitSuccess, const FHitResult HitResult, FGameplayTagContainer Contexts) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};