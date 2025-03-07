#pragma once

#include "CoreMinimal.h"
#include "NetworkReplayStreaming.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "GuardsReplaySubsystem.generated.h"

class UDemoNetDriver;

// Saved replay with metadata
UCLASS(BlueprintType)
class UGuardsReplayInfo : public UObject
{
	GENERATED_BODY()

public:
	FNetworkReplayStreamInfo StreamInfo;

	/** The display name of the replay */
	UFUNCTION(BlueprintPure, Category = "Guards|Replays")
	FString GetFriendlyName() const { return StreamInfo.FriendlyName; }

	/** The unique name of the replay (filename) */
	UFUNCTION(BlueprintPure, Category = "Guards|Replays")
	FString GetUniqueName() const { return StreamInfo.Name; }

	/** The date and time the replay was recorded */
	UFUNCTION(BlueprintPure, Category = "Guards|Replays")
	FDateTime GetTimestamp() const { return StreamInfo.Timestamp; }

	/** The duration of the replay */
	UFUNCTION(BlueprintPure, Category = "Guards|Replays")
	FTimespan GetDuration() const { return FTimespan::FromMilliseconds(StreamInfo.LengthInMS); }
};

// Holds a list of Replay Infos
UCLASS(BlueprintType)
class UGuardsReplayList : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Guards|Replays")
	TArray<UGuardsReplayInfo*> Results;
};

UCLASS()
class GUARDS_API UGuardsReplaySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Guards|Replays")
	bool IsRecording() const;

	UFUNCTION(BlueprintPure, Category = "Guards|Replays")
	bool IsPlaying() const;

	/** Start recording replay to a file */
	UFUNCTION(BlueprintCallable, Category = "Guards|Replays")
	void RecordReplay(const FString& Filename);

	/** Stop the current recording */
	UFUNCTION(BlueprintCallable, Category = "Guards|Replays")
	void StopRecording();

	/** Play a replay by filename */
	UFUNCTION(BlueprintCallable, Category = "Guards|Replays")
	void PlayReplay(const FString& Filename);

	/** Get the name of the current (or last) recording or playing replay */
	UFUNCTION(BlueprintPure, Category = "Guards|Replays")
	FString GetCurrentReplayName() const;

	/** Get the length of the current replay, in seconds */
	UFUNCTION(BlueprintPure, Category = "Guards|Replays")
	float GetReplayLength() const;

	/** Get the timestamp of the current replay playback, in seconds */
	UFUNCTION(BlueprintPure, Category = "Guards|Replays")
	float GetCurrentTime() const;

	/** Jump to a timestamp in the active replay, in seconds */
	UFUNCTION(BlueprintCallable, Category = "Guards|Replays")
	void GotoTime(float TimeInSeconds);

	/** Get the active time dilation for the active replay */
	UFUNCTION(BlueprintPure, Category = "Guards|Replays")
	float GetPlaybackSpeed() const;

	/** Set the time dilation for the active replay */
	UFUNCTION(BlueprintCallable, Category = "Guards|Replays")
	void SetPlaybackSpeed(float Speed) const;

	/** Utility function to open the saved/Demos folder in file explorer */
	UFUNCTION(BlueprintCallable, Category = "Guards|Replays")
	void OpenReplayFolder() const;

	/** Returns true if the demo playback is paused for the current World */
	UFUNCTION(BlueprintPure, Category = "Guards|Replays")
	bool IsPlaybackPaused() const;

	UFUNCTION(BlueprintCallable, Category = "Guards|Replays")
	void SetPlaybackPaused(bool Paused);

private:
	UDemoNetDriver* GetDemoDriver() const;

	//Last recorded or played replay name
	FString LastReplay;
};