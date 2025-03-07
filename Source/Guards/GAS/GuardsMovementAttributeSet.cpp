#include "GuardsMovementAttributeSet.h"
#include "Net/UnrealNetwork.h"

void UGuardsMovementAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGuardsMovementAttributeSet, SpeedMultiplier, COND_None, REPNOTIFY_Always);
}

void UGuardsMovementAttributeSet::OnRep_SpeedMultiplier(const FGameplayAttributeData& OldSpeedMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGuardsMovementAttributeSet, SpeedMultiplier, OldSpeedMultiplier);
}
