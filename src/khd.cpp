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
#include "event.h"

#define internal static
extern "C" bool CGSIsSecureEventInputSet();
#define IsSecureKeyboardEntryEnabled CGSIsSecureEventInputSet

internal CFMachPortRef KhdEventTap;
internal const char *KhdVersion = "0.0.0";
internal char *ConfigFile;

mode DefaultBindingMode = {};
mode *ActiveBindingMode = NULL;
uint32_t Compatibility = 0;

internal inline void
Error(const char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    vprintf(Format, Args);
    va_end(Args);

    exit(EXIT_FAILURE);
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
            hotkey *Hotkey = (hotkey *) malloc(sizeof(hotkey));
            if(Hotkey)
            {
                if(HotkeyForCGEvent(Event, Hotkey))
                {
                    printf("matched hotkey\n");
                    bool Passthrough = HasFlags(Hotkey, Hotkey_Flag_Passthrough);
                    ConstructEvent(Event_Hotkey, Hotkey);
                    if(!Passthrough)
                        return NULL;
                }
                else
                {
                    printf("key pressed\n");
                    free(Hotkey);
                }
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
    {
        Error("Khd: Could not create event-tap, try running as root!\n");
    }

    CFRunLoopAddSource(CFRunLoopGetMain(),
                       CFMachPortCreateRunLoopSource(kCFAllocatorDefault, KhdEventTap, 0),
                       kCFRunLoopCommonModes);
}

internal inline char *
ReadFile(const char *File)
{
    char *Contents = NULL;
    FILE *Descriptor = fopen(File, "r");

    if(Descriptor)
    {
        fseek(Descriptor, 0, SEEK_END);
        unsigned int Length = ftell(Descriptor);
        fseek(Descriptor, 0, SEEK_SET);

        Contents = (char *) malloc(Length + 1);
        fread(Contents, Length, 1, Descriptor);
        Contents[Length] = '\0';

        fclose(Descriptor);
    }

    return Contents;
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
    const char *Short = "vc:e:";
    struct option Long[] =
    {
        { "version", no_argument, NULL, 'v' },
        { "config", required_argument, NULL, 'c' },
        { "emit", required_argument, NULL, 'e' },
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
            case 'c':
            {
                ConfigFile = strdup(optarg);
                printf("Khd: Using config '%s'\n", ConfigFile);
            } break;
            case 'e':
            {
                int SockFD;
                if(ConnectToDaemon(&SockFD))
                {
                    char *Emit = strdup(optarg);
                    printf("emitting '%s'\n", Emit);
                    WriteToSocket(Emit, SockFD);
                    CloseSocket(SockFD);
                    free(Emit);
                    exit(EXIT_SUCCESS);
                }
                else
                {
                    Error("Could not connect to daemon! Terminating.\n");
                }
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
    StartEventLoop();
    ConfigureRunLoop();
    CFRunLoopRun();

    return EXIT_SUCCESS;
}
