#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <Carbon/Carbon.h>

#include "daemon.h"
#include "parse.h"
#include "hotkey.h"
#include "sharedworkspace.h"

#define internal static
extern "C" bool CGSIsSecureEventInputSet();
#define IsSecureKeyboardEntryEnabled CGSIsSecureEventInputSet

internal CFMachPortRef KhdEventTap;
internal const char *KhdVersion = "1.1.0";

mode DefaultBindingMode = {};
mode *ActiveBindingMode = NULL;
uint32_t Compatibility = 0;
pthread_mutex_t Lock;
char *ConfigFile;
char *FocusedApp;

hotkey Modifiers = {};
long long ModifierTriggerTime;
double ModifierTriggerTimeout;

internal inline void
Error(const char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    vfprintf(stderr, Format, Args);
    va_end(Args);

    exit(EXIT_FAILURE);
}

internal inline void
SetFocus(const char *Name)
{
    pthread_mutex_lock(&Lock);
    if(FocusedApp)
    {
        free(FocusedApp);
        FocusedApp = NULL;
    }

    if(Name)
    {
        FocusedApp = strdup(Name);
    }

    pthread_mutex_unlock(&Lock);
}

internal CGEventRef
KeyCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Context)
{
    switch(Type)
    {
        case kCGEventTapDisabledByTimeout:
        case kCGEventTapDisabledByUserInput:
        {
            printf("Khd: Restarting event-tap\n");
            CGEventTapEnable(KhdEventTap, true);
        } break;
        case kCGEventKeyDown:
        {
            CGEventFlags Flags = CGEventGetFlags(Event);
            CGKeyCode Key = CGEventGetIntegerValueField(Event, kCGKeyboardEventKeycode);

            hotkey *Hotkey = NULL;
            if(HotkeyForCGEvent(Flags, Key, &Hotkey, true))
            {
                if((ExecuteHotkey(Hotkey)) &&
                   (!HasFlags(Hotkey, Hotkey_Flag_Passthrough)))
                {
                    return NULL;
                }
            }
        } break;
        case kCGEventFlagsChanged:
        {
            CGEventFlags Flags = CGEventGetFlags(Event);
            CGKeyCode Key = CGEventGetIntegerValueField(Event, kCGKeyboardEventKeycode);
            RefreshModifierState(Flags, Key);
        } break;
    }

    return Event;
}

internal inline void
ConfigureRunLoop()
{
    CGEventMask KhdEventMask = (1 << kCGEventKeyDown) |
                               (1 << kCGEventFlagsChanged);
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
        char *Home = getenv("HOME");
        if(Home)
        {
            int Length = strlen(Home) + strlen("/.khdrc");
            ConfigFile = (char *) malloc(Length + 1);
            strcpy(ConfigFile, Home);
            strcat(ConfigFile, "/.khdrc");
        }
        else
        {
            ConfigFile = strdup(".khdrc");
        }
    }

    DefaultBindingMode.Name = strdup("default");
    ActiveBindingMode = &DefaultBindingMode;
    ModifierTriggerTimeout = 0.1;

    printf("Khd: Using config '%s'\n", ConfigFile);
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

    if(pthread_mutex_init(&Lock, NULL) != 0)
    {
        Error("Khd: Could not create mutex");
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
            } break;
        }
    }

    return false;
}

internal inline bool
CheckPrivileges()
{
    bool Result = false;
    const void *Keys[] = { kAXTrustedCheckOptionPrompt };
    const void *Values[] = { kCFBooleanTrue };

    CFDictionaryRef Options;
    Options = CFDictionaryCreate(kCFAllocatorDefault,
                                 Keys, Values, sizeof(Keys) / sizeof(*Keys),
                                 &kCFCopyStringDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);

    Result = AXIsProcessTrustedWithOptions(Options);
    CFRelease(Options);

    return Result;
}

int main(int Count, char **Args)
{
    if(ParseArguments(Count, Args))
        return EXIT_SUCCESS;

    if(IsSecureKeyboardEntryEnabled())
        Error("Khd: Secure keyboard entry is enabled! Terminating.\n");

    if(getuid() != 0 && !CheckPrivileges())
        Error("Khd: Must be run with accessibility access, or as root.\n");

    if(!StartDaemon())
        Error("Khd: Could not start daemon! Terminating.\n");

    Init();
    SharedWorkspaceInitialize(SetFocus);
    ConfigureRunLoop();
    CFRunLoopRun();

    return EXIT_SUCCESS;
}
