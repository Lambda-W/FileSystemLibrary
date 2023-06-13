// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Copyright Lambda Works, Samuel Metters 2019. All rights reserved.


#include "DialogManager.h"

DialogManager::DialogManager()
{
}

DialogManager::~DialogManager()
{
}

bool DialogManager::OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, bool MultipleFiles, TArray<FString>& OutFilenames)
{


	return false;
}

bool DialogManager::SaveFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, bool MultipleFiles, TArray<FString>& OutFilenames)
{
	return false;
}

bool DialogManager::OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName)
{
	return false;
}

