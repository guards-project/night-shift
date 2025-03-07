//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/CellFlow/CellFlowConfig.h"

#include "Materials/MaterialInterface.h"

FCellFlowMeshProfile::FCellFlowMeshProfile() {
	if (!IsRunningCommandlet()) {
		static const TCHAR* DefaultMaterialName = TEXT("/DungeonArchitect/Core/Editors/FlowGraph/CellFlow/M_CellFlow_Voronoi_Inst");
		// Structure to hold one-time initialization
		struct FConstructorStatics {
			ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Material;

			FConstructorStatics() :
				Material(DefaultMaterialName)
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		Material = ConstructorStatics.Material.Get();
	}
}

