#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <Carbon/Carbon.h>

#include "daemon.h"
#include "parse.h"
#include "hotkey.h"
#include "sharedworkspace.h"

#define internal static
extern "C" bool CGSIsSecureEventInputSet();
#define IsSecureKeyboardEntryEnabled CGSIsSecureEventInputSet

internal CFMachPortRef KhdEventTap;
internal const char *KhdVersion = "0.0.4";

mode DefaultBindingMode = {};
mode *ActiveBindingMode = NULL;
uint32_t Compatibility = 0;
char *ConfigFile;
char *FocusedApp;

internal inline void
Error(const char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    vprintf(Format, Args);
    va_end(Args);

    exit(EXIT_FAILURE);
}

internal inline void
SetFocus(const char *Name)
{
    printf("Set focus '%s'\n", Name);
    if(FocusedApp)
    {
        free(FocusedApp);
        FocusedApp = NULL;
    }

    FocusedApp = strdup(Name);
}

internal CGEventRef
KeyCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Context)
{
    switch(Type)
    {
        case kCGEventTapDisabledByTimeout:
        case kCGEventTapDisabledByUserInput:
        {
            printf("restart event-tap\n");
            CGEventTapEnable(KhdEventTap, true);
        } break;
        case kCGEventKeyDown:
        {
            hotkey *Hotkey = NULL;
            if(HotkeyForCGEvent(Event, &Hotkey))
            {
                ExecuteHotkey(Hotkey);
                if(!HasFlags(Hotkey, Hotkey_Flag_Passthrough))
                    return NULL;
            }
        } break;
    }

    return Event;
}

internal inline void
ConfigureRunLoop()
{
    CGEventMask KhdEventMask = (1 << kCGEventKeyDown);
    KhdEventTap = CGEventTapCreate(kCGSessionEventTap,
                                   kCGHeadInsertEventTap,
                                   kCGEventTapOptionDefault,
                                   KhdEventMask,
                                   KeyCallback,
                                   NULL);

    if(!KhdEventTap || !CGEventTapIsEnabled(KhdEventTap))
        Error("Khd: Could not create event-tap, try running as root!\n");

    CFRunLoopAddSource(CFRunLoopGetMain(),
                       CFMachPortCreateRunLoopSource(kCFAllocatorDefault, KhdEventTap, 0),
                       kCFRunLoopCommonModes);
}

internal inline void
Init()
{
    if(!ConfigFile)
    {
        ConfigFile = strdup(".khdrc");
    }

    DefaultBindingMode.Name = strdup("default");
    ActiveBindingMode = &DefaultBindingMode;

    char *Contents = ReadFile(ConfigFile);
    if(Contents)
    {
        /* NOTE(koekeishiya): Callee frees memory. */
        ParseConfig(Contents);
    }
    else
    {
        Error("Khd: Could not open file '%s'\n", ConfigFile);
    }

    signal(SIGCHLD, SIG_IGN);
}

internal inline bool
ParseArguments(int Count, char **Args)
{
    int Option;
    const char *Short = "vc:e:w:p:";
    struct option Long[] =
    {
        { "version", no_argument, NULL, 'v' },
        { "config", required_argument, NULL, 'c' },
        { "emit", required_argument, NULL, 'e' },
        { "write", required_argument, NULL, 'w' },
        { "press", required_argument, NULL, 'p' },
        { NULL, 0, NULL, 0 }
    };

    while((Option = getopt_long(Count, Args, Short, Long, NULL)) != -1)
    {
        switch(Option)
        {
            case 'v':
            {
                printf("Khd Version %s\n", KhdVersion);
                return true;
            } break;
            case 'w':
            {
                SendKeySequence(optarg);
                return true;
            } break;
            case 'p':
            {
                SendKeyPress(optarg);
                return true;
            } break;
            case 'e':
            {
                int SockFD;
                if(ConnectToDaemon(&SockFD))
                {
                    WriteToSocket(optarg, SockFD);
                    CloseSocket(SockFD);
                    return true;
                }
                else
                {
                    Error("Could not connect to daemon! Terminating.\n");
                }
            } break;
            case 'c':
            {
                ConfigFile = strdup(optarg);
                printf("Khd: Using config '%s'\n", ConfigFile);
            } break;
        }
    }

    return false;
}

int main(int Count, char **Args)
{
    if(ParseArguments(Count, Args))
        return EXIT_SUCCESS;

    if(IsSecureKeyboardEntryEnabled())
        Error("Khd: Secure keyboard entry is enabled! Terminating.\n");

    if(!StartDaemon())
        Error("Khd: Could not start daemon! Terminating.\n");

    Init();
    SharedWorkspaceInitialize(SetFocus);
    ConfigureRunLoop();
    CFRunLoopRun();

    return EXIT_SUCCESS;
}
