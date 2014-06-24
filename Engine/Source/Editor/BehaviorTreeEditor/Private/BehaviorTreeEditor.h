// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IBehaviorTreeEditor.h"
#include "Toolkits/AssetEditorToolkit.h"

class UBehaviorTree;
class UBlackboardData;
struct FBlackboardEntry;

class FBehaviorTreeEditor : public IBehaviorTreeEditor, public FEditorUndoClient, public FNotifyHook
{
public:
	FBehaviorTreeEditor();
	/** Destructor */
	virtual ~FBehaviorTreeEditor();

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	void InitBehaviorTreeEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* InObject );

	// Begin IToolkit interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FText GetToolkitName() const override;
	// End IToolkit interface

	// Begin IBehaviorTreeEditor interface
	virtual uint32 GetSelectedNodesCount() const override { return SelectedNodesCount; }
	virtual void InitializeDebuggerState(class FBehaviorTreeDebugger* ParentDebugger) const override;
	virtual UEdGraphNode* FindInjectedNode(int32 Index) const override;
	virtual void DoubleClickNode(class UEdGraphNode* Node) override;
	virtual void FocusWindow(UObject* ObjectToFocusOn = NULL) override;
	// End IBehaviorTreeEditor interface

	// Begin FEditorUndoClient Interface
	virtual void	PostUndo(bool bSuccess) override;
	virtual void	PostRedo(bool bSuccess) override;
	// End of FEditorUndoClient

	// Begin FNotifyHook Interface
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged ) override;
	// End of FNotifyHook

	// Delegates
	void OnNodeDoubleClicked(class UEdGraphNode* Node);
	void OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor);
	FGraphPanelSelectionSet GetSelectedNodes() const;

	void OnAddInputPin();
	bool CanAddInputPin() const;
	void OnRemoveInputPin();
	bool CanRemoveInputPin() const;

	void OnEnableBreakpoint();
	bool CanEnableBreakpoint() const;
	void OnToggleBreakpoint();
	bool CanToggleBreakpoint() const;
	void OnDisableBreakpoint();
	bool CanDisableBreakpoint() const;
	void OnAddBreakpoint();
	bool CanAddBreakpoint() const;
	void OnRemoveBreakpoint();
	bool CanRemoveBreakpoint() const;
	
	void SelectAllNodes();
	bool CanSelectAllNodes() const;
	void DeleteSelectedNodes();
	bool CanDeleteNodes() const;
	void DeleteSelectedDuplicatableNodes();
	void CutSelectedNodes();
	bool CanCutNodes() const;
	void CopySelectedNodes();
	bool CanCopyNodes() const;
	void PasteNodes();
	void PasteNodesHere(const FVector2D& Location);
	bool CanPasteNodes() const;
	void DuplicateNodes();
	bool CanDuplicateNodes() const;

	void SearchTree();
	bool CanSearchTree() const;

	void JumpToNode(const UEdGraphNode* Node);

	bool IsPropertyVisible( const struct FPropertyAndParent& PropertyAndParent ) const;
	bool IsPropertyEditable() const;
	void OnPackageSaved(const FString& PackageFileName, UObject* Outer);
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	void OnClassListUpdated();

	void UpdateToolbar();
	bool IsDebuggerReady() const;

	/** Get whether the debugger is currently running and the PIE session is paused */
	bool IsDebuggerPaused() const;

	/** Get whether we can edit the tree/blackboard with the debugger active */
	bool CanEditWithDebuggerActive() const;

	TSharedRef<class SWidget> OnGetDebuggerActorsMenu();
	void OnDebuggerActorSelected(TWeakObjectPtr<UBehaviorTreeComponent> InstanceToDebug);
	FString GetDebuggerActorDesc() const;
	FGraphAppearanceInfo GetGraphAppearance() const;
	bool InEditingMode(bool bGraphIsEditable) const;

	void DebuggerSwitchAsset(UBehaviorTree* NewAsset);
	void DebuggerUpdateGraph();

	EVisibility GetDebuggerDetailsVisibility() const;
	EVisibility GetRangeLowerVisibility() const;
	EVisibility GetRangeSelfVisibility() const;
	EVisibility GetInjectedNodeVisibility() const;

	TWeakPtr<SGraphEditor> GetFocusedGraphPtr() const;

	/** Check whether the behavior tree mode can be accessed (i.e whether we have a valid tree to edit) */
	bool CanAccessBehaviorTreeMode() const;

	/** Check whether the blackboard mode can be accessed (i.e whether we have a valid blackboard to edit) */
	bool CanAccessBlackboardMode() const;

	/** 
	 * Get the localized text to display for the specified mode 
	 * @param	InMode	The mode to display
	 * @return the localized text representation of the mode
	 */
	static FText GetLocalizedMode(FName InMode);

	/** Access the toolbar builder for this editor */
	TSharedPtr<class FBehaviorTreeEditorToolbar> GetToolbarBuilder() { return ToolbarBuilder; }

	/** Get the behavior tree we are editing (if any) */
	UBehaviorTree* GetBehaviorTree() const;

	/** Get the blackboard we are editing (if any) */
	UBlackboardData* GetBlackboardData() const;

	/** Spawns the tab with the update graph inside */
	TSharedRef<SWidget> SpawnProperties();

	/** Spawns the search tab */
	TSharedRef<SWidget> SpawnSearch();

	/** Spawn blackboard details tab */
	TSharedRef<SWidget> SpawnBlackboardDetails();

	/** Spawn blackboard view tab */
	TSharedRef<SWidget> SpawnBlackboardView();

	/** Spawn blackboard editor tab */
	TSharedRef<SWidget> SpawnBlackboardEditor();

	// @todo This is a hack for now until we reconcile the default toolbar with application modes [duplicated from counterpart in Blueprint Editor]
	void RegisterToolbarTab(const TSharedRef<class FTabManager>& TabManager);

	/** Restores the behavior tree graph we were editing or creates a new one if none is available */
	void RestoreBehaviorTree();

	/** Save the graph state for later editing */
	void SaveEditedObjectState();

	/** Delegate handler for selection in the blackboard entry list */
	void HandleBlackboardEntrySelected(const FBlackboardEntry* BlackboardEntry, bool bIsInherited);

	/** Delegate handler used to retrieve current blackboard selection */
	int32 HandleGetSelectedBlackboardItemIndex(bool& bOutIsInherited);

	/** Delegate handler for displaying debugger values */
	FText HandleGetDebugKeyValue(const FName& InKeyName, bool bUseCurrentState) const;

	/** Delegate handler for retrieving timestamp to display */
	float HandleGetDebugTimeStamp(bool bUseCurrentState) const;

	/** Delegate handler for when the debugged blackboard changes */
	void HandleDebuggedBlackboardChanged(UBlackboardData* InObject);

	/** Delegate handler for determining whether to display the current state */
	bool HandleGetDisplayCurrentState() const;

	/** Get the currently selected blackboard entry */
	void GetBlackboardSelectionInfo(int32& OutSelectionIndex, bool& bOutIsInherited) const;

	/** Check to see if we can create a new task node */
	bool CanCreateNewTask() const;

	/** Check to see if we can create a new decorator node */
	bool CanCreateNewDecorator() const;

	/** Check to see if we can create a new service node */
	bool CanCreateNewService() const;

	/** Create the menu used to make a new task node */
	TSharedRef<SWidget> HandleCreateNewTaskMenu() const;

	/** Create the menu used to make a new decorator */
	TSharedRef<SWidget> HandleCreateNewDecoratorMenu() const;

	/** Create the menu used to make a new service */
	TSharedRef<SWidget> HandleCreateNewServiceMenu() const;

	/** Handler for when a node class is picked */
	void HandleNewNodeClassPicked(UClass* InClass) const;

	/** Create a new task from UBTTask_BlueprintBase */
	void CreateNewTask() const;

	/** Whether the single button to create a new Blueprint-based task is visible */
	bool IsNewTaskButtonVisible() const;

	/** Whether the combo button to create a new Blueprint-based task from all available base classes is visible */
	bool IsNewTaskComboVisible() const;

	/** Create a new decorator from UBTDecorator_BlueprintBase */
	void CreateNewDecorator() const;

	/** Whether the single button to create a new Blueprint-based decorator is visible */
	bool IsNewDecoratorButtonVisible() const;

	/** Whether the combo button to create a new Blueprint-based decorator from all available base classes is visible */
	bool IsNewDecoratorComboVisible() const;

	/** Create a new service from UBTService_BlueprintBase */
	void CreateNewService() const;

	/** Whether the single button to create a new Blueprint-based service is visible */
	bool IsNewServiceButtonVisible() const;

	/** Whether the combo button to create a new Blueprint-based service from all available base classes is visible */
	bool IsNewServiceComboVisible() const;

