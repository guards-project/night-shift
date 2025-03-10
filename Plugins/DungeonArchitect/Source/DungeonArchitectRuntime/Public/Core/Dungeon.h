//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonBuilder.h"
#include "Core/DungeonConfig.h"
#include "Core/DungeonModel.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamer.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamingModel.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"
#include "Frameworks/ThemeEngine/Markers/DungeonMarkerEmitter.h"

#include "Async/AsyncWork.h"
#include "GameFramework/Actor.h"
#include "Dungeon.generated.h"

class ADungeon;
class UBillboardComponent;
class UDungeonModel;
class UDungeonQuery;
class UDungeonConfig;
class UDungeonBuilder;
class UDungeonToolData;
class UDungeonEventListener;
class UProceduralMarkerEmitter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDungeonBuildCompleteBindableEvent, ADungeon*, Dungeon, bool, bSuccess);


DECLARE_LOG_CATEGORY_EXTERN(DungeonLog, Log, All);

class DUNGEONARCHITECTRUNTIME_API FDungeonBuilderTask : public FNonAbandonableTask {
public:
    friend class FAutoDeleteAsyncTask<FDungeonBuilderTask>;
    FDungeonBuilderTask(UWorld* pWorld, ADungeon* pDungeon);

    void DoWork();

    void RunGameThreadCommands();

    static const TCHAR* Name() {
        return TEXT("DungeonBuilderTask");
    }

    FORCEINLINE TStatId GetStatId() const {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FDungeonBuilderTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    bool IsRunningGameThreadCommands() const { return bRunningGameThreadCommands; }

private:
    ADungeon* Dungeon;
    UWorld* World;
    TSharedPtr<class FDungeonSceneProvider> SceneProvider;

    bool bRunningGameThreadCommands;
};


/**
* The main dungeon actor responsible for creating a dungeon.  
* Drop this actor into your scene, assign a theme and click "Build Dungeon" button
* to create your dungeon.  
* From code, call ADungeon::BuildDungeon after adding an entry into the ADungeon::Themes array
*/
UCLASS(NotBlueprintable, hidecategories = (Rendering,Input,Actor,Misc,Replication,Collision,LOD,Cooking,HLOD,Physics,Networking))
class DUNGEONARCHITECTRUNTIME_API ADungeon : public AActor {
    GENERATED_BODY()

public:
    ADungeon(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    void BuildDungeon();

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    void DestroyDungeon();

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    UDungeonBuilder* SetBuilderClass(TSubclassOf<UDungeonBuilder> InBuilderClass);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Dungeon)
    UDungeonQuery* GetQuery();

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    void RandomizeSeed();
    
    /**
     * Resets the dungeon id.  Do this if you have another copy of the dungeon somehow (e.g. streaming in the same level multiple times on the scene)
     * NOTE: This will unlink all the existing dungeon spawned items and they will not clean up. Make sure the existing dungeon is destroyed first
     */
    UFUNCTION(BlueprintCallable, Category=Advanced)
    void ResetDungeonId();
    
    virtual void Destroyed() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    virtual bool IsLevelBoundsRelevant() const override { return false; }

    void CreateBuilderInstance();
    void CreateBuilderInstance(const FObjectInitializer& ObjectInitializer);

    virtual UDungeonToolData* GetToolData() const { return ToolData; }
    virtual UDungeonConfig* GetConfig() const { return Config; }
    virtual UDungeonModel* GetModel() const { return DungeonModel; }
    virtual UDungeonBuilder* GetBuilder() const { return Builder; }

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
#endif

#if WITH_EDITORONLY_DATA
    UBillboardComponent* GetSpriteComponent() const;
#endif //WITH_EDITORONLY_DATA
    
private:
#if WITH_EDITORONLY_DATA
    UPROPERTY()
    UBillboardComponent* SpriteComponent;
#endif //WITH_EDITORONLY_DATA

private:
    void PostDungeonBuild();
    void InitializeQueryObject();
    void ProcessLevelStreaming() const;
    void AssignNewDungeonId();

public:
    UPROPERTY()
    FGuid Uid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TArray<UDungeonThemeAsset*> Themes;

    /** Lets you swap out the default dungeon builder with your own implementation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TSubclassOf<UDungeonBuilder> BuilderClass;

    /** Lets you emit your own markers into the scene using custom blueprints */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category="Marker Emitters", DisplayName="Marker Emitter Blueprints")
    TArray<UDungeonMarkerEmitter*> MarkerEmitters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category="Marker Emitters")
    TArray<UProceduralMarkerEmitter*> ProceduralMarkerEmitters;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category = Advanced)
    TArray<UDungeonEventListener*> EventListeners;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced)
    TArray<FClusterThemeInfo> ClusterThemes;

    UPROPERTY(EditAnywhere, Category = Advanced)
    bool bDrawDebugData;

    UPROPERTY(EditAnywhere, Category = Advanced)
    bool bUseCustomItemFolderName{};
    
    UPROPERTY(EditAnywhere, Category = Advanced, meta=(EditCondition="bUseCustomItemFolderName"))
    FString CustomItemFolderName{};
    
    UPROPERTY(BlueprintReadOnly, Category = Misc)
    UDungeonBuilder* Builder;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Misc)
    UDungeonConfig* Config;

    UPROPERTY(BlueprintReadOnly, Category = Misc)
    UDungeonModel* DungeonModel;

    UPROPERTY()
    UDungeonToolData* ToolData;

    UPROPERTY()
    UDungeonQuery* Query;

    UPROPERTY()
    FDungeonLevelStreamingConfig LevelStreamingConfig;

    UPROPERTY(BlueprintReadOnly, Category = Dungeon)
    UDungeonLevelStreamingModel* LevelStreamingModel;

    /** When the dungeon is built asynchronously over multiple frames, objects closer to this point are built first */
    UPROPERTY(BlueprintReadWrite, Category = Dungeon)
    FVector BuildPriorityLocation;

    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDungeonBuildCompleteBindableEvent OnDungeonBuildComplete;

    typedef TSharedPtr<FAsyncTask<FDungeonBuilderTask>> FDungeonBuilderAsyncTaskPtr;
    typedef TArray<FDungeonBuilderAsyncTaskPtr> FDungeonBuilderAsyncTaskArray;

    // The active build task
    FDungeonBuilderAsyncTaskPtr BuildTask;

    // The next build task to run
    FDungeonBuilderAsyncTaskPtr QueuedBuildTask;

    // Flag to indicate if the dungeon build has been initiated in a separate thread
    bool bBuildInProgress;

    /** The folder under which the spawned actors should be placed in */
    FName ItemFolderPath;
};

