# libjpeg

## Available prebuilt libraries

All prebuilt libraries are available in the php-libs
[repositories](http://windows.php.net/downloads/php-sdk/deps/)

Only static library (.lib) are available for now.

# Building for PHP

## Requirements

  * libjpeg sources, fetch our patched (see below) [version](https://github.com/winlibs/) or the original [sources](http://www.ijg.org)

  * Common tools used to compile PHP

## Preparing the sources

To compile libjpeg to be used with PHP (and more generally with any decent
VC), the makefile has to be altered, add the /MD flag as follow (~ line 15):

    
    CFLAGS= $(cflags) $(cdebug) $(cvars) -I. /MD

then

    
     copy jconfig.vc jconfig.h

## Configuration

Two modes are available, debug or non debug (fully optimized).

## Compilation

### Release

    
    nmake /f makefile.vc nodebug=1

### Debug

    
    nmake /f makefile.vc
