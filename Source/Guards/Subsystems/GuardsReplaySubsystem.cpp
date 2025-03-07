#include "GuardsReplaySubsystem.h"

#include "Engine/DemoNetDriver.h"
#include "Engine/World.h"

bool UGuardsReplaySubsystem::IsRecording() const
{
	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		return DemoDriver->IsRecording();
	}
	return false;
}

bool UGuardsReplaySubsystem::IsPlaying() const
{
	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		return DemoDriver->IsPlaying();
	}
	return false;
}

void UGuardsReplaySubsystem::RecordReplay(const FString& Filename)
{
	// Note: we could pass the description into the friendly name
	LastReplay = Filename;
	GetGameInstance()->StartRecordingReplay(Filename, Filename);
}

void UGuardsReplaySubsystem::StopRecording()
{
	GetGameInstance()->StopRecordingReplay();
}

void UGuardsReplaySubsystem::PlayReplay(const FString& Filename)
{
	LastReplay = Filename;
	GetGameInstance()->PlayReplay(Filename);
}

FString UGuardsReplaySubsystem::GetCurrentReplayName() const
{
	return LastReplay;
}

float UGuardsReplaySubsystem::GetReplayLength() const
{
	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		return DemoDriver->GetDemoTotalTime();
	}
	return 0.0f;
}

float UGuardsReplaySubsystem::GetCurrentTime() const
{
	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		return DemoDriver->GetDemoCurrentTime();
	}
	return 0.0f;
}

void UGuardsReplaySubsystem::GotoTime(float TimeInSeconds)
{
	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		TimeInSeconds = FMath::Max(TimeInSeconds, 0.f);
		DemoDriver->GotoTimeInSeconds(TimeInSeconds);
	}
}

float UGuardsReplaySubsystem::GetPlaybackSpeed() const
{
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		if (AWorldSettings* WorldSettings = World->GetWorldSettings(false, false))
		{
			return WorldSettings->DemoPlayTimeDilation;
		}
	}
	return 0.0f;
}

void UGuardsReplaySubsystem::SetPlaybackSpeed(float Speed) const
{
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		if (AWorldSettings* WorldSettings = World->GetWorldSettings(false, false))
		{
			Speed = FMath::Clamp(Speed, 0.f, 100.f);
			WorldSettings->DemoPlayTimeDilation = Speed;
		}
	}
}

void UGuardsReplaySubsystem::OpenReplayFolder() const
{
	auto ReplayStreamer = FNetworkReplayStreaming::Get().GetFactory().CreateReplayStreamer();
	if (ReplayStreamer.IsValid())
	{
		FString DemoPath;
		if (ReplayStreamer->GetDemoPath(DemoPath) == EStreamingOperationResult::Success)
		{
			DemoPath = FPaths::ConvertRelativePathToFull(DemoPath);
			DemoPath = FPaths::Combine(TEXT("file:///"), DemoPath);
			FPlatformProcess::LaunchURL(*DemoPath, nullptr, nullptr);
		}
	}
}

bool UGuardsReplaySubsystem::IsPlaybackPaused() const
{
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		if (AWorldSettings* WorldSettings = World->GetWorldSettings(false, false))
		{
			return WorldSettings->GetPauserPlayerState() != nullptr;
		}
	}
	return false;
}

void UGuardsReplaySubsystem::SetPlaybackPaused(bool Paused)
{
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		if (Paused)
		{
			auto DemoNetDriver = GetDemoDriver();
			if (DemoNetDriver != nullptr && DemoNetDriver->ServerConnection != nullptr && DemoNetDriver->ServerConnection->OwningActor != nullptr)
			{
				APlayerController* PlayerController = Cast<APlayerController>(DemoNetDriver->ServerConnection->OwningActor);
				if (PlayerController != nullptr)
				{
					if (AWorldSettings* WorldSettings = World->GetWorldSettings(false, false))
					{
						WorldSettings->SetPauserPlayerState(PlayerController->PlayerState);
					}
				}
			}
		}
		else
		{
			if (AWorldSettings* WorldSettings = World->GetWorldSettings(false, false))
			{
				WorldSettings->SetPauserPlayerState(nullptr);
			}
		}
	}
}

UDemoNetDriver* UGuardsReplaySubsystem::GetDemoDriver() const
{
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		return World->GetDemoNetDriver();
	}
	return nullptr;
}
