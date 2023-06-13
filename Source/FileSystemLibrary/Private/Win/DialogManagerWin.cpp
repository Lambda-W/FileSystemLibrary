// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Copyright Lambda Works, Samuel Metters 2019. All rights reserved.

// This class is responsible for Dialogs on the Windows platform.

#include "Win/DialogManagerWin.h"
#if PLATFORM_WINDOWS
#include "Windows/MinWindows.h"
#include "Windows/COMPointer.h"
#endif
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "HAL/FileManager.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <Winver.h>
#include <LM.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

bool DialogManagerWin::OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, bool MultipleFiles, TArray<FString>& OutFilenames)
{
    #if PLATFORM_WINDOWS
	int32 dummy = 0;
	return FileDialogShared(false, ParentWindowHandle, DialogTitle, DefaultPath, DefaultFile, FileTypes, MultipleFiles, OutFilenames, dummy);
#endif
    return false;
}

bool DialogManagerWin::SaveFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, bool MultipleFiles, TArray<FString>& OutFilenames)
{
    #if PLATFORM_WINDOWS
	int32 dummy = 0;
	return FileDialogShared(true, ParentWindowHandle, DialogTitle, DefaultPath, DefaultFile, FileTypes, MultipleFiles, OutFilenames, dummy);
#endif
    return false;
}

bool DialogManagerWin::OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName)
{
	bool bSuccess = false;
    
    #if PLATFORM_WINDOWS
	TComPtr<IFileOpenDialog> FileDialog;

	if (SUCCEEDED(::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&FileDialog))))
	{
		// Set this up as a folder picker
		DWORD dwFlags = 0;
		FileDialog->GetOptions(&dwFlags);
		FileDialog->SetOptions(dwFlags | FOS_PICKFOLDERS);

		// Set up common settings
		FileDialog->SetTitle(*DialogTitle);
		if (!DefaultPath.IsEmpty())
		{
			// SHCreateItemFromParsingName requires the given path be absolute and use \ rather than / as our normalized paths do
			FString DefaultWindowsPath = FPaths::ConvertRelativePathToFull(DefaultPath);
			DefaultWindowsPath.ReplaceInline(TEXT("/"), TEXT("\\"), ESearchCase::CaseSensitive);

			TComPtr<IShellItem> DefaultPathItem;
			if (SUCCEEDED(::SHCreateItemFromParsingName(*DefaultWindowsPath, nullptr, IID_PPV_ARGS(&DefaultPathItem))))
			{
				FileDialog->SetFolder(DefaultPathItem);
			}
		}

		// Show the picker
		if (SUCCEEDED(FileDialog->Show((HWND)ParentWindowHandle)))
		{
			TComPtr<IShellItem> Result;
			if (SUCCEEDED(FileDialog->GetResult(&Result)))
			{
				PWSTR pFilePath = nullptr;
				if (SUCCEEDED(Result->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath)))
				{
					bSuccess = true;

					OutFolderName = pFilePath;
					FPaths::NormalizeDirectoryName(OutFolderName);

					::CoTaskMemFree(pFilePath);
				}
			}
		}
	}
#endif

	return bSuccess;
}

