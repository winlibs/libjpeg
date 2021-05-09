!IFNDEF VERSION
VERSION=unknown
!ENDIF

OUTPUT=$(MAKEDIR)\..\libjpeg-turbo-$(VERSION)-$(PHP_SDK_VS)-$(PHP_SDK_ARCH)
ARCHIVE=$(OUTPUT).zip

all:
	git checkout .
	git clean -fdx

	cmake -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DWITH_JPEG8=1 -DWITH_CRT_DLL=1 .
	sed -i "s/jpeg-static\.lib/libjpeg_a\.lib/g" CMakeFiles\jpeg-static.dir\build.make
	nmake jpeg-static

	-rmdir /q /s $(OUTPUT)
	xcopy jconfig.h $(OUTPUT)\include\*
	xcopy jerror.h $(OUTPUT)\include\*
	xcopy jmorecfg.h $(OUTPUT)\include\*
	xcopy jpeglib.h $(OUTPUT)\include\*
	xcopy jversion.h $(OUTPUT)\include\*
	xcopy libjpeg_a.lib $(OUTPUT)\lib\*

	del $(ARCHIVE)
	7za a $(ARCHIVE) $(OUTPUT)\*
