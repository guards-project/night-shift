//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonConfig.h"
#include "SnapMapDungeonConfig.generated.h"

class USnapMapModuleDatabase;
class ASnapMapMinimapActor;

DECLARE_LOG_CATEGORY_EXTERN(SnapMapDungeonConfigLog, Log, All);

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API USnapMapDungeonConfig : public UDungeonConfig {
    GENERATED_UCLASS_BODY()

public:
    /** A module database asset that contains the references of the modules to use for building the dungeon */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    USnapMapModuleDatabase* ModuleDatabase;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    class USnapMapAsset* DungeonFlowGraph;

    /**
     When modules are stitched together, the builder makes sure they do not overlap.  This parameter is used to 
     control the tolerance level.  If set to 0, even the slightest overlap with a nearby module would not create an adjacent module
     Leaving to a small number like 100, would tolerate an overlap with nearby module by 100 unreal units.
     Adjust this depending on your art asset
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    int32 CollisionTestContraction;

    /** The max processing time (in seconds) before bailing out. This keeps the build function from hanging if it doesn't find a solution */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    float MaxProcessingTimeSecs = 4.0f;

    /** If the dungeon build fails, it will be retried with another seed multiple times (see field NumBuildRetries) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    bool bSupportBuildRetries = false;

    /** If the dungeon build fails, it will be retried with another seed multiple tiles based on this count */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon, Meta = (EditCondition = "bSupportBuildRetries"))
    int32 NumBuildRetries = 20;

	/**
	* Build the entire dungeon in the main level by copying over the module actors to the persistent level. This will disable level streaming
	* and is meant to be used in the editor only.  You may bake your lightmaps with this method
	**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced)
	bool bGenerateSinglePersistentDungeon = false;

};

