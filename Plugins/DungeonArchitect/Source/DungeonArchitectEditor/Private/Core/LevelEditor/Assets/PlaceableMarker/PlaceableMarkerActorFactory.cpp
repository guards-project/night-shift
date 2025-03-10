//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Assets/PlaceableMarker/PlaceableMarkerActorFactory.h"

#include "Frameworks/ThemeEngine/Markers/PlaceableMarker.h"

#include "AssetRegistry/AssetData.h"

UPlaceableMarkerActorFactory::UPlaceableMarkerActorFactory(const FObjectInitializer& ObjectInitializer) : Super(
    ObjectInitializer) {
    DisplayName = NSLOCTEXT("PlaceableMarker", "PlaceableMarkerFactoryDisplayName", "Add Placeable Marker");
    NewActorClass = APlaceableMarkerActor::StaticClass();
}

UObject* UPlaceableMarkerActorFactory::GetAssetFromActorInstance(AActor* ActorInstance) {
    APlaceableMarkerActor* MarkerActor = Cast<APlaceableMarkerActor>(ActorInstance);
    return MarkerActor ? MarkerActor->PlaceableMarkerComponent->MarkerAsset : nullptr;
}

AActor* UPlaceableMarkerActorFactory::SpawnActor(UObject* InAsset, ULevel* InLevel, const FTransform& InTransform, const FActorSpawnParameters& InSpawnParams) {
    AActor* Actor = UActorFactory::SpawnActor(InAsset, InLevel, InTransform, InSpawnParams);
    if (const APlaceableMarkerActor* MarkerActor = Cast<APlaceableMarkerActor>(Actor)) {
        MarkerActor->PlaceableMarkerComponent->MarkerAsset = Cast<UPlaceableMarkerAsset>(InAsset);
    }
    return Actor;
}

void UPlaceableMarkerActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor) {
    APlaceableMarkerActor* MarkerActor = Cast<APlaceableMarkerActor>(NewActor);
    if (MarkerActor && MarkerActor->PlaceableMarkerComponent) {
        MarkerActor->PlaceableMarkerComponent->MarkerAsset = Cast<UPlaceableMarkerAsset>(Asset);
    }
}

void UPlaceableMarkerActorFactory::PostCreateBlueprint(UObject* Asset, AActor* CDO) {
    APlaceableMarkerActor* MarkerActor = Cast<APlaceableMarkerActor>(CDO);
    if (MarkerActor && MarkerActor->PlaceableMarkerComponent) {
        MarkerActor->PlaceableMarkerComponent->MarkerAsset = Cast<UPlaceableMarkerAsset>(Asset);
    }
}

bool UPlaceableMarkerActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) {
    if (AssetData.IsValid() && AssetData.GetClass()->IsChildOf(UPlaceableMarkerAsset::StaticClass())) {
        return true;
    }
    OutErrorMsg = NSLOCTEXT("PlaceableMarker", "PlaceableMarkerFactoryDisplayName", "No marker asset specified.");
    return false;
}

