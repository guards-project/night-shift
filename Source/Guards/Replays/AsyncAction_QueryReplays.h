#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "NetworkReplayStreaming.h"

#include "Subsystems/GuardsReplaySubsystem.h"

#include "AsyncAction_QueryReplays.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQueryReplayAsyncDelegate, const UGuardsReplayList*, Results);

/**
 * Get a list of Replay files available for viewing
 */
UCLASS()
class GUARDS_API UAsyncAction_QueryReplays : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	// Watches for team changes in the specified player controller
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Guards|Replays"))
	static UAsyncAction_QueryReplays* QueryReplays();

	virtual void Activate() override;

	// Called when the replay query completes
	UPROPERTY(BlueprintAssignable)
	FQueryReplayAsyncDelegate QueryComplete;

private:
	void OnEnumerateStreamsComplete(const FEnumerateStreamsResult& Result);

	UPROPERTY()
	UGuardsReplayList* ReplayList;

	TSharedPtr<INetworkReplayStreamer> ReplayStreamer;
};