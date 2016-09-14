#ifndef KHD_EVENT_H
#define KHD_EVENT_H

#include <pthread.h>
#include <queue>

struct event;
#define EVENT_CALLBACK(name) void name(event *Event)
typedef EVENT_CALLBACK(EventCallback);

extern EVENT_CALLBACK(Callback_Event_Hotkey);

enum event_type
{
    Event_Hotkey,
};

struct event
{
    EventCallback *Handle;
    void *Context;
};

struct event_loop
{
    pthread_cond_t State;
    pthread_mutex_t StateLock;
    pthread_mutex_t WorkerLock;
    pthread_t Worker;

    bool Running;
    std::queue<event> Queue;
};

bool StartEventLoop();
void StopEventLoop();
void AddEvent(event Event);

#define ConstructEvent(EventType, EventContext) \
    do { event Event = {}; \
         Event.Context = EventContext; \
         Event.Handle = &Callback_##EventType; \
         AddEvent(Event); \
       } while(0)

#endif