protected:
	/** Called when "Save" is clicked for this asset */
	virtual void SaveAsset_Execute() override;

private:
	/** Create widget for graph editing */
	TSharedRef<class SGraphEditor> CreateGraphEditorWidget(UEdGraph* InGraph);

	/** Creates all internal widgets for the tabs to point at */
	void CreateInternalWidgets();

	/** Add custom menu options */
	void ExtendMenu();

	/** Setup common commands */
	void BindCommonCommands();

	/** Setup commands */
	void BindDebuggerToolbarCommands();

	/** Called when the selection changes in the GraphEditor */
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	/** prepare range of nodes that can be aborted by this decorator */
	void GetAbortModePreview(const class UBehaviorTreeGraphNode_CompositeDecorator* Node, struct FAbortDrawHelper& Mode0, struct FAbortDrawHelper& Mode1);

	/** prepare range of nodes that can be aborted by this decorator */
	void GetAbortModePreview(const class UBTDecorator* DecoratorOb, struct FAbortDrawHelper& Mode0, struct FAbortDrawHelper& Mode1);

	/** Refresh the debugger's display */
	void RefreshDebugger();

	TSharedPtr<FDocumentTracker> DocumentManager;
	TWeakPtr<FDocumentTabFactory> GraphEditorTabFactoryPtr;

	/* The Behavior Tree being edited */
	UBehaviorTree* BehaviorTree;

	/* The Blackboard Data being edited */
	UBlackboardData* BlackboardData;

	TWeakPtr<SGraphEditor> FocusedGraphEdPtr;
	TWeakObjectPtr<class UBehaviorTreeGraphNode_CompositeDecorator> FocusedGraphOwner;

	/** Property View */
	TSharedPtr<class IDetailsView> DetailsView;

	/** The command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;

	TSharedPtr<class FBehaviorTreeDebugger> Debugger;

	/** Find results log as well as the search filter */
	TSharedPtr<class SFindInBT> FindResults;

	uint32 bShowDecoratorRangeLower : 1;
	uint32 bShowDecoratorRangeSelf : 1;
	uint32 bForceDisablePropertyEdit : 1;
	uint32 bSelectedNodeIsInjected : 1;
	uint32 SelectedNodesCount;

	TSharedPtr<class FBehaviorTreeEditorToolbar> ToolbarBuilder;

	/** The details view we use to display the blackboard */
	TSharedPtr<IDetailsView> BlackboardDetailsView;

	/** The blackboard view widget */
	TSharedPtr<class SBehaviorTreeBlackboardView> BlackboardView;

	/** The blackboard editor widget */
	TSharedPtr<class SBehaviorTreeBlackboardEditor> BlackboardEditor;

	/** The current blackboard selection index, stored here so it can be accessed by our details customization */
	int32 CurrentBlackboardEntryIndex;

	/** Whether the current selection is inherited, stored here so it can be accessed by our details customization */
	bool bIsCurrentBlackboardEntryInherited;

public:
	/** Modes in mode switcher */
	static const FName BehaviorTreeMode;
	static const FName BlackboardMode;
};
