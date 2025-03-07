#include "Effects/AnimNotify_GuardsContextEffect.h"

#include "GuardsLogChannels.h"
#include "Effects/GuardsContextEffectComponent.h"

#include "CollisionQueryParams.h"

FString UAnimNotify_GuardsContextEffect::GetNotifyName_Implementation() const
{
	// If the Effect Tag is valid, pass the string name to the notify name
	if (Effect.IsValid())
	{
		return Effect.ToString();
	}

	return Super::GetNotifyName_Implementation();
}

void UAnimNotify_GuardsContextEffect::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp)
	{
		if (AActor* OwningActor = MeshComp->GetOwner())
		{
			bool bHitSuccess = false;
			FHitResult HitResult;
			FCollisionQueryParams QueryParams;

			QueryParams.AddIgnoredActor(OwningActor);
			QueryParams.bReturnPhysicalMaterial = true;

			// if (bPerformTrace)
			{
				// If trace is needed, set up Start Location to Attached
				const FVector TraceStart = SocketName.IsNone() ? MeshComp->GetComponentLocation() : MeshComp->GetSocketLocation(SocketName);
				const auto TraceChannel = ECollisionChannel::ECC_Visibility;

				// Make sure World is valid and trace
				if (UWorld* World = OwningActor->GetWorld())
				{
					// Call Line Trace, Pass in relevant properties
					bHitSuccess = World->LineTraceSingleByChannel(HitResult, TraceStart, (TraceStart + EndTraceLocationOffset),
						TraceChannel, QueryParams, FCollisionResponseParams::DefaultResponseParam);
				}

				// Prepare Contexts in advance
				FGameplayTagContainer Contexts;

				TArray<UObject*> EffectImplementingObjects;

				// Collect listeners
				// First, owning actor
				if (OwningActor->Implements<UGuardsContextEffectInterface>())
				{
					EffectImplementingObjects.Add(OwningActor);
				}

				// Then all components if they implement the interface
				for (const auto Component : OwningActor->GetComponents())
				{
					if (Component)
					{
						if (Component->Implements<UGuardsContextEffectInterface>())
						{
							EffectImplementingObjects.Add(Component);
						}
					}
				}

				// Notify listeners
				for (auto ImplementingObject : EffectImplementingObjects)
				{
					if (ImplementingObject)
					{
						IGuardsContextEffectInterface::Execute_AnimMotionEffect(
							ImplementingObject,
							SocketName, Effect, MeshComp,
							Animation, bHitSuccess, HitResult, Contexts);
					}
				}
			}
		}
	}
}
