#include "Effects/GuardsContextEffectComponent.h"

UGuardsContextEffectComponent::UGuardsContextEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
}

void UGuardsContextEffectComponent::AnimMotionEffect_Implementation(const FName Bone,
	const FGameplayTag MotionEffect, USceneComponent* StaticMeshComponent,
	const UAnimSequenceBase* AnimationSequence, const bool bHitSuccess,
	const FHitResult HitResult, FGameplayTagContainer Contexts)
{
}

void UGuardsContextEffectComponent::BeginPlay()
{
	// Load effects
	Super::BeginPlay();
}

void UGuardsContextEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unload effects
	Super::EndPlay(EndPlayReason);
}
