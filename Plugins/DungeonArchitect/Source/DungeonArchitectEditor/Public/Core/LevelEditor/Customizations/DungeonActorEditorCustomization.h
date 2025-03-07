//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DetailCategoryBuilder.h"
#include "IDetailCustomization.h"
#include "IPropertyChangeListener.h"
#include "DungeonActorEditorCustomization.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDungeonCustomization, Log, All);

class ADungeon;
class UDungeonBuilder;

class FDungeonBuilderClassCombo : public TSharedFromThis<FDungeonBuilderClassCombo> {
public:
    typedef TFunction<TSubclassOf<UDungeonBuilder>()> FnGetBuilder;
    typedef TFunction<void(TSubclassOf<UDungeonBuilder>)> FnSetBuilder;
    FDungeonBuilderClassCombo(const FnGetBuilder& InGetBuilder, const FnSetBuilder& InSetBuilder);
    void CreateBuilderCombo(FDetailWidgetRow& InRowWidget);
    
private:
    struct FBuilderClassEntry {
        explicit FBuilderClassEntry(const TSubclassOf<UDungeonBuilder>& InBuilderClass)
            : BuilderClass(InBuilderClass) {
        }

        TSubclassOf<UDungeonBuilder> BuilderClass;
    };

private:
    void RegenerateBuilderComboItems();
    void UseSelectedBuilderClass() const;
    void OnBrowseToSelectedBuilderClass() const;

    TSharedRef<SWidget> GenerateBuilderClassComboRowItem(TSharedPtr<FBuilderClassEntry> Item) const;

private:
    TArray<TSharedPtr<FBuilderClassEntry>> BuilderClassComboItems;
    TSharedPtr<FBuilderClassEntry> SelectedBuilderItem;
    FnGetBuilder GetBuilder;
    FnSetBuilder SetBuilder;
};

class FDungeonActorEditorCustomization : public IDetailCustomization {
public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    // End of IDetailCustomization interface

    static TSharedRef<IDetailCustomization> MakeInstance();

    static FReply RebuildDungeon(IDetailLayoutBuilder* DetailBuilder);
    static FReply DestroyDungeon(IDetailLayoutBuilder* DetailBuilder);
    static FReply RandomizeSeed(IDetailLayoutBuilder* DetailBuilder);
    static FReply OpenHelpSystem(IDetailLayoutBuilder* DetailBuilder);

    static FReply LaunchSupportForum(IDetailLayoutBuilder* DetailBuilder);
    static FReply LaunchDiscord(IDetailLayoutBuilder* DetailBuilder);
    static FReply LaunchDocumentation(IDetailLayoutBuilder* DetailBuilder);

private:
    TSharedPtr<FDungeonBuilderClassCombo> BuilderClassCustomization;
};

class FSnapModuleDatabaseCustomization : public IDetailCustomization {
public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    // End of IDetailCustomization interface

    static TSharedRef<IDetailCustomization> MakeInstance();
    static FReply BuildDatabaseCache(IDetailLayoutBuilder* DetailBuilder);
};

UCLASS()
class USnapGridFlowModuleDBImportSettings : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = Module)
    FName Category = "Room";

    UPROPERTY(EditAnywhere, Category = Module)
    bool bAllowRotation = true;
};

class FSnapGridFlowModuleDatabaseCustomization : public IDetailCustomization, public FGCObject {
public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    // End of IDetailCustomization interface
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override;

    static TSharedRef<IDetailCustomization> MakeInstance();
    static FReply BuildDatabaseCache(IDetailLayoutBuilder* DetailBuilder);
    
    FReply HandleTool_AddModulesFromDir(IDetailLayoutBuilder* DetailBuilder);
    FReply HandleTool_AddModulesFromDir_ButtonOK(USnapGridFlowModuleDBImportSettings* InSettings, class USnapGridFlowModuleDatabase* ModuleDatabase, FString ImportPath);
    FReply HandleTool_AddModulesFromDir_ButtonCancel();

private:
    USnapGridFlowModuleDBImportSettings* DirImportSettings = nullptr;
    TWeakPtr<SWindow> DirImportSettingsWindow;
};

class FDungeonArchitectMeshNodeCustomization : public IDetailCustomization {
public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    // End of IDetailCustomization interface

    static FReply EditAdvancedOptions(IDetailLayoutBuilder* DetailBuilder);
};


class FDungeonEditorViewportPropertiesCustomization : public IDetailCustomization {
public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    // End of IDetailCustomization interface
    static TSharedRef<IDetailCustomization> MakeInstance();

private:
    TSharedPtr<FDungeonBuilderClassCombo> BuilderClassCustomization;
};

class FDungeonArchitectVolumeCustomization : public IDetailCustomization {
public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    // End of IDetailCustomization interface

    static TSharedRef<IDetailCustomization> MakeInstance();

    static FReply RebuildDungeon(IDetailLayoutBuilder* DetailBuilder);
};

class FDAExecRuleNodeCustomization : public IDetailCustomization {
public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;
    // End of IDetailCustomization interface

    void OnExecutionModeChanged(class UEdGraphNode_GrammarExecRuleNode* Node);

    static TSharedRef<IDetailCustomization> MakeInstance();
private:
    TWeakPtr<IDetailLayoutBuilder> CachedDetailBuilder;
};

class FDungeonPropertyChangeListener : public TSharedFromThis<FDungeonPropertyChangeListener> {
public:
    void Initialize();
    void OnPropertyChanged(UObject* Object, struct FPropertyChangedEvent& Event);

private:
    TSharedPtr<IPropertyChangeListener> PropertyChangeListener;
};


class FDungeonDebugCustomization : public IDetailCustomization {
public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    // End of IDetailCustomization interface

    static TSharedRef<IDetailCustomization> MakeInstance();

    static FReply ExecuteCommand(IDetailLayoutBuilder* DetailBuilder, int32 CommandID);
};


class FMGPatternRuleCustomization : public IDetailCustomization {
public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    // End of IDetailCustomization interface

    static TSharedRef<IDetailCustomization> MakeInstance();

    static FReply RandomizeColor(IDetailLayoutBuilder* DetailBuilder);
};

class FMGPatternGridLayerCustomization : public IDetailCustomization {
public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    // End of IDetailCustomization interface

    static TSharedRef<IDetailCustomization> MakeInstance();
};

