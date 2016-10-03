## Description [![Build Status](https://travis-ci.org/koekeishiya/khd.svg?branch=master)](https://travis-ci.org/koekeishiya/khd)

**Khd** is a simple modal hotkey-daemon for OSX, using [Quartz Event Services](https://developer.apple.com/reference/coregraphics/1658572-quartz_event_services?language=objc).

**Khd** must have access to the *Accessibility API*, or be ran as root.

[Secure Keyboard Entry](https://github.com/koekeishiya/khd/issues/7) must be disabled for **Khd** to receive key-events.

See [sample config](https://github.com/koekeishiya/khd/blob/master/examples/khdrc) for syntax information.

See [#1](https://github.com/koekeishiya/khd/issues/1) for information regarding keycodes.

## Install

A codesigned binary release is available through Homebrew

      brew install koekeishiya/khd/khd

Manage *Khd* using brew services

      brew services start khd

Location of logs

      stdout -> /tmp/khd.out
      stderr -> /tmp/khd.err

## Usage

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

## Configuration

**Khd** will load the configuration file `$HOME/.khdrc`, unless otherwise specified.

See [sample config](https://github.com/koekeishiya/khd/blob/master/examples/khdrc) for syntax information.

See [#1](https://github.com/koekeishiya/khd/issues/1) for information regarding keycodes.

Customize mode:
```
# modifier only binds consist of the sequence mod_pressed -> mod_released
# and must be performed within a given timeframe.
# the following specifies the timeout (in seconds) to be used.
khd mod_trigger_timeout 0.1

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

## Development

*Khd* does not use any C++ features, except for function overloading and implicit struct typedefs.

There is also a tiny bit of objective-c code, to expose required functionality from NSWorkspace.

Build *Khd*

      make install      # release version
      make              # debug version

