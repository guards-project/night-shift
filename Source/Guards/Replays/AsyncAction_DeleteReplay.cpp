#include "Replays/AsyncAction_DeleteReplay.h"
#include "AsyncAction_DeleteReplay.h"

UAsyncAction_DeleteReplay* UAsyncAction_DeleteReplay::DeleteReplay(const FString& FriendlyName)
{
	auto Action = NewObject<UAsyncAction_DeleteReplay>();
	Action->ReplayName = FriendlyName;
	return Action;
}

void UAsyncAction_DeleteReplay::Activate()
{
	if (!ReplayName.IsEmpty())
	{
		ReplayStreamer = FNetworkReplayStreaming::Get().GetFactory().CreateReplayStreamer();
		ReplayStreamer->DeleteFinishedStream(ReplayName, FDeleteFinishedStreamCallback::CreateUObject(this, &ThisClass::OnDeleteReplayComplete));
	}
	else
	{
		Completed.Broadcast(TEXT("Failed to delete, no name supplied"));
	}
}

void UAsyncAction_DeleteReplay::OnDeleteReplayComplete(const FDeleteFinishedStreamResult& Result)
{
	Completed.Broadcast(Result.WasSuccessful() ? TEXT("Replay deleted") : TEXT("Failed to delete replay"));
}
