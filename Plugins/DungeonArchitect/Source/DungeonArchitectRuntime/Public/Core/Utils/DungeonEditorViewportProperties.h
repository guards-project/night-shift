//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonConfig.h"

#include "Templates/SubclassOf.h"
#include "DungeonEditorViewportProperties.generated.h"

class UDungeonMarkerEmitter;
class UProceduralMarkerEmitter;

class FDungeonEditorViewportPropertiesListener {
public:
    virtual void OnPropertyChanged(FString PropertyName, class UDungeonEditorViewportProperties* Properties) = 0;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonEditorViewportProperties : public UObject {
    GENERATED_BODY()

public:
    UDungeonEditorViewportProperties(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(BlueprintReadWrite, Category = Config)
    UDungeonConfig* DungeonConfig;

    /** Lets you swap out the default dungeon builder with your own implementation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder")
    TSubclassOf<class UDungeonBuilder> BuilderClass;

    /** Lets you emit your own markers into the scene */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category="Marker Emitters")
    TArray<UDungeonMarkerEmitter*> MarkerEmitters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category="Marker Emitters")
    TArray<UProceduralMarkerEmitter*> ProceduralMarkerEmitters;
    
#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
    void PostEditChangeConfigProperty(struct FPropertyChangedEvent& e);
#endif // WITH_EDITOR

    TWeakPtr<FDungeonEditorViewportPropertiesListener> PropertyChangeListener;
};

