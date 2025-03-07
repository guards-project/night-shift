#include "GAS/GuardsAbilityFunctionLibrary.h"

#include "GameplayCueManager.h"
#include "AbilitySystemGlobals.h"

void UGuardsAbilityFunctionLibrary::ExecuteLocalGameplayCueOnActor(AActor* Target, const FGameplayTag GameplayCueTag, const FGameplayCueParameters& Parameters)
{
	//UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	UGameplayCueManager::ExecuteGameplayCue_NonReplicated(Target, GameplayCueTag, Parameters);
}
