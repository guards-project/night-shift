//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonModel.h"
#include "Frameworks/Snap/Lib/Serialization/SnapConnectionSerialization.h"
#include "Frameworks/Snap/Lib/Serialization/SnapModuleInstanceSerialization.h"
#include "SnapDungeonModelBase.generated.h"

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API USnapDungeonModelBase : public UDungeonModel {
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<FSnapConnectionInstance> Connections;

	UPROPERTY()
	TArray<FSnapWallInstance> Walls;
    
	UPROPERTY()
	TArray<FSnapModuleInstanceSerializedData> ModuleInstances;
};

