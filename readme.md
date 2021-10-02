# Help game for NetHack Sokoban.

## Usage

```bash
./Sokoban.exe mapfile [actionfile]
```

## Key:
h/←: move left  
j/↓: move down  
k/↑: move up  
l/→: move right  
backspace: undo last action  
esc: exit game  

You can copy actions and save to a text file as actionfile.

## Preview
``` plantext
 ----          -----------
--..--------   |.........|
|..@.......|   |.........|
|.0-----0-.|   |.........|
|..|...|.0.|   |....<....|
|.0.0....0-|   |.........|
|.0..0..|..|   |.........|
|.----0.--.|   |.........|
|..0...0.|.--  |.........|
|.---0-...0.------------+|
|...|..0-.0.^^^^^^^^^^^^.|
|..0......----------------
-----..|..|
    -------
actions(00001/10000): d
```
