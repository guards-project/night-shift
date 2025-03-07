#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GuardsAbilityFunctionLibrary.generated.h"

struct FHitResult;

/**
 * Utility functions for working with gameplay abilities
 */
UCLASS()
class GUARDS_API UGuardsAbilityFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Execute a gameplay cue locally only, not replicated
	 */
	UFUNCTION(BlueprintCallable, Category = "GameplayCue", Meta = (GameplayTagFilter = "GameplayCue"), DisplayName = "Execute Local Only GameplayCue On Actor")
	static void ExecuteLocalGameplayCueOnActor(AActor* Target, const FGameplayTag GameplayCueTag, const FGameplayCueParameters& Parameters);
};