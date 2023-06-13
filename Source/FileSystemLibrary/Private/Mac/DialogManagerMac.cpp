// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Copyright Lambda Works, Samuel Metters 2019. All rights reserved.

// This class is responsible for Dialogs on the Windows platform.

#include "Mac/DialogManagerMac.h"
#include "Mac/MacApplication.h"
#include "Misc/FeedbackContextMarkup.h"
#include "Mac/CocoaThread.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Guid.h"
#include "HAL/FileManager.h"

class FCocoaScopeContext
{
public:
	FCocoaScopeContext(void)
	{
		SCOPED_AUTORELEASE_POOL;
		PreviousContext = [NSOpenGLContext currentContext];
	}

	~FCocoaScopeContext(void)
	{
		SCOPED_AUTORELEASE_POOL;
		NSOpenGLContext *NewContext = [NSOpenGLContext currentContext];
		if (PreviousContext != NewContext)
		{
			if (PreviousContext)
			{
				[PreviousContext makeCurrentContext];
			}
			else
			{
				[NSOpenGLContext clearCurrentContext];
			}
		}
	}

private:
	NSOpenGLContext *PreviousContext;
};

/**
 * Custom accessory view class to allow choose kind of file extension
 */
@interface FileDialogAccessoryView : NSView
{
@private
	NSPopUpButton *PopUpButton;
	NSTextField *TextField;
	NSSavePanel *DialogPanel;
	NSMutableArray *AllowedFileTypes;
	int32 SelectedExtension;
}

- (id)initWithFrame:(NSRect)frameRect dialogPanel:(NSSavePanel *)panel;
- (void)PopUpButtonAction:(id)sender;
- (void)AddAllowedFileTypes:(NSArray *)array;
- (void)SetExtensionsAtIndex:(int32)index;
- (int32)SelectedExtension;

@end

@implementation FileDialogAccessoryView

- (id)initWithFrame:(NSRect)frameRect dialogPanel:(NSSavePanel *)panel
{
	self = [super initWithFrame:frameRect];
	DialogPanel = panel;

	FString FieldText = TEXT("File extension:");
	CFStringRef FieldTextCFString = FPlatformString::TCHARToCFString(*FieldText);
	TextField = [[NSTextField alloc] initWithFrame:NSMakeRect(0.0, 48.0, 90.0, 25.0)];
	[TextField setStringValue:(NSString *)FieldTextCFString];
	[TextField setEditable:NO];
	[TextField setBordered:NO];
	[TextField setBackgroundColor:[NSColor controlColor]];

	PopUpButton = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(88.0, 50.0, 160.0, 25.0)];
	[PopUpButton setTarget:self];
	[PopUpButton setAction:@selector(PopUpButtonAction:)];

	[self addSubview:TextField];
	[self addSubview:PopUpButton];

	return self;
}

- (void)AddAllowedFileTypes:(NSMutableArray *)array
{
	check(array);

	AllowedFileTypes = array;
	int32 ArrayCount = [AllowedFileTypes count];
	if (ArrayCount)
	{

		[PopUpButton removeAllItems];

		for (int32 Index = 0; Index < ArrayCount; Index += 2)
		{
			[PopUpButton addItemWithTitle:[AllowedFileTypes objectAtIndex:Index]];
		}

		// Set allowed extensions
		[self SetExtensionsAtIndex:0];
	}
	else
	{
		// Allow all file types
		[DialogPanel setAllowedFileTypes:nil];
	}
}

- (void)PopUpButtonAction:(id)sender
{
	NSInteger Index = [PopUpButton indexOfSelectedItem];
	[self SetExtensionsAtIndex:Index];
}

- (void)SetExtensionsAtIndex:(int32)index
{
	check([AllowedFileTypes count] >= index * 2);
	SelectedExtension = index;

	NSString *ExtsToParse = [AllowedFileTypes objectAtIndex:index * 2 + 1];
	if ([ExtsToParse compare:@"*.*"] == NSOrderedSame)
	{
		[DialogPanel setAllowedFileTypes:nil];
	}
	else
	{
		NSArray *ExtensionsWildcards = [ExtsToParse componentsSeparatedByString:@";"];
		NSMutableArray *Extensions = [NSMutableArray arrayWithCapacity:[ExtensionsWildcards count]];

		for (int32 Index = 0; Index < [ExtensionsWildcards count]; ++Index)
		{
			NSString *Temp = [[ExtensionsWildcards objectAtIndex:Index] stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"*."]];
			[Extensions addObject:Temp];
		}

		[DialogPanel setAllowedFileTypes:Extensions];
	}
}

- (int32)SelectedExtension
{
	return SelectedExtension;
}

@end

bool DialogManagerMac::OpenFileDialog(const void *ParentWindowHandle, const FString &DialogTitle, const FString &DefaultPath, const FString &DefaultFile, const FString &FileTypes, bool MultipleFiles, TArray<FString> &OutFilenames)
{
	int32 dummy = 0;
	return FileDialogShared(false, ParentWindowHandle, DialogTitle, DefaultPath, DefaultFile, FileTypes, MultipleFiles, OutFilenames, dummy);
}

bool DialogManagerMac::SaveFileDialog(const void *ParentWindowHandle, const FString &DialogTitle, const FString &DefaultPath, const FString &DefaultFile, const FString &FileTypes, bool MultipleFiles, TArray<FString> &OutFilenames)
{
	int32 dummy = 0;
	return FileDialogShared(true, ParentWindowHandle, DialogTitle, DefaultPath, DefaultFile, FileTypes, MultipleFiles, OutFilenames, dummy);
}

bool DialogManagerMac::OpenDirectoryDialog(const void *ParentWindowHandle, const FString &DialogTitle, const FString &DefaultPath, FString &OutFolderName)
{

	bool bSuccess = false;
	{
		bSuccess = MainThreadReturn(^{
		  SCOPED_AUTORELEASE_POOL;
		  FCocoaScopeContext ContextGuard;

		  NSOpenPanel *Panel = [NSOpenPanel openPanel];
		  [Panel setCanChooseFiles:false];
		  [Panel setCanChooseDirectories:true];
		  [Panel setAllowsMultipleSelection:false];
		  [Panel setCanCreateDirectories:true];

		  CFStringRef Title = FPlatformString::TCHARToCFString(*DialogTitle);
		  [Panel setTitle:(NSString *)Title];
		  CFRelease(Title);

		  CFStringRef DefaultPathCFString = FPlatformString::TCHARToCFString(*DefaultPath);
		  NSURL *DefaultPathURL = [NSURL fileURLWithPath:(NSString *)DefaultPathCFString];
		  [Panel setDirectoryURL:DefaultPathURL];
		  CFRelease(DefaultPathCFString);

		  bool bResult = false;

		  NSInteger Result = [Panel runModal];

		  if (Result == NSModalResponseOK)
		  {
			  NSURL *FolderURL = [[Panel URLs] objectAtIndex:0];
			  TCHAR FolderPath[MAC_MAX_PATH];
			  FPlatformString::CFStringToTCHAR((CFStringRef)[FolderURL path], FolderPath);
			  OutFolderName = FolderPath;
			  FPaths::NormalizeFilename(OutFolderName);

			  bResult = true;
		  }

		  [Panel close];

		  return bResult;
		});
	}

	return bSuccess;
}

bool DialogManagerMac::FileDialogShared(bool bSave, const void *ParentWindowHandle, const FString &DialogTitle, const FString &DefaultPath, const FString &DefaultFile, const FString &FileTypes, bool MultipleFiles, TArray<FString> &OutFilenames, int32 &OutFilterIndex)
{

	bool bSuccess = false;
	{
		bSuccess = MainThreadReturn(^{
		  SCOPED_AUTORELEASE_POOL;
		  FCocoaScopeContext ContextGuard;

		  NSSavePanel *Panel = bSave ? [NSSavePanel savePanel] : [NSOpenPanel openPanel];

		  if (!bSave)
		  {
			  NSOpenPanel *OpenPanel = (NSOpenPanel *)Panel;
			  [OpenPanel setCanChooseFiles:true];
			  [OpenPanel setCanChooseDirectories:false];
			  [OpenPanel setAllowsMultipleSelection:MultipleFiles];
		  }

		  [Panel setCanCreateDirectories:bSave];

		  CFStringRef Title = FPlatformString::TCHARToCFString(*DialogTitle);
		  [Panel setTitle:(NSString *)Title];
		  CFRelease(Title);

		  CFStringRef DefaultPathCFString = FPlatformString::TCHARToCFString(*DefaultPath);
		  NSURL *DefaultPathURL = [NSURL fileURLWithPath:(NSString *)DefaultPathCFString];
		  [Panel setDirectoryURL:DefaultPathURL];
		  CFRelease(DefaultPathCFString);

		  CFStringRef FileNameCFString = FPlatformString::TCHARToCFString(*DefaultFile);
		  [Panel setNameFieldStringValue:(NSString *)FileNameCFString];
		  CFRelease(FileNameCFString);

		  FileDialogAccessoryView *AccessoryView = [[FileDialogAccessoryView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 250.0, 85.0) dialogPanel:Panel];
		  [Panel setAccessoryView:AccessoryView];

		  TArray<FString> FileTypesArray;
		  int32 NumFileTypes = FileTypes.ParseIntoArray(FileTypesArray, TEXT("|"), true);

		  NSMutableArray *AllowedFileTypes = [NSMutableArray arrayWithCapacity:NumFileTypes];

		  if (NumFileTypes > 0)
		  {
			  for (int32 Index = 0; Index < NumFileTypes; ++Index)
			  {
				  CFStringRef Type = FPlatformString::TCHARToCFString(*FileTypesArray[Index]);
				  [AllowedFileTypes addObject:(NSString *)Type];
				  CFRelease(Type);
			  }
		  }
		  if ([AllowedFileTypes count] == 0)
		  {
			  [AllowedFileTypes addObject:@"All files"];
			  [AllowedFileTypes addObject:@""];
		  }

		  if ([AllowedFileTypes count] == 1)
		  {
			  [AllowedFileTypes addObject:@""];
		  }

		  [AccessoryView AddAllowedFileTypes:AllowedFileTypes];

		  bool bOkPressed = false;
		  NSWindow *FocusWindow = [[NSApplication sharedApplication] keyWindow];

		  NSInteger Result = [Panel runModal];
		  [AccessoryView release];

		  if (Result == NSModalResponseOK)
		  {
			  if (bSave)
			  {
				  TCHAR FilePath[MAC_MAX_PATH];
				  FPlatformString::CFStringToTCHAR((CFStringRef)[[Panel URL] path], FilePath);
				  new (OutFilenames) FString(FilePath);
			  }
			  else
			  {
				  NSOpenPanel *OpenPanel = (NSOpenPanel *)Panel;
				  for (NSURL *FileURL in [OpenPanel URLs])
				  {
					  TCHAR FilePath[MAC_MAX_PATH];
					  FPlatformString::CFStringToTCHAR((CFStringRef)[FileURL path], FilePath);
					  new (OutFilenames) FString(FilePath);
				  }
				  OutFilterIndex = [AccessoryView SelectedExtension];
			  }

			  // Make sure all filenames gathered have their paths normalized
			  for (auto OutFilenameIt = OutFilenames.CreateIterator(); OutFilenameIt; ++OutFilenameIt)
			  {
				  FString &OutFilename = *OutFilenameIt;
				  OutFilename = IFileManager::Get().ConvertToRelativePath(*OutFilename);
				  FPaths::NormalizeFilename(OutFilename);
			  }

			  bOkPressed = true;
		  }

		  [Panel close];

		  if (FocusWindow)
		  {
			  [FocusWindow makeKeyWindow];
		  }

		  return bOkPressed;
		});
	}

	return bSuccess;
}
