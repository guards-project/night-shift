//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "SnapMapAssetFactory.generated.h"

UCLASS()
class DUNGEONARCHITECTEDITOR_API USnapMapAssetFactory : public UFactory {
    GENERATED_UCLASS_BODY()

    // UFactory interface
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context,
                                      FFeedbackContext* Warn) override;
    virtual bool CanCreateNew() const override;
    // End of UFactory interface
};

