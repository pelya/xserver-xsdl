//
//  Preferences.m
//
//  This class keeps track of the user preferences.
//
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/Preferences.m,v 1.2 2003/01/15 02:34:06 torrey Exp $ */

#import "Preferences.h"
#import "quartzCommon.h"
#include <IOKit/hidsystem/IOLLEvent.h>	// for modifier masks

// Macros to build the path name
#ifndef XBINDIR
#define XBINDIR /usr/X11R6/bin
#endif
#define STR(s) #s
#define XSTRPATH(s) STR(s)


@implementation Preferences

+ (void)initialize
{
    // Provide user defaults if needed
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithInt:0], @"Display",
        @"YES", @"FakeButtons",
        [NSNumber numberWithInt:NX_COMMANDMASK], @"Button2Mask",
        [NSNumber numberWithInt:NX_ALTERNATEMASK], @"Button3Mask",
        NSLocalizedString(@"USA.keymapping",@""), @"KeymappingFile",
        @"YES", @"UseKeymappingFile",
        NSLocalizedString(@"Cmd-Opt-a",@""), @"SwitchString",
        @"YES", @"UseRootlessMode",
        @"YES", @"UseAGLforGLX",
        @"YES", @"ShowModePickWindow",
        @"YES", @"ShowStartupHelp",
        [NSNumber numberWithInt:0], @"SwitchKeyCode",
        [NSNumber numberWithInt:(NSCommandKeyMask | NSAlternateKeyMask)],
        @"SwitchModifiers", @"NO", @"UseSystemBeep", 
        @"YES", @"DockSwitch", 
        @"NO", @"AllowMouseAccelChange",
        [NSNumber numberWithInt:qdCursor_Not8Bit], @"UseQDCursor",
        @"YES", @"Xinerama",
        @"YES", @"AddToPath",
        [NSString stringWithCString:XSTRPATH(XBINDIR)], @"AddToPathString",
        @"YES", @"UseDefaultShell",
        @"/bin/tcsh", @"Shell",
        [NSNumber numberWithInt:depth_Current], @"Depth", nil];

    [super initialize];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
}

// Initialize internal state info of switch key button
- (void)initSwitchKey
{
    keyCode = [Preferences keyCode];
    modifiers = [Preferences modifiers];
    [switchString setString:[Preferences switchString]];
}

- (id)init
{
    self = [super init];

    isGettingKeyCode=NO;
    switchString=[[NSMutableString alloc] init];
    [self initSwitchKey];

    return self;
}

// Set a modifiers checkbox matrix to match a modifier mask
- (void)resetMatrix:(NSMatrix *)aMatrix withMask:(int)aMask
{
    [aMatrix setState:(aMask & NX_SHIFTMASK)       atRow:0 column:0];
    [aMatrix setState:(aMask & NX_CONTROLMASK)     atRow:1 column:0];
    [aMatrix setState:(aMask & NX_COMMANDMASK)     atRow:2 column:0];
    [aMatrix setState:(aMask & NX_ALTERNATEMASK)   atRow:3 column:0];
    [aMatrix setState:(aMask & NX_SECONDARYFNMASK) atRow:4 column:0];
}

// Generate a modifiers mask from a modifiers checkbox matrix
- (int)getMaskFromMatrix:(NSMatrix *)aMatrix
{
    int theMask = 0;

    if ([[aMatrix cellAtRow:0 column:0] state])
        theMask |= NX_SHIFTMASK;
    if ([[aMatrix cellAtRow:1 column:0] state])
        theMask |= NX_CONTROLMASK;
    if ([[aMatrix cellAtRow:2 column:0] state])
        theMask |= NX_COMMANDMASK;
    if ([[aMatrix cellAtRow:3 column:0] state])
        theMask |= NX_ALTERNATEMASK;
    if ([[aMatrix cellAtRow:4 column:0] state])
        theMask |= NX_SECONDARYFNMASK;

    return theMask;
}

