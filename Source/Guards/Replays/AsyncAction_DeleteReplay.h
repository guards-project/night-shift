#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "NetworkReplayStreaming.h"

#include "AsyncAction_DeleteReplay.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDeleteReplayAsyncDelegate, FString, ResultMessage);

/**
 * Delete a replay by name
 */
UCLASS()
class GUARDS_API UAsyncAction_DeleteReplay : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	// Watches for team changes in the specified player controller
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "Guards|Replays"))
	static UAsyncAction_DeleteReplay* DeleteReplay(const FString& FriendlyName);

	virtual void Activate() override;

	// Called when the deletion completes or fails
	UPROPERTY(BlueprintAssignable)
	FDeleteReplayAsyncDelegate Completed;

private:
	void OnDeleteReplayComplete(const FDeleteFinishedStreamResult& Result);

	UPROPERTY()
	FString ReplayName;

	TSharedPtr<INetworkReplayStreamer> ReplayStreamer;
};