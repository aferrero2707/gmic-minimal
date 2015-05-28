/** -*- mode: c++ ; c-basic-offset: 3 -*-
 * @file   MainWindow.h
 * @author Sebastien Fourey
 * @date   July 2010
 * @brief  Declaration of the class ImageFilter
 * 
 * This file is part of the ZArt software's source code.
 * 
 * Copyright Sebastien Fourey / GREYC Ensicaen (2010-...) 
 * 
 *                    https://foureys.users.greyc.fr/
 * 
 * This software is a computer program whose purpose is to demonstrate
 * the possibilities of the GMIC image processing language by offering the
 * choice of several manipulations on a video stream aquired from a webcam. In
 * other words, ZArt is a GUI for G'MIC real-time manipulations on the output
 * of a webcam.
 * 
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". See also the directory "Licence" which comes
 * with this source code for the full text of the CeCILL license. 
 * 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability. 
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or 
 * data to be ensured and,  more generally, to use and operate it in the 
 * same conditions as regards security. 
 * 
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */
#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QtXml>
#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <QLineEdit>
#include <QDomNode>
#include <QVector>
#include "ui_MainWindow.h"
#include "WebcamSource.h"
#include "StillImageSource.h"
#include "VideoFileSource.h"
#include "FilterThread.h"
#include <QMutex>
#include <QSemaphore>


class QScrollArea;
class ImageView;
class QSpinBox;
class QComboBox;
class QNetworkReply;
class QNetworkAccessManager;
class QMenu;
class TreeWidgetPresetItem;
class FullScreenWidget;

class MainWindow : public QMainWindow, public Ui::MainWindow {
   Q_OBJECT
public:


   MainWindow( QWidget * parent = 0 );
   ~MainWindow();
   QString getPreset( const QString & name );

   enum Source { Webcam, StillImage, Video };
   enum DisplayMode { InWindow, FullScreen };

public slots:

   void play();
   void stop(bool restart = false);
   void onImageAvailable();
   void onEndOfSource();
   void onButtonPlay(bool);
   void commandModified();
   void presetClicked( QTreeWidgetItem * item, int column );

   void imageViewMouseEvent( QMouseEvent * );
   void snapshot();
   void about();
   void license();
   void visitGMIC();
   void setWebcamSkipFrames(int );
   void setVideoSkipFrames(int );
   void setImageFPS(int );
   void setVideoFPS(int );
   void setPresetsFile( const QString & = QString() );
   void savePresetsFile();

   void onWebcamComboChanged( int index );
   void onUseOnlinePresets( bool );
   void onUseBuiltinPresets( bool );
   void onReloadPresets();
   void networkReplyFinished(QNetworkReply*);
   void onPreviewModeChanged( int index );
   void onRightPanel( bool );
   void onComboSourceChanged( int );
   void onOpenImageFile();
   void onOpenVideoFile();
   void updateWindowTitle();
   void onVideoFileLoop(bool);
   void changePlayButtonAppearence(bool);
   void onFilterThreadFinished();
   void onCommandParametersChanged();
   void enterFullScreenMode();
   void leaveFullScreenMode();

private:

   void setPresets( const QDomElement & );
   void addPresets( const QDomElement &,
                    TreeWidgetPresetItem *,
                    TreeWidgetPresetItem * );
   void setCurrentPreset( QDomNode node );
   void showOneSourceImage();

   int _firstWebcamIndex;
   int _secondWebcamIndex;
   WebcamSource _webcam;
   StillImageSource _stillImage;
   VideoFileSource _videoFile;
   ImageSource * _currentSource;
   FilterThread * _filterThread;
   QDomDocument _presets;
   QDomNode _currentPresetNode;
   QString _currentDir;
   QString _imageFilters;
   QNetworkAccessManager * _networkManager;
   QButtonGroup * _bgZoom;
   QString _filtersPath;
   QAction * _onlinePresetsAction;
   QAction * _builtInPresetsAction;
   QAction * _startStopAction;
   Source _source;
   DisplayMode _displayMode;
   FullScreenWidget * _fullScreenWidget;
   int _presetsCount;
   QSemaphore _filterThreadSemaphore;
   bool _zeroFPS;
};

#endif