// Set the window controls to the state in user defaults
- (void)resetWindow
{
    if ([Preferences keymapFile] == nil)
        [keymapFileField setStringValue:@" "];
    else
        [keymapFileField setStringValue:[Preferences keymapFile]];

    if ([Preferences switchString] == nil)
        [switchKeyButton setTitle:@"--"];
    else
        [switchKeyButton setTitle:[Preferences switchString]];

    [displayField setIntValue:[Preferences display]];
    [dockSwitchButton setIntValue:[Preferences dockSwitch]];
    [fakeButton setIntValue:[Preferences fakeButtons]];
    [self resetMatrix:button2ModifiersMatrix
          withMask:[Preferences button2Mask]];
    [self resetMatrix:button3ModifiersMatrix
          withMask:[Preferences button3Mask]];
    [modeMatrix setState:[Preferences rootless] atRow:0 column:1];
    [startupHelpButton setIntValue:[Preferences startupHelp]];
    [modeWindowButton setIntValue:[Preferences modeWindow]];
    [systemBeepButton setIntValue:[Preferences systemBeep]];
    [mouseAccelChangeButton setIntValue:[Preferences mouseAccelChange]];
    [useXineramaButton setIntValue:[Preferences xinerama]];
    [addToPathButton setIntValue:[Preferences addToPath]];
    [addToPathField setStringValue:[Preferences addToPathString]];
    [useDefaultShellMatrix setState:![Preferences useDefaultShell]
                           atRow:1 column:0];
    [useOtherShellField setStringValue:[Preferences shellString]];
    [depthButton selectItemAtIndex:[Preferences depth]];
}

- (void)awakeFromNib
{
    [self resetWindow];
}

// Preference window delegate
- (void)windowWillClose:(NSNotification *)aNotification
{
    [self resetWindow];
    [self initSwitchKey];
}

// User cancelled the changes
- (IBAction)close:(id)sender
{
    [window orderOut:nil];
    [self resetWindow];  	// reset window controls
    [self initSwitchKey];	// reset switch key state
}

// Pick keymapping file
- (IBAction)pickFile:(id)sender
{
    int result;
    NSArray *fileTypes = [NSArray arrayWithObject:@"keymapping"];
    NSOpenPanel *oPanel = [NSOpenPanel openPanel];

    [oPanel setAllowsMultipleSelection:NO];
    result = [oPanel runModalForDirectory:@"/System/Library/Keyboards"
                     file:nil types:fileTypes];
    if (result == NSOKButton) {
        [keymapFileField setStringValue:[oPanel filename]];
    }
}

// User saved changes
- (IBAction)saveChanges:(id)sender
{
    [Preferences setKeyCode:keyCode];
    [Preferences setModifiers:modifiers];
    [Preferences setSwitchString:switchString];
    [Preferences setKeymapFile:[keymapFileField stringValue]];
    [Preferences setUseKeymapFile:YES];
    [Preferences setDisplay:[displayField intValue]];
    [Preferences setDockSwitch:[dockSwitchButton intValue]];
    [Preferences setFakeButtons:[fakeButton intValue]];
    [Preferences setButton2Mask:
                    [self getMaskFromMatrix:button2ModifiersMatrix]];
    [Preferences setButton3Mask:
                    [self getMaskFromMatrix:button3ModifiersMatrix]];
    [Preferences setRootless:[[modeMatrix cellAtRow:0 column:1] state]];
    [Preferences setModeWindow:[modeWindowButton intValue]];
    [Preferences setStartupHelp:[startupHelpButton intValue]];
    [Preferences setSystemBeep:[systemBeepButton intValue]];
    [Preferences setMouseAccelChange:[mouseAccelChangeButton intValue]];
    [Preferences setXinerama:[useXineramaButton intValue]];
    [Preferences setAddToPath:[addToPathButton intValue]];
    [Preferences setAddToPathString:[addToPathField stringValue]];
    [Preferences setUseDefaultShell:
                    [[useDefaultShellMatrix cellAtRow:0 column:0] state]];
    [Preferences setShellString:[useOtherShellField stringValue]];
    [Preferences setDepth:[depthButton indexOfSelectedItem]];
    [Preferences saveToDisk];

    [window orderOut:nil];
}

- (IBAction)setKey:(id)sender
{
    [switchKeyButton setTitle:NSLocalizedString(@"Press key",@"")];
    isGettingKeyCode=YES;
    [switchString setString:@""];
}

