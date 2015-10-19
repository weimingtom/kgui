# Building for Windows using Microsoft Visual Studio #

open the kgui solution file in Visual Studio
then press f5 to build

# Building for Windows using MSYS / MinGW #

open the mingw shell

```
./configure
make other
make
```

To build the samples just go into the sample directory and

```
cd samples
cd (name of sample to build)
make
```

# Building for Linux #

For complete beginners, you will need to install the compiler and build enviroment, and also subversion to download the source code.

```
apt-get install build-essential
apt-get install subversion
```

The linux build uses two external libraries:
**XWindows** - for the window manager / screen / mouse / keyboard
and **Cups** - for printing

```
apt-get install xlibs-static-dev
```

_on  some systems the above is called xorg-dev_

```
apt-get install libcupsys2-dev
apt-get install libcupimage2-dev
```

if you are using the mysql code in kgui then you need to also have the mysql code

```
apt-get install mysql-server
apt-get install libmysqlclient15-dev
```

Once you have the packages installed then you can start the building

```
./configure
make other
make
```

**_currently it will build ( in the make other step )until it gets to matrixsll and then it dies since the matrixssl makefile is currently not working with linux you can just ignore this since it is the last thing to be built._**

To build the samples just go into the sample directory and

```
cd samples
cd (name of sample to build)
make
```

# Building Macintosh #

```
open a terminal window

./configure
make other
make
```

To build the samples just go into the sample directory and

```
cd samples
cd (name of sample to build)
make
```