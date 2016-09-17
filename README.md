## Khd

Simple event-tap hotkey daemon for OSX.

See [sample config](https://github.com/koekeishiya/khd/blob/master/examples/khdrc) for syntax information.
See [#1](https://github.com/koekeishiya/khd/issues/1) for information regarding keycodes.

Possible launch arguments:

'-v': Print version number to stdout
```
    khd -v
```

'-c': Specify location of config file
```
    khd -c ~/.khdrc
```

'-e': Emit text to an already running instance of Khd
```
    khd -e "mode activate default"
```

'-w': Simulate a sequence of keystrokes
```
    khd -w "this text will be printed by generating key-up/down events"
```

'-p': Simulate a keypress (parsed just like <keysym>)
```
    khd -p "cmd - right"
```
