// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "NiagaraMetaDataCollectionViewModel.h"
#include "NiagaraScriptInputCollectionViewModel.h"
#include "NiagaraScriptOutputCollectionViewModel.h"
#include "NiagaraMetaDataCollectionViewModel.h"
#include "NiagaraMetaDataViewModel.h"
#include "NiagaraScriptViewModel.h"
#include "NiagaraParameterViewModel.h"
#include "NiagaraEditorStyle.h"
#include "IDetailChildrenBuilder.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Images/SImage.h"
#include "NiagaraEditorModule.h"
#include "Modules/ModuleManager.h"
#include "INiagaraEditorTypeUtilities.h"
#include "Widgets/Layout/SBox.h"
#include "NiagaraScript.h"
#include "UObject/StructOnScope.h"
#include "Internationalization/Text.h"
#include "IDetailGroup.h"


#define LOCTEXT_NAMESPACE "NiagaraMetaDataCustomNodeBuilder"

class FNiagaraMetaDataCustomNodeBuilder : public IDetailCustomNodeBuilder
{
public:
	FNiagaraMetaDataCustomNodeBuilder(TSharedRef<FNiagaraMetaDataCollectionViewModel> InViewModel)
		: ViewModel(InViewModel)
	{
		ViewModel->OnCollectionChanged().AddRaw(this, &FNiagaraMetaDataCustomNodeBuilder::OnCollectionViewModelChanged);
	}

	~FNiagaraMetaDataCustomNodeBuilder()
	{
		ViewModel->OnCollectionChanged().RemoveAll(this);
	}

	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override
	{
		OnRebuildChildren = InOnRegenerateChildren;
	}

	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) {}
	virtual void Tick(float DeltaTime) override {}
	virtual bool RequiresTick() const override { return false; }
	virtual bool InitiallyCollapsed() const { return false; }
	
	virtual FName GetName() const  override
	{
		static const FName NiagaraMetadataCustomNodeBuilder("NiagaraMetadataCustomNodeBuilder");
		return NiagaraMetadataCustomNodeBuilder;
	}

	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override
	{
		const TArray<TSharedRef<FNiagaraMetaDataViewModel>>& VariableModels = ViewModel->GetVariableModels();

		FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::GetModuleChecked<FNiagaraEditorModule>("NiagaraEditor");

		for (const TSharedRef<FNiagaraMetaDataViewModel>& MetadataViewModel : VariableModels)
		{
			IDetailGroup& MetaDataGroup = ChildrenBuilder.AddGroup(MetadataViewModel->GetName(), FText::FromName(MetadataViewModel->GetName()));
			MetaDataGroup.ToggleExpansion(true);
			TSharedRef<FStructOnScope> StructData = MetadataViewModel->GetValueStruct();
			
			IDetailPropertyRow* DescriptionRow = ChildrenBuilder.AddExternalStructureProperty(StructData, FName(TEXT("Description")), "Description");

			if (DescriptionRow)
			{
				DescriptionRow->Visibility(EVisibility::Hidden); // hide it here, show it in groups only
				MetaDataGroup.AddPropertyRow(DescriptionRow->GetPropertyHandle()->AsShared());

				TSharedPtr<IPropertyHandle> PropertyHandle = DescriptionRow->GetPropertyHandle();
				PropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(MetadataViewModel, &FNiagaraMetaDataViewModel::NotifyMetaDataChanged));
			}

			IDetailPropertyRow* MetaDataRow = ChildrenBuilder.AddExternalStructureProperty(StructData, FName(TEXT("PropertyMetaData")), "PropertyMetaData");
			
			if (MetaDataRow)
			{
				MetaDataRow->ShouldAutoExpand(true);
				MetaDataRow->Visibility(EVisibility::Hidden);
				MetaDataGroup.AddPropertyRow(MetaDataRow->GetPropertyHandle()->AsShared());

				TSharedPtr<IPropertyHandle> PropertyHandle = MetaDataRow->GetPropertyHandle();
				PropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(MetadataViewModel, &FNiagaraMetaDataViewModel::NotifyMetaDataChanged));
				PropertyHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(MetadataViewModel, &FNiagaraMetaDataViewModel::NotifyMetaDataChanged));
			}
		}
	}

private:
	void OnCollectionViewModelChanged()
	{
		OnRebuildChildren.ExecuteIfBound();
	}

private:
	TSharedRef<FNiagaraMetaDataCollectionViewModel> ViewModel;
	FSimpleDelegate OnRebuildChildren;
};

#undef LOCTEXT_NAMESPACE // "NiagaraMetaDataCustomNodeBuilder"
