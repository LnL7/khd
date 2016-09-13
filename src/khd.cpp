#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <getopt.h>
#include <string.h>
#include <Carbon/Carbon.h>

#include "parse.h"
#include "hotkey.h"

#define internal static
extern "C" bool CGSIsSecureEventInputSet();
#define IsSecureKeyboardEntryEnabled CGSIsSecureEventInputSet

internal CFMachPortRef KhdEventTap;

internal const char *KhdVersion = "0.0.0";
internal char *ConfigFile;

mode DefaultBindingMode = {};
mode *ActiveBindingMode = NULL;

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
            CGEventTapEnable(KhdEventTap, true);
        } break;
        case kCGEventKeyDown:
        {
            printf("Khd: Key down\n");
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
    DefaultBindingMode.Color = 0xddd5c4a1;
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
}

internal inline bool
ParseArguments(int Count, char **Args)
{
    int Option;
    const char *Short = "vc:";
    struct option Long[] =
    {
        { "version", no_argument, NULL, 'v' },
        { "config", required_argument, NULL, 'c' },
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

    Init();
    ConfigureRunLoop();
    CFRunLoopRun();

    return EXIT_SUCCESS;
}