bool DialogManagerWin::FileDialogShared(bool isSaveFileDialog, const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, bool MultipleFiles, TArray<FString>& OutFilenames, int32& OutFilterIndex)
{
	bool bSuccess = false;
    
#if PLATFORM_WINDOWS
	TComPtr<IFileDialog> FileDialog;

	if (SUCCEEDED(::CoCreateInstance(isSaveFileDialog ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, isSaveFileDialog ? IID_IFileSaveDialog : IID_IFileOpenDialog, IID_PPV_ARGS_Helper(&FileDialog))))
	{
		// Is this a FileSaveDialog
		if (isSaveFileDialog)
		{
			// Set the default "filename"
			if (!DefaultFile.IsEmpty())
			{
				FileDialog->SetFileName(*FPaths::GetCleanFilename(DefaultFile));
			}
		}
		
		// If not it is a FileSelectDialog
		else
		{
			// Set this up as a multi-select picker
			if (MultipleFiles)
			{
				DWORD dwFlags = 0;
				FileDialog->GetOptions(&dwFlags);
				FileDialog->SetOptions(dwFlags | FOS_ALLOWMULTISELECT);
			}
		}

		// Set up common settings
		FileDialog->SetTitle(*DialogTitle);
		if (!DefaultPath.IsEmpty())
		{
			// SHCreateItemFromParsingName requires the given path be absolute and use \ rather than / as our normalized paths do
			FString DefaultWindowsPath = FPaths::ConvertRelativePathToFull(DefaultPath);
			DefaultWindowsPath.ReplaceInline(TEXT("/"), TEXT("\\"), ESearchCase::CaseSensitive);

			TComPtr<IShellItem> DefaultPathItem;
			if (SUCCEEDED(::SHCreateItemFromParsingName(*DefaultWindowsPath, nullptr, IID_PPV_ARGS(&DefaultPathItem))))
			{
				FileDialog->SetFolder(DefaultPathItem);
			}
		}


		// Set-up the file type filters
		TArray<FString> UnformattedExtensions;
		TArray<COMDLG_FILTERSPEC> FileDialogFilters;
		{
			// Split the given filter string (formatted as "Pair1String1|Pair1String2|Pair2String1|Pair2String2") into the Windows specific filter struct
			FileTypes.ParseIntoArray(UnformattedExtensions, TEXT("|"), true);

			if (UnformattedExtensions.Num() % 2 == 0)
			{
				FileDialogFilters.Reserve(UnformattedExtensions.Num() / 2);
				for (int32 ExtensionIndex = 0; ExtensionIndex < UnformattedExtensions.Num();)
				{
					COMDLG_FILTERSPEC& NewFilterSpec = FileDialogFilters[FileDialogFilters.AddDefaulted()];
					NewFilterSpec.pszName = *UnformattedExtensions[ExtensionIndex++];
					NewFilterSpec.pszSpec = *UnformattedExtensions[ExtensionIndex++];
				}
			}
		}
		FileDialog->SetFileTypes(FileDialogFilters.Num(), FileDialogFilters.GetData());

		// Show the picker
		if (SUCCEEDED(FileDialog->Show((HWND)ParentWindowHandle)))
		{
			OutFilterIndex = 0;
			if (SUCCEEDED(FileDialog->GetFileTypeIndex((UINT*)&OutFilterIndex)))
			{
				OutFilterIndex -= 1; // GetFileTypeIndex returns a 1-based index
			}

			auto AddOutFilename = [&OutFilenames](const FString& InFilename)
			{
				FString& OutFilename = OutFilenames[OutFilenames.Add(InFilename)];
				OutFilename = IFileManager::Get().ConvertToRelativePath(*OutFilename);
				FPaths::NormalizeFilename(OutFilename);
			};

			// Clean the output path depending on whether or not this is a FileSaveDialog or FileSelectDialog
			
			if (isSaveFileDialog)
			{
				TComPtr<IShellItem> Result;
				if (SUCCEEDED(FileDialog->GetResult(&Result)))
				{
					PWSTR pFilePath = nullptr;
					if (SUCCEEDED(Result->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath)))
					{
						bSuccess = true;

						// Apply the selected extension if the given filename doesn't already have one
						FString SaveFilePath = pFilePath;
						if (FileDialogFilters.IsValidIndex(OutFilterIndex))
						{
							// May have multiple semi-colon separated extensions in the pattern
							const FString ExtensionPattern = FileDialogFilters[OutFilterIndex].pszSpec;
							TArray<FString> SaveExtensions;
							ExtensionPattern.ParseIntoArray(SaveExtensions, TEXT(";"));

							// Build a "clean" version of the selected extension (without the wildcard)
							FString CleanExtension = SaveExtensions[0];
							if (CleanExtension == TEXT("*.*"))
							{
								CleanExtension.Reset();
							}
							else
							{
								int32 WildCardIndex = INDEX_NONE;
								if (CleanExtension.FindChar(TEXT('*'), WildCardIndex))
								{
									CleanExtension = CleanExtension.RightChop(WildCardIndex + 1);
								}
							}

							// We need to split these before testing the extension to avoid anything within the path being treated as a file extension
							FString SaveFileName = FPaths::GetCleanFilename(SaveFilePath);
							SaveFilePath = FPaths::GetPath(SaveFilePath);

							// Apply the extension if the file name doesn't already have one
							if (FPaths::GetExtension(SaveFileName).IsEmpty() && !CleanExtension.IsEmpty())
							{
								SaveFileName = FPaths::SetExtension(SaveFileName, CleanExtension);
							}

							SaveFilePath /= SaveFileName;
						}
						AddOutFilename(SaveFilePath);

						::CoTaskMemFree(pFilePath);
					}
				}
			}
			
			else
			{
				IFileOpenDialog* FileOpenDialog = static_cast<IFileOpenDialog*>(FileDialog.Get());

				TComPtr<IShellItemArray> Results;
				if (SUCCEEDED(FileOpenDialog->GetResults(&Results)))
				{
					DWORD NumResults = 0;
					Results->GetCount(&NumResults);
					for (DWORD ResultIndex = 0; ResultIndex < NumResults; ++ResultIndex)
					{
						TComPtr<IShellItem> Result;
						if (SUCCEEDED(Results->GetItemAt(ResultIndex, &Result)))
						{
							PWSTR pFilePath = nullptr;
							if (SUCCEEDED(Result->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath)))
							{
								bSuccess = true;
								AddOutFilename(pFilePath);
								::CoTaskMemFree(pFilePath);
							}
						}
					}
				}
			}
		}
	}

#endif
	return bSuccess;
}
