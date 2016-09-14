#include "helpers.h"
#include <string.h>

#define internal static
#define local_persist static

internal void
CheckPrefixTimeout()
{
    if(ActiveMode->Prefix)
    {
        kwm_time_point NewPrefixTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> Diff = NewPrefixTime - KWMHotkeys.ActiveMode->Time;
        if(Diff.count() > ActiveMode->Timeout)
        {
            DEBUG("Prefix timeout expired. Switching to mode " << ActiveMode->Restore);
            ActivateBindingMode(ActiveMode->Restore);
        }
    }
}

void ActivateBindingMode(std::string Mode)
{
    mode *BindingMode = GetBindingMode(Mode);
    if(!DoesBindingModeExist(Mode))
        BindingMode = GetBindingMode("default");

    ActiveMode = BindingMode;
    UpdateBorder(&FocusedBorder, FocusedApplication->Focus);
    if(BindingMode->Prefix)
    {
        BindingMode->Time = std::chrono::steady_clock::now();
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, BindingMode->Timeout * NSEC_PER_SEC), dispatch_get_main_queue(),
        ^{
            CheckPrefixTimeout();
        });
    }
}

void EmitText(char *Text)
{
    CFStringRef TextRef = CFStringCreateWithCString(NULL, Text, kCFStringEncodingMacRoman);
    CGEventRef KeyDownEvent = CGEventCreateKeyboardEvent(NULL, 0, true);
    CGEventRef KeyUpEvent = CGEventCreateKeyboardEvent(NULL, 0, false);

    UniChar OutputBuffer;
    for(size_t Index = 0;
        *Text != '\0';
        ++Index, ++Text)
    {
        CFStringGetCharacters(TextRef, CFRangeMake(Index, 1), &OutputBuffer);

        CGEventSetFlags(KeyDownEvent, 0);
        CGEventKeyboardSetUnicodeString(KeyDownEvent, 1, &OutputBuffer);
        CGEventPost(kCGHIDEventTap, KeyDownEvent);

        CGEventSetFlags(KeyUpEvent, 0);
        CGEventKeyboardSetUnicodeString(KeyUpEvent, 1, &OutputBuffer);
        CGEventPost(kCGHIDEventTap, KeyUpEvent);
    }

    CFRelease(KeyUpEvent);
    CFRelease(KeyDownEvent);
    CFRelease(TextRef);
}
