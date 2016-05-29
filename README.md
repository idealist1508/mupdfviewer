PDF/XPS/CBZ reader based on muPDF library for Pocketbook Pro 912
Based on work of Denis Dadimov <denisdadimov@mail.ru> and <damiano.rullo@gmail.com>

## Features

- good performance due to muPDF and combination of page caching/background rendering
- improved rendering quality
- selective viewing of text only/text+vector graphics/text+vector graphics+raster graphics
- improved bookmarking
- gestures support for touch devices

## Installation

1. Copy system/bin/mupdfview.app to system/bin
2. Copy system/bin/extensions.cfg to system/config OR append contents if there is one already
3. Reboot device

## Building
##### Compile MuPDF
- Download MuPDF 1.2
- Add pocketbook section to Makerules file:

```
ifeq "$(OS)" "pocketbook"
CC = arm-none-linux-gnueabi-gcc
LD = arm-none-linux-gnueabi-gcc
AR = arm-none-linux-gnueabi-ar
CFLAGS += -s -fsigned-char -Wno-format-y2k -Wno-unused-parameter
CROSSCOMPILE=yes
NOX11=yes
endif
```
- Debug build for emulator: `make` - creates mupdf/build/debug folder
- Release build: `make OS=pocketbook build=release` - creates mupdf/build/release

##### Compile app
build-scripts makepc.sh and makearm.sh assume that pdfview folder contains symlinks fwlib, mupdf, mupdfd (->mupdf/build/debug), mupdfr (->mupdf/build/release), sdk (->FRSCSDK)

## Short usage guide

### 1. Tool menu.

On large screen models (>800 pix wide) main functions are summarized on the top toolbar. On all models main menu can be invoked by pressing `[OK]` button OR touching around screen center. 

You can exit menu without selecting any function by pressing `[Back]` button OR top/left menu `[X]` button.

You can exit reader by pressing `[Back]` button while not in main menu OR top/right menu `[X]` button and select "yes" on confirmation dialog.

### 2. Navigation.

* Single page forward/backward:
  - `[Next]`/`[Prev]` hardware buttons (short click)
  - `[Left]`/`[Right]` hardware buttons
  - Left/Right swipe gesture anywhere on a page
* Multiple pages forward/backward:
  - `[Next]`/`[Prev]` hardware buttons (long press) - once you press and hold button, indicator in the top/left corner shows how many pages will be skipped when you release button
* You can navigate to arbitrary page using tool menu:
  - invoke menu if necessary
  - you can navigate to first/last page using `[|<]` and `[>|]` buttons
  - dial page number using 1-9 buttons (watch top toolbar for what you are dialing) and click `[V]` button. You can amend dialed page number by pressing `[<]` buton to erase last dialed digit.
* You can open table of contents by pressing `[C]` button in tool menu or designated hardware button
* Current page / Number of pages is displayed on the top toolbar

### 3. View mode.
##### Rotated view
 
The page is rotated 90o clockwise and zoomed in. You can switch rotated view on/off by pressing `[R]` menu button or `[Zoom in]` hardware button. While in rotated view, you can scroll page using `[Left]`/`[Right]` hardware buttons (they will look now as up/down buttons) or left / right swipe gesture anywhere on a page. When you reach page bottom/top, next/prev page will show up.

##### Margins cropping

Margins are calculated automatically to fit page contents. You can switch cropping on/off by pressing `[M]` menu button.

##### Selective content

You can selectively switch off raster and vector graphics on the page by choosing view mode. This can be useful to speed up page rendering by removing unnecessary (especially raster images) elements:
* `[V3]` menu button: all page contents are displayed
* `[V2]` menu button: text/vector graphics are displayed
* `[V1]` menu button: only text is displayed. All text is made pure black.

### 4. Bookmarks.

Bookmarks allow user to mark selected pages and quickly switch between them. Bookmarked page numbers are listed at the left bar. You can mark current page by:
- clicking empty bookmark on touch or stylus models
- navigate to the empty bookmark using `[Up]`/`[Down]` hardware buttons and pressing `[OK]` button. You can cancel bookmark selection by pressing `[Back]` button
- if there are no empty bookmarks and you want to override one of existing bookmarks, invoke main menu, click `[B+]` button then select the bookmark you'd like to to overwrite

You can goto bookmarked page by clicking it or selecting it using `[Up]`/`[Down]`/`[OK]` buttons.

You can unmark current page by clicking it's bookmark (it's highlighted).

You can remove any bookmark by selecting `[B-]` menu button and then unwanted bookmark.

You can hide bookmarks bar if not needed by toggling `[B]` menu button

### 5. Book state

When you close book, it's reading state (last page, view mode, bookmarks) are stored in the associated plain text file with "toc" extension.  Also, this file may contain table of contents which you can prepare or modify manually.
Table of contents is stored in the section starting with line `TOC<tab><number of TOC lines>` followed by lines `<entry level><tab><page num><tab><entry description>`

### 6. Settings

You can open settings using `[S]` menu button. Currently, there are following options:

##### Full Update Interval

Number of pages between full page update. 
  - `0` means no full updates wanted, 
  - `1` - every page is fully updated, etc.

This is a controversial issue. Ideally it should work like this: fast
partial page updates cause "ghost" images to accumulate and gradually
clutter page image. When slow full update is performed, all ghosts should
disappear leaving crystal clear image. Unfortunately, there is a problem
of "fading" screens like mine which constitute about 25% of all devices
which behave poorly in full update. In fact, on my device full update leaves
dirty mess with hardly readable text, while normal partial updates provide
strong black text. So on devices like this you should set this interval to 0.

##### Hardware margins

On some devices part of the screen around the edge is hidden under bezel, so this setting effectively shrinks screen area to make all the text visible. Experiment with your device for best result

##### Cache size

reduce this to consume less memory. Each read page image is stored in a cache of fixed length just in case you may come back in the nearest future. As you read pages, old pages are discarded from this cached.  The cache is priority-sorted, so that frequently viewed pages appear at the top of the cache and are discarded later than less frequently used pages. Each cached page consumes less than 500K, so 10-20 pages should not cause any problems.

### 7. Other functionality

* On 9xx models you can force full page update anytime using `[Zoom out]`
button
* Top tool bar displays status info: battery %, temperatur, time, and
memory allocated to reader 

