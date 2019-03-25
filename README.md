# Sandio Key Mapper
This is a custom key mapping program for the abandoned **Sandio 3D Game O2** mouse.

![Mouse Photo](sandio1.jpg)

It watches the positions of the three "3D Buttons" (which are physically very much like joystick "hats") and emits configurable keystrokes for games and other applications.

Unlike the original key mapping software that came with the mouse, this program can sense and map diagonal positions as well as arbitrary combinations of positions AKA "gestures".
However, it lacks the GUI and gloss of the original software.
It is based on the SDK that came with the mouse driver, but does not require that driver to be installed.

# App Window
The application window displays the status of the 3D buttons and the information about the most recent keystroke (when the app window was in focus, physical or simulated by the program) for reference and testing.

# Configuration files
[Sample Config File](config.txt)
## Location
By default the program reads key mapping configuration from the file "config.txt" in its directory.  An alternate config file can be specified on the command line (including using a custom Windows shortcut).
## Simple Syntax
On each line (not counting comments and blank lines) list a button, the direction it should be tilted, and a keystroke to emit when that happens, like this:

```
TopForward: W    # emulate pressing W when the top button is tilted forward
LeftDown: Left   # emulate pressing the left arrow when the left button is tilted down
```

* `#` begins a comment.
* The valid button names are `Top`, `Right`, and `Left`.
* The directions are `Forward`, `Back`, `Left`, `Right`, `Up`, and `Down`.
* These are case-insensitive, can be abbreviated with just the first letter e.g. `T`, and some synomyms like `backwards` also work.
* There should be no space between the button and the direction, e.g. **not** `Top Forward`.
* The word following the colon (including another colon) specifies the key to emit.
* In additioin to typographical characters, the key can be any of the following:
  * `Space`, `Enter`
  * `Up`, `Down`, `Left`, `Right`
  * `Home`, `End`, `PageUp`, `PageDown`
  * `NumPad0` through `NumPad9`
  * `Insert`, `Delete`, `NumPad+`, `NumPad-`, `NumPad/`, `NumPad*`
  * `F1` through `F24`
  * Some common synonyms of the above like `PgDn` and `num5`
  * A comma-separated two-tuple or three-tuple containing the Virtual Key Code, Scan Code, and optional Flags, explicitly defining an arbitrary keystroke.  These codes may be keyboard-dependent; you can observe them in the program window while pressing keys on the keyboard.
    * `37,75` or `37,75,0` emits left arrow on the numeric keypad (on my keyboard)
    * `37,75,1` emits left arrow on the arrows pad
    * The numbers can be specified in decimal, hexadecimal, or octal just like C numeric literals.

## Compounding Syntax
* Each word on the left side of the colon has a button followed by one or more directions.
  * `TopForwardRight: E` emits E when the top button is tilted diagonally forward and right.
* A `!` (exclamation point) in a word means the button must **not** be tilted in the subsequent directions.
  * `TopForward!LeftRight: W`, or `TF!LR:W` for short, defines a mapping for the strictly-forward direction (not diagonal forward-and-left nor forward-and-right)
* All listed conditions must be met in order for the mapping to trigger.
  * `LeftUp RightDown: NumPad+` triggers on a gesture where the left button is tilted up while the right button is tilted down.
* Mappings are triggered in the order they are listed.  The following rules will type "cat" whenever the top button is tilted back on the mouse:
  * ```
    TB: C
    TB: A
    TB: T
    ```
