#include "Replays/AsyncAction_QueryReplays.h"

UAsyncAction_QueryReplays* UAsyncAction_QueryReplays::QueryReplays()
{
	auto Action = NewObject<UAsyncAction_QueryReplays>();
	return Action;
}

void UAsyncAction_QueryReplays::Activate()
{
	ReplayStreamer = FNetworkReplayStreaming::Get().GetFactory().CreateReplayStreamer();
	ReplayList = NewObject<UGuardsReplayList>();
	if (ReplayStreamer.IsValid())
	{
		FNetworkReplayVersion EnumerateStreamsVersion = FNetworkVersion::GetReplayVersion();
		ReplayStreamer->EnumerateStreams(EnumerateStreamsVersion, INDEX_NONE, FString(), TArray<FString>(), FEnumerateStreamsCallback::CreateUObject(this, &ThisClass::OnEnumerateStreamsComplete));
	}
	else //failure
	{
		QueryComplete.Broadcast(ReplayList);
	}
}

void UAsyncAction_QueryReplays::OnEnumerateStreamsComplete(const FEnumerateStreamsResult& Result)
{
	for (const auto& Stream : Result.FoundStreams)
	{
		auto NewReplayEntry = NewObject<UGuardsReplayInfo>(ReplayList);
		NewReplayEntry->StreamInfo = Stream;
		ReplayList->Results.Add(NewReplayEntry);
	}

	QueryComplete.Broadcast(ReplayList);
}
