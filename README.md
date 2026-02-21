# Text editor made for fun

## About The Project
I always wanted to explore what it would be like to create text editor from scratch, so I finally decided to do it. It is not, by any means, a fully fledgededitor. Just made for fun to see if I could do it or not and learn more about C.

### Built With
For this project I tried to use very few external libraries. They are:
- raylib for gui
- clay ui library to create simple layout
- clay's own renderer for raylib
- tinyfiledialogs to create dialog menu's for saving and opening files

### Current Features
- Gutter line numbers 
- Blinking cursor
- Move cursor with arrows
- Jump cursor through text with Ctrl+ArrowKey's combo
- Copy, paste, cut with Ctrl+C, Ctrl+V, Ctrl+X respectively
- Highlight all text with Ctrl+A
- Highlight words with Ctrl+Shift+ArrowKey's combo
- Highlight text with mouse
- Scroll + scrollbar
- Simple syntax highlighting for basic keywords (int, void, char, long, float, etc.)
- Open file with Ctrl+O combo, drag and drop or through open file dialog
- Save file with Ctrl+S combo or throught save file dialog
- Window title shows unsaved state and file name if opened
- Ability to detect if opened file was changed externally and reload it

! [Editor screenshot](resources/screenshot.pn)
