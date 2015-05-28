TEMPLATE = app
QT += xml network widgets
CONFIG	+= qt
CONFIG	+= warn_on

!macx {
	INCLUDEPATH	+= .. ./include /usr/include/opencv ../src/
} else {
	INCLUDEPATH	+= .. ./include /opt/local/include /opt/local/include/opencv ../src/
}

DEPENDPATH += ./include

HEADERS	+= include/ImageView.h \
           include/MainWindow.h \
           include/FilterThread.h \
           include/CommandEditor.h \
           include/DialogAbout.h \
           include/ImageConverter.h \
           include/DialogLicense.h \
    include/ImageSource.h \
    include/WebcamSource.h \
    include/StillImageSource.h \
    include/VideoFileSource.h \
    include/Common.h \
    include/TreeWidgetPresetItem.h \
    include/AbstractParameter.h \
    include/IntParameter.h \
    include/CommandParamsWidget.h \
    include/SeparatorParameter.h \
    include/NoteParameter.h \
    include/FloatParameter.h \
    include/BoolParameter.h \
    include/ChoiceParameter.h \
    include/ColorParameter.h \
    include/CriticalRef.h \
    include/FullScreenWidget.h \
    include/FileParameter.h \
    include/FolderParameter.h \
    include/TextParameter.h \
    include/LinkParameter.h \
    include/ConstParameter.h

SOURCES	+= \
           src/ImageView.cpp \
           src/MainWindow.cpp \
           src/ZArt.cpp \
           src/FilterThread.cpp \
           src/DialogAbout.cpp \
           src/CommandEditor.cpp \
           src/ImageConverter.cpp \
           src/DialogLicense.cpp \
    src/ImageSource.cpp \
    src/WebcamSource.cpp \
    src/StillImageSource.cpp \
    src/VideoFileSource.cpp \
    src/TreeWidgetPresetItem.cpp \
    src/AbstractParameter.cpp \
    src/IntParameter.cpp \
    src/CommandParamsWidget.cpp \
    src/SeparatorParameter.cpp \
    src/NoteParameter.cpp \
    src/FloatParameter.cpp \
    src/BoolParameter.cpp \
    src/ChoiceParameter.cpp \
    src/ColorParameter.cpp \
    src/FullScreenWidget.cpp \
    src/FileParameter.cpp \
    src/FolderParameter.cpp \
    src/TextParameter.cpp \
    src/LinkParameter.cpp \
    src/ConstParameter.cpp

RESOURCES = zart.qrc
FORMS = ui/MainWindow.ui ui/DialogAbout.ui ui/DialogLicense.ui \
    ui/FullScreenWidget.ui

exists( /usr/include/opencv2 ) {
 DEFINES += OPENCV2_HEADERS
}

system(pkg-config opencv --libs > /dev/null 2>&1) {
# LIBS += -lX11 ../src/libgmic.a `pkg-config opencv --libs` -lfftw3 -lfftw3_threads
 OPENCVLIBS = $$system(pkg-config opencv --libs)
 OPENCVLIBS = $$replace( OPENCVLIBS, -lcvaux, )
# LIBS += -lX11 ../src/libgmic.a $$OPENCVLIBS -lfftw3 -lfftw3_threads -lz -Dcimg_use_openmp -fopenmp
LIBS += -lX11 ../src/libgmic.a $$OPENCVLIBS -lfftw3 -lfftw3_threads -lz -ljpeg -lpng -ltiff -lX11 -lcurl
!macx {
LIBS += -Dcimg_use_openmp -fopenmp
}
} else {
  LIBS += -lX11 ../src/libgmic.a -lopencv_core -lopencv_highgui -lfftw3 -lfftw3_threads -lz -ljpeg -lpng -ltiff -lX11 -lcurl -lopencv_imgproc -lopencv_objdetect -Dcimg_use_openmp -fopenmp
# LIBS += -lX11 ../src/libgmic.a -lcxcore -lcv -lml -lhighgui -lfftw3 -lfftw3_threads
}

PRE_TARGETDEPS +=
QMAKE_CXXFLAGS_DEBUG += -Dcimg_use_fftw3 -Dcimg_use_zlib -D_ZART_DEBUG_
QMAKE_CXXFLAGS_RELEASE += -ffast-math -Dcimg_use_fftw3 -Dcimg_use_zlib
UI_DIR = .ui
MOC_DIR = .moc
RCC_DIR = .qrc
OBJECTS_DIR = .obj

unix:!macx {
	DEFINES += _IS_UNIX_
}

DEFINES += cimg_display=0

#QMAKE_LIBS =
#QMAKE_LFLAGS_DEBUG = -lcxcore -lcv -lhighgui -lml
#QMAKE_LFLAGS_RELEASE = -lcxcore -lcv -lhighgui -lml
