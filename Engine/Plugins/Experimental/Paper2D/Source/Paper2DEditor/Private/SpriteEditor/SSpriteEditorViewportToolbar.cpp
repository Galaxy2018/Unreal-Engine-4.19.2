// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "SSpriteEditorViewportToolbar.h"
#include "SpriteEditorCommands.h"
#include "SEditorViewport.h"

#define LOCTEXT_NAMESPACE "SSpriteEditorViewportToolbar"

///////////////////////////////////////////////////////////
// SSpriteEditorViewportToolbar

void SSpriteEditorViewportToolbar::Construct(const FArguments& InArgs, TSharedPtr<class ICommonEditorViewportToolbarInfoProvider> InInfoProvider)
{
	SCommonEditorViewportToolbarBase::Construct(SCommonEditorViewportToolbarBase::FArguments(), InInfoProvider);
}

TSharedRef<SWidget> SSpriteEditorViewportToolbar::GenerateShowMenu() const
{
	GetInfoProvider().OnFloatingButtonClicked();
	
	TSharedRef<SEditorViewport> ViewportRef = GetInfoProvider().GetViewportWidget();

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder ShowMenuBuilder(bInShouldCloseWindowAfterMenuSelection, ViewportRef->GetCommandList());
	{
		ShowMenuBuilder.AddMenuEntry(FSpriteEditorCommands::Get().SetShowSockets);
		ShowMenuBuilder.AddMenuEntry(FSpriteEditorCommands::Get().SetShowPivot);

		ShowMenuBuilder.AddMenuSeparator();

		ShowMenuBuilder.AddMenuEntry(FSpriteEditorCommands::Get().SetShowGrid);
		ShowMenuBuilder.AddMenuEntry(FSpriteEditorCommands::Get().SetShowBounds);
		ShowMenuBuilder.AddMenuEntry(FSpriteEditorCommands::Get().SetShowNormals);

		ShowMenuBuilder.AddMenuSeparator();

		ShowMenuBuilder.AddMenuEntry(FSpriteEditorCommands::Get().SetShowCollision);
		ShowMenuBuilder.AddMenuEntry(FSpriteEditorCommands::Get().SetShowMeshEdges);
	}

	return ShowMenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
