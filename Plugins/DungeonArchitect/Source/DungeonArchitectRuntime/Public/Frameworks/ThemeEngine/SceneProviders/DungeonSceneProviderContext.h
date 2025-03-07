//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

struct FRandomStream;
class UDungeonSpawnLogic;

struct DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderContext {
    FDungeonSceneProviderContext()
        : Transform(FTransform::Identity)
    {
        
    }

    FTransform Transform;
    FName NodeId;
    const FRandomStream* RandomStream{};
    TArray<UDungeonSpawnLogic*> SpawnLogics;
    TSharedPtr<class IDungeonMarkerUserData> MarkerUserData;
};