- (BOOL)sendEvent:(NSEvent*)anEvent
{
    if(isGettingKeyCode) {
        if([anEvent type]==NSKeyDown) // wait for keyup
            return YES;
        if([anEvent type]!=NSKeyUp)
            return NO;

        if([anEvent modifierFlags] & NSCommandKeyMask)
            [switchString appendString:@"Cmd-"];
        if([anEvent modifierFlags] & NSControlKeyMask)
            [switchString appendString:@"Ctrl-"];
        if([anEvent modifierFlags] & NSAlternateKeyMask)
            [switchString appendString:@"Opt-"];
        if([anEvent modifierFlags] & NSNumericPadKeyMask) // doesn't work
            [switchString appendString:@"Num-"];
        if([anEvent modifierFlags] & NSHelpKeyMask)
            [switchString appendString:@"Help-"];
        if([anEvent modifierFlags] & NSFunctionKeyMask) // powerbooks only
            [switchString appendString:@"Fn-"];
        
        [switchString appendString:[anEvent charactersIgnoringModifiers]];
        [switchKeyButton setTitle:switchString];
        
        keyCode = [anEvent keyCode];
        modifiers = [anEvent modifierFlags];
        isGettingKeyCode=NO;
        
        return YES;
    }
    return NO;
}

+ (void)setKeymapFile:(NSString*)newFile
{
    [[NSUserDefaults standardUserDefaults] setObject:newFile
            forKey:@"KeymappingFile"];
}

+ (void)setUseKeymapFile:(BOOL)newUseKeymapFile
{
    [[NSUserDefaults standardUserDefaults] setBool:newUseKeymapFile
            forKey:@"UseKeymappingFile"];
}

+ (void)setSwitchString:(NSString*)newString
{
    [[NSUserDefaults standardUserDefaults] setObject:newString
            forKey:@"SwitchString"];
}

+ (void)setKeyCode:(int)newKeyCode
{
    [[NSUserDefaults standardUserDefaults] setInteger:newKeyCode
            forKey:@"SwitchKeyCode"];
}

+ (void)setModifiers:(int)newModifiers
{
    [[NSUserDefaults standardUserDefaults] setInteger:newModifiers
            forKey:@"SwitchModifiers"];
}

+ (void)setDisplay:(int)newDisplay
{
    [[NSUserDefaults standardUserDefaults] setInteger:newDisplay
            forKey:@"Display"];
}

+ (void)setDockSwitch:(BOOL)newDockSwitch
{
    [[NSUserDefaults standardUserDefaults] setBool:newDockSwitch
            forKey:@"DockSwitch"];
}

+ (void)setFakeButtons:(BOOL)newFakeButtons
{
    [[NSUserDefaults standardUserDefaults] setBool:newFakeButtons
            forKey:@"FakeButtons"];
    // Update the setting used by the X server thread
    darwinFakeButtons = newFakeButtons;
}

+ (void)setButton2Mask:(int)newFakeMask
{
    [[NSUserDefaults standardUserDefaults] setInteger:newFakeMask
            forKey:@"Button2Mask"];
    // Update the setting used by the X server thread
    darwinFakeMouse2Mask = newFakeMask;
}

+ (void)setButton3Mask:(int)newFakeMask
{
    [[NSUserDefaults standardUserDefaults] setInteger:newFakeMask
            forKey:@"Button3Mask"];
    // Update the setting used by the X server thread
    darwinFakeMouse3Mask = newFakeMask;
}

+ (void)setMouseAccelChange:(BOOL)newMouseAccelChange
{
    [[NSUserDefaults standardUserDefaults] setBool:newMouseAccelChange
            forKey:@"AllowMouseAccelChange"];
    // Update the setting used by the X server thread
    quartzMouseAccelChange = newMouseAccelChange;
}

+ (void)setUseQDCursor:(int)newUseQDCursor
{
    [[NSUserDefaults standardUserDefaults] setInteger:newUseQDCursor
            forKey:@"UseQDCursor"];
}

+ (void)setModeWindow:(BOOL)newModeWindow
{
    [[NSUserDefaults standardUserDefaults] setBool:newModeWindow
            forKey:@"ShowModePickWindow"];
}

+ (void)setRootless:(BOOL)newRootless
{
    [[NSUserDefaults standardUserDefaults] setBool:newRootless
            forKey:@"UseRootlessMode"];
}

+ (void)setUseAGL:(BOOL)newUseAGL
{
    [[NSUserDefaults standardUserDefaults] setBool:newUseAGL
            forKey:@"UseAGLforGLX"];
}

+ (void)setStartupHelp:(BOOL)newStartupHelp
{
    [[NSUserDefaults standardUserDefaults] setBool:newStartupHelp
            forKey:@"ShowStartupHelp"];
}

