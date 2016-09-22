**Khd**, Simple event-tap hotkey daemon for OSX.

See [sample config](https://github.com/koekeishiya/khd/blob/master/examples/khdrc) for syntax information.

See [#1](https://github.com/koekeishiya/khd/issues/1) for information regarding keycodes.

Arguments:
```
-v | --version: Print version number to stdout
    khd -v

-c | --config: Specify location of config file
    khd -c ~/.khdrc

-e | --emit: Emit text to an already running instance of Khd
    khd -e "mode activate default"

-w | --write: Simulate a sequence of keystrokes
    khd -w "this text will be printed by generating key-up/down events"

-p | --press: Simulate a keypress (parsed just like <keysym>)
    khd -p "cmd - right"
```

Interactive commands:
```
# activate a mode
khd -e "mode activate my_mode"

# reload config
khd -e "reload"
```

Customize mode:
```
# enable prefix mode
khd mode my_mode prefix on

# specify prefix timeout
khd mode my_mode timeout 0.75

# activate mode on timeout (defaults to 'default')
khd mode my_mode restore some_other_mode

# specify color for this mode (kwm compatibility mode)
khd mode my_mode color 0xAARRGGBB
```

Compatibility with Kwm:
```
# set color of focused border to color of active mode
khd kwm on
```
