//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/FloorPlan/Volumes/FloorPlanRoomVolume.h"


AFloorPlanRoomVolume::AFloorPlanRoomVolume(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
      , bCreateWalls(true)
      , Priority(100) {

}