+ (void)setSystemBeep:(BOOL)newSystemBeep
{
    [[NSUserDefaults standardUserDefaults] setBool:newSystemBeep
            forKey:@"UseSystemBeep"];
    // Update the setting used by the X server thread
    quartzUseSysBeep = newSystemBeep;
}

+ (void)setXinerama:(BOOL)newXinerama
{
    [[NSUserDefaults standardUserDefaults] setBool:newXinerama
            forKey:@"Xinerama"];
}

+ (void)setAddToPath:(BOOL)newAddToPath
{
    [[NSUserDefaults standardUserDefaults] setBool:newAddToPath
            forKey:@"AddToPath"];
}

+ (void)setAddToPathString:(NSString*)newAddToPathString
{
    [[NSUserDefaults standardUserDefaults] setObject:newAddToPathString
            forKey:@"AddToPathString"];
}

+ (void)setUseDefaultShell:(BOOL)newUseDefaultShell
{
    [[NSUserDefaults standardUserDefaults] setBool:newUseDefaultShell
            forKey:@"UseDefaultShell"];
}

+ (void)setShellString:(NSString*)newShellString
{
    [[NSUserDefaults standardUserDefaults] setObject:newShellString
            forKey:@"Shell"];
}

+ (void)setDepth:(int)newDepth
{
    [[NSUserDefaults standardUserDefaults] setInteger:newDepth
            forKey:@"Depth"];
}

+ (void)saveToDisk
{
    [[NSUserDefaults standardUserDefaults] synchronize];
}

+ (BOOL)useKeymapFile
{
    return [[NSUserDefaults standardUserDefaults]
                boolForKey:@"UseKeymappingFile"];
}

+ (NSString*)keymapFile
{
    return [[NSUserDefaults standardUserDefaults]
                stringForKey:@"KeymappingFile"];
}

+ (NSString*)switchString
{
    return [[NSUserDefaults standardUserDefaults]
                stringForKey:@"SwitchString"];
}

+ (unsigned int)keyCode
{
    return [[NSUserDefaults standardUserDefaults]
                integerForKey:@"SwitchKeyCode"];
}

+ (unsigned int)modifiers
{
    return [[NSUserDefaults standardUserDefaults]
                integerForKey:@"SwitchModifiers"];
}

+ (int)display
{
    return [[NSUserDefaults standardUserDefaults]
                integerForKey:@"Display"];
}

+ (BOOL)dockSwitch
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"DockSwitch"];
}

+ (BOOL)fakeButtons
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"FakeButtons"];
}

+ (int)button2Mask
{
    return [[NSUserDefaults standardUserDefaults]
                integerForKey:@"Button2Mask"];
}

+ (int)button3Mask
{
    return [[NSUserDefaults standardUserDefaults]
                integerForKey:@"Button3Mask"];
}

+ (BOOL)mouseAccelChange
{
    return [[NSUserDefaults standardUserDefaults]
                boolForKey:@"AllowMouseAccelChange"];
}

+ (int)useQDCursor
{
    return [[NSUserDefaults standardUserDefaults]
                integerForKey:@"UseQDCursor"];
}

+ (BOOL)rootless
{
    return [[NSUserDefaults standardUserDefaults]
                boolForKey:@"UseRootlessMode"];
}

+ (BOOL)useAGL
{
    return [[NSUserDefaults standardUserDefaults]
                boolForKey:@"UseAGLforGLX"];
}

+ (BOOL)modeWindow
{
    return [[NSUserDefaults standardUserDefaults]
                boolForKey:@"ShowModePickWindow"];
}

+ (BOOL)startupHelp
{
    return [[NSUserDefaults standardUserDefaults]
                boolForKey:@"ShowStartupHelp"];
}

+ (BOOL)systemBeep
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"UseSystemBeep"];
}

+ (BOOL)xinerama
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"Xinerama"];
}

+ (BOOL)addToPath
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"AddToPath"];
}

+ (NSString*)addToPathString
{
    return [[NSUserDefaults standardUserDefaults]
                stringForKey:@"AddToPathString"];
}

+ (BOOL)useDefaultShell
{
    return [[NSUserDefaults standardUserDefaults]
                boolForKey:@"UseDefaultShell"];
}

+ (NSString*)shellString
{
    return [[NSUserDefaults standardUserDefaults]
                stringForKey:@"Shell"];
}

+ (int)depth
{
    return [[NSUserDefaults standardUserDefaults]
                integerForKey:@"Depth"];
}


@end
