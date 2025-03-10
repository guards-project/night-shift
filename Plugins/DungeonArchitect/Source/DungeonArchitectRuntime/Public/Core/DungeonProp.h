//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonSpatialConstraint.h"
#include "Frameworks/ThemeEngine/Rules/Selector/DungeonSelectorLogic.h"
#include "Frameworks/ThemeEngine/Rules/Spawn/DungeonSpawnLogic.h"
#include "Frameworks/ThemeEngine/Rules/Transformer/DungeonTransformLogic.h"
#include "DungeonProp.generated.h"

class UProceduralDungeonTransformLogic;

USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FDAMarkerInfo {
    GENERATED_USTRUCT_BODY()

    FDAMarkerInfo() : Id(-1) {
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonMarker)
    int32 Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonMarker)
    FString MarkerName;

    /** The name of the clustered theme to override on this marker */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonMarker)
    FString ClusterThemeOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonMarker)
    FTransform Transform;

    TSharedPtr<class IDungeonMarkerUserData> UserData;

    FORCEINLINE bool operator==(const FDAMarkerInfo& other) const {
        return other.Id == Id;
    }
};

/** Props can emit new sockets when they are inserted, to add more child props relative to them */
USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FPropChildSocketData {
    GENERATED_USTRUCT_BODY()

    FPropChildSocketData() : Offset(FTransform::Identity) {
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    FString SocketType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    FTransform Offset;
};

USTRUCT(Blueprintable)
struct DUNGEONARCHITECTRUNTIME_API FPropTypeData {
    GENERATED_USTRUCT_BODY()

    FPropTypeData()
        : AssetObject(nullptr)
          , AttachToSocket(TEXT("None"))
          , Probability(1)
          , bUseSelectionLogic(false)
          , bLogicOverridesAffinity(true)
          , bUseTransformLogic(false)
          , bUseSpawnLogic(false)
          , ConsumeOnAttach(false)
          , Offset(FTransform::Identity)
          , bUseSpatialConstraint(false)
          , SpatialConstraint(nullptr) {
    }

    UPROPERTY()
    FName NodeId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    UObject* AssetObject;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    FString AttachToSocket;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    float Probability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    bool bUseSelectionLogic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    bool bLogicOverridesAffinity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    TArray<UDungeonSelectorLogic*> SelectionLogics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    bool bUseTransformLogic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    TArray<UDungeonTransformLogic*> TransformLogics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    bool bUseProceduralTransformLogic{};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    TArray<UProceduralDungeonTransformLogic*> ProceduralTransformLogics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    bool bUseSpawnLogic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    TArray<UDungeonSpawnLogic*> SpawnLogics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    bool ConsumeOnAttach;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    FTransform Offset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    bool bUseSpatialConstraint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    UDungeonSpatialConstraint* SpatialConstraint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DungeonProp)
    TArray<FPropChildSocketData> ChildSockets;
};

