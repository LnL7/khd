#import <Cocoa/Cocoa.h>
#import "sharedworkspace.h"
#define internal static

@interface WorkspaceWatcher : NSObject {
}
- (id)init;
@end

internal WorkspaceWatcher *Watcher;
internal void (*SetFocusedApplication)(const char *Name);

void SharedWorkspaceInitialize(void (*Func)(const char *))
{
    Watcher = [[WorkspaceWatcher alloc] init];
    SetFocusedApplication = Func;

    const char *Name = [[[[NSWorkspace sharedWorkspace] frontmostApplication] localizedName] UTF8String];
    (*SetFocusedApplication)(Name);
}

@implementation WorkspaceWatcher
- (id)init
{
    if ((self = [super init]))
    {
       [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                selector:@selector(didActivateApplication:)
                name:NSWorkspaceDidActivateApplicationNotification
                object:nil];
    }

    return self;
}

- (void)dealloc
{
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
    [super dealloc];
}

- (void)didActivateApplication:(NSNotification *)notification
{
    const char *Name = [[[notification.userInfo objectForKey:NSWorkspaceApplicationKey] localizedName] UTF8String];
    (*SetFocusedApplication)(Name);
}

@end
