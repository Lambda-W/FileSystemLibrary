// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Copyright Lambda Works, Samuel Metters 2019. All rights reserved.

// This class is responsible for Dialogs on the Windows platform.

#pragma once

#include "CoreMinimal.h"
#include "DialogManager.h"



class DialogManagerWin : public DialogManager
{
public:
	virtual bool OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, bool MultipleFiles, TArray<FString>& OutFilenames) override;
	virtual bool SaveFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, bool MultipleFiles, TArray<FString>& OutFilenames)override;
	virtual bool OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName) override;

private:
	bool FileDialogShared(bool bSave, const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, bool MultipleFiles, TArray<FString>& OutFilenames, int32& OutFilterIndex);

};

