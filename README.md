## Map_Exchange_String_Decoder
Factorio Map Exchange String Decoder (C++).

### Usage: 
    Map_Exchange_String_Decoder.exe input.txt MapGenSettings.json MapSettings.json
    where:
      input.txt           - a file that contains "Map Exchange String"
      MapGenSettings.json - output file
      MapSettings.json    - output file
      
### Building (Win)
    Install MSYS2.
    Install packages in MSYS2:
      pacman -S mingw-w64-x86_64-gcc
      pacman -S mingw-w64-x86_64-make
      pacman -S mingw-w64-x86_64-gdb
      pacman -S mingw-w64-x86_64-zlib
    In the MSYS2 shell, run:
      cd "A:/some kind of folder/source_directory"
      make -f makefile.gcc clear
      make -f makefile.gcc
