# **kGUI** is a cross platform c++ framework library. #

It mainly consists of a extensible cross platform gui. It also has many frequently used classes like hash tables, bsps, asynchronous http download, threading, xml reader/writer, an html renderer, a report generator for printing. It currently compiles and runs on Windows using Visual Studio or MSYS/MINGW. It also compiles and runs on both the Mac and Linux using the Gnu compiler. The framework is designed so that your application doesn't contain any platform specific code as the platform differences are all handled by the framework.

## NOTE: The lastest version of the source is in the SVN repository, the zip file from downloads is not up to date. ##

**I've just started a Wiki Page for [HowToBuild](HowToBuild.md)**

### If you would like to join this project, report a bug or make a comment please so do on my blog page ###

**See below for a list of current features and future plans**

List of projects that use kGUI:

## GPSTurbo - open source project for Geocaching ##
http://code.google.com/p/gpsturbo/
![http://gpsturbo.googlecode.com/svn/gpsturbo.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo.jpg)

## 'Up the Down Market' Fundraising Dinner Stock Trading simulation program, for the Downs Syndrome Research Foundation. ##

This program runs on 10 laptops. One is the Game machine, one is the Graphics machine and is connected to the two large screens. The remaining 8 machines are for trade input. The program uses a **mysql** database to keep track of the transactions and results. Each round consists of printing out order tickets for each table (usually 40-50) then sending these out to the tables. Then the tickets are collected and the trades are input using the 8 input computers. Once all the trades have been input, the the Game machine does the round calculations. There are then two reports generated for each table, an individual results report showing their status and a group report showing all the tables. The Graphics machine then shows multiple results graphs on the big screens. Then the next trading round opens and order tickets are printed for the new round. The game plays for a variable number of rounds, typically 5 or 6 until the winner is declared at the end of the game.

http://www.dsrf.org/index.cfm?fuseaction=events.main

![http://www.scale18.com/updown.jpg](http://www.scale18.com/updown.jpg)

## Browse - One of the kgui Sample programs ##
This is a tabbed web browser. It does not do Javascript yet and at this time doesn't pass any of the ACID tests but is a work in progress that I started so I could learn HTML and CSS. It does have a lot of error reporting built in so you can see errors on pages. You can also see full page headers and post data for pages. It also has a color-blind simulator mode where you can see what your pages look like to color-blind people.

![http://wpanel.googlecode.com/svn/branches/browse.jpg](http://wpanel.googlecode.com/svn/branches/browse.jpg)

## Current Features ##
  * multiple overlapping windows, keyboard and mouse input support
  * gui items currently include: windows, buttons, tick boxes, radio boxes, listboxes, tables, tab object, popup menus, scrollbars, inputboxes, dividers, scroll grid, scroll text, static text, rich text, auto layout object
  * draw primitives include text, rotated text, rectangles, boxes, lines, circles, scaled images, polygons
  * popup windows for input requestor, messages box requestor, file load / save requestor, busy bar
  * full hiearchical menus
  * images: jpeg, png, gif, animated gif, ico
  * movies: all formats that are supported by ffmpeg
  * all windows and gui items can have attached event callbacks
  * all gui items can be printed ( and print previewed ) using the report class
  * customizeable skinning for gui objects
  * string class that handles multiple encodings, ISO-8859-1, UTF-8,....
  * hash table class for strings, case sensitive, non-cs and fixed len binary index
  * heap class for quick memory allocation
  * date class for handling dates in local and GMT and converting to / from text
  * directory class for reading filenames in directories
  * data class for handling data from files, bigfiles and memory based files.
  * random number class
  * bit stream class for reading variable bits from a stream
  * bigfile handling, bigfiles can contain many files and all objects images, movies etc can be inside of bigfiles.
  * thread handling class
  * communication stack class for messaging between threads
  * mutex class
  * basic language class for use in apps as a scripting language.
  * download html class, handles redirects, authentication, cookies, both sync and asynchronous
  * web browser object and html renderer object, browser has extensive tag and css error reporting so you can use it to debug you down webpages
  * email send / receive class
  * bsp class
  * csv file class for loading / saving csv files
  * xml file class for loading / saving xml files
  * database connection class for connecting to a mysql database
  * encryption class for encrypting private data
  * standardize multi language support for kgui and apps
  * traveling salesman class for optimal pathfinding
  * file wrapper class for fileexists, deletefile, filesize, makedir
  * **secure** mode for html download class, NOTE: the default SLL class ( if enabled ) is MatrixSSL, it is GPL'ed and not LGPL'ed. The project is designed so a different SSL handler ( if desired ) can be used easily in place of MatrixSSL. Any commerical project would need to either use a different SSL class or get a licence from MatrixSSL

## Known problems / Issues ##
  * HTML renderer layout engine still needs work, also not fully CSS3 compliant.
  * HTML renderer needs to handle multiple fonts ( currently only uses 1 default font)
  * Browser needs to handle selecting / copying, also 'Save As for images/objects'
  * Printing on Linux/Mac uses postscript files. Characters with accents and other special characters are not printing also non-standard fonts are not working either.
  * Numeric pad on Linux not working
  * Linux / Mac Need to get page sizes for printers using CUPS
  * Doesn't set app Icon on Gnome / KDE taskbar
  * Clipboard in Linux not being handled properly when copying to a different app

## Planned Features ##
  * update code comments for doxygen auto doc generation
  * add javascript to browser
  * need to run program through valgrind to look for problems and optimization issues.
  * pdf renderer object class
  * postscript renderer class
  * automated crash tracking and reporting
  * update bigfiles to handle >4GB files
  * add strcmp member to string class to handle comparing 2 strings with different encoding
  * look into making kgui work with various PDA operating systems
  * add matrix class and vector class
  * add breakpoints / debugging for basic language class
  * add const data arrays to basic language class example: int fred[.md](.md)={1,2,3,4}
  * add multiple undo / redo for inputbox object
  * heiarchical gui list with children that can be be opened or closed
  * "new" folder, and rename file in file requestor
