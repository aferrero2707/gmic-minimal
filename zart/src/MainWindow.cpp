/** -*- mode: c++ ; c-basic-offset: 3 -*-
 * @file   MainWindow.cpp
 * @author Sebastien Fourey
 * @date   July 2010
 * @brief  Declaration of the class MainWindow
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
#include <iostream>

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageWriter>
#include <QKeySequence>
#include <QLabel>
#include <QList>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QShortcut>
#include <QSlider>
#include <QUrl>
#include <QtXml>
#include <QToolTip>
#include <QGridLayout>
#include <QMessageBox>

#include "Common.h"
#include "DialogAbout.h"
#include "DialogLicense.h"
#include "ImageConverter.h"
#include "ImageView.h"
#include "MainWindow.h"
#include "WebcamSource.h"
#include "FilterThread.h"
#include "TreeWidgetPresetItem.h"
#include "FullScreenWidget.h"

MainWindow::MainWindow( QWidget * parent )
  : QMainWindow( parent ),
    _filterThread(0),
    _source( Webcam ),
    _currentSource( &_webcam ),
    _currentDir( "." ),
    _zeroFPS( false ),
    _presetsCount(0)
{
  setupUi(this);

  delete _frameImageView->layout();
  _frameImageView->setLayout(new QGridLayout);
  _frameImageView->layout()->addWidget(_imageView);

  QMargins margins = _rightPanel->layout()->contentsMargins();
  margins.setTop(0);
  margins.setBottom(0);
  _rightPanel->layout()->setContentsMargins(margins);
  _rightPanelSplitter->setCollapsible(0,false);

  _verticalSplitter->setCollapsible(0,false);
  _verticalSplitter->setCollapsible(1,true);

  _fullScreenWidget = new FullScreenWidget(this);
  connect( _fullScreenWidget, SIGNAL(escapePressed()),
           this, SLOT( leaveFullScreenMode()) );
  _displayMode = InWindow;

  QSettings settings;

  // Menu and actions
  QMenu * menu;
  menu = menuBar()->addMenu( "&File" );

  QAction * action = new QAction( "&Save presets...", this );
  action->setShortcut( QKeySequence::SaveAs );
#if QT_VERSION >= 0x040600
  action->setIcon( QIcon::fromTheme( "document-save-as" ) );
#endif
  menu->addAction( action );
  connect( action, SIGNAL( triggered() ),
           this, SLOT( savePresetsFile() ) );

  action = new QAction( "&Quit", this );
  action->setShortcut( QKeySequence::Quit );
#if QT_VERSION >= 0x040600
  action->setIcon( QIcon::fromTheme( "application-exit",
                                     QIcon(":/images/application-exit.png") ) );
#else
  action->setIcon( QIcon(":/images/application-exit.png") );
#endif
  connect( action, SIGNAL( triggered() ),
           qApp, SLOT( closeAllWindows() ) );
  menu->addAction( action );

#if QT_VERSION >= 0x040600
  _tbZoomOriginal->setIcon( QIcon::fromTheme("zoom-original",
                                             QIcon(":/images/zoom-original.png") ));
  _tbZoomFit->setIcon( QIcon::fromTheme("zoom-fit-best",
                                        QIcon(":/images/zoom-fit-best.png") ));
  _tbCamResolutionsRefresh->setIcon( QIcon::fromTheme("view-refresh" ) );
#else
  _tbZoomOriginal->setIcon( QIcon(":/images/zoom-original.png") );
  _tbZoomFit->setIcon( QIcon(":/images/zoom-fit-best.png") );
#endif

  connect(_tbCamResolutionsRefresh,SIGNAL(clicked(bool)),
          this,SLOT(onRefreshCameraResolutions()));



  // Find available cameras, and setup the default one
  QList<int> cameras = WebcamSource::getWebcamList();
  initGUIFromCameraList(cameras);

  QSize cameraSize = _comboCamResolution->currentData().toSize();
  if ( ! cameraSize.isValid() ) {
    _imageView->resize( QSize(640,480) );
  } else {
    _imageView->resize( cameraSize );
  }
  _frameImageView->resize( _imageView->size() );

  // Options menu
  menu = menuBar()->addMenu( "&Options" );

  action = menu->addAction("Show right panel",this,SLOT(onRightPanel(bool)),QKeySequence("Ctrl+F"));
  action->setCheckable(true);
  action->setChecked(settings.value("showRightPanel",true).toBool());

  action = menu->addAction("&Full screen",this,SLOT( enterFullScreenMode() ), QKeySequence( Qt::Key_F5 ) );
  action->setShortcutContext( Qt::ApplicationShortcut );

  menu->addSeparator();
  action = menu->addAction("Detect &cameras", this,SLOT(onDetectCameras()) );
  menu->addSeparator();

  // Presets
  QString presetsConfig = settings.value("Presets",QString("Built-in")).toString();

  QActionGroup * group = new QActionGroup(menu);
  group->setExclusive(true);

  // Built-in
  _builtInPresetsAction = new QAction( "&Built-in presets", menu );
  _builtInPresetsAction->setCheckable( true );
  connect( _builtInPresetsAction, SIGNAL( toggled(bool) ),
           this, SLOT( onUseBuiltinPresets(bool) ) );
  group->addAction( _builtInPresetsAction );
  menu->addAction( _builtInPresetsAction );
  _builtInPresetsAction->setChecked(true); // Default to Built-in presets

  // File
  action = menu->addAction("&Presets file...",this,SLOT( setPresetsFile() ));
  action->setCheckable( true );
  group->addAction( action );
  QString filename = settings.value("PresetsFile",QString()).toString();
  if ( presetsConfig == "File" && !filename.isEmpty() ) {
    setPresetsFile(filename);
    action->setChecked(true);
  }

  // Refresh
  menu->addSeparator();

  action = menu->addAction("&Reload presets",this,SLOT(onReloadPresets()),QKeySequence("Ctrl+R"));
  action->setShortcutContext(Qt::ApplicationShortcut);

  // Help menu
  menu = menuBar()->addMenu( "&Help" );

  action = menu->addAction("&Visit G'MIC website", this, SLOT( visitGMIC() ) );
  action->setIcon( QIcon(":/images/gmic_hat.png") );
  action = menu->addAction("&License...", this, SLOT(license()) );
  action = menu->addAction("&About...", this, SLOT(about()));

  _imageView->QWidget::resize( _webcam.width(), _webcam.height() );

  _bgZoom = new QButtonGroup(this);
  _bgZoom->setExclusive(true);
  _tbZoomFit->setCheckable(true);
  _tbZoomFit->setChecked(true);
  _tbZoomOriginal->setCheckable(true);
  _bgZoom->addButton(_tbZoomOriginal);
  _bgZoom->addButton(_tbZoomFit);

  _cbPreviewMode->addItem("Full",FilterThread::Full);
  _cbPreviewMode->addItem("Top",FilterThread::TopHalf);
  _cbPreviewMode->addItem("Left",FilterThread::LeftHalf);
  _cbPreviewMode->addItem("Bottom",FilterThread::BottomHalf);
  _cbPreviewMode->addItem("Right",FilterThread::RightHalf);
  _cbPreviewMode->addItem("Original",FilterThread::Original);

#if QT_VERSION >= 0x040600
  _cbPreviewMode->setItemIcon(0,QIcon::fromTheme("view-fullscreen"));
  _cbPreviewMode->setItemIcon(1,QIcon::fromTheme("go-up"));
  _cbPreviewMode->setItemIcon(2,QIcon::fromTheme("go-previous"));
  _cbPreviewMode->setItemIcon(3,QIcon::fromTheme("go-down"));
  _cbPreviewMode->setItemIcon(4,QIcon::fromTheme("go-next"));
  _cbPreviewMode->setItemIcon(5,QIcon::fromTheme("go-home"));
  _tbCamera->setIcon(QIcon::fromTheme("camera-photo",QIcon(":/images/camera.png")));
#else
  _tbCamera->setIcon(QIcon(":images/camera.png");
#endif


  connect( _cbPreviewMode, SIGNAL(activated(int)),
           this, SLOT(onPreviewModeChanged(int)));

  connect( _tbZoomOriginal, SIGNAL( clicked() ),
           _imageView, SLOT( zoomOriginal() ) );

  connect( _tbZoomFit, SIGNAL( clicked() ),
           _imageView, SLOT( zoomFitBest() ) );

  connect( _pbApply, SIGNAL( clicked() ),
           this, SLOT( commandModified() ) );

  connect( _tbCamera, SIGNAL( clicked() ),
           this, SLOT( snapshot() ) );

  _imageView->setMouseTracking( true );

  connect( _imageView, SIGNAL( mousePress( QMouseEvent * ) ),
           this, SLOT( imageViewMouseEvent( QMouseEvent * ) ) );

  connect( _imageView, SIGNAL( mouseMove( QMouseEvent * ) ),
           this, SLOT( imageViewMouseEvent( QMouseEvent * ) ) );

  connect( _treeGPresets, SIGNAL( itemClicked( QTreeWidgetItem *, int )),
           this, SLOT( presetClicked( QTreeWidgetItem *, int ) ) );

  connect( _fullScreenWidget->treeWidget(), SIGNAL( itemClicked( QTreeWidgetItem *, int )),
           this, SLOT( presetClicked( QTreeWidgetItem *, int ) ) );

  connect( _commandEditor, SIGNAL( commandModified() ),
           this, SLOT( commandModified() ) );

  _sliderWebcamSkipFrames->setRange(0,10);
  connect( _sliderWebcamSkipFrames, SIGNAL( valueChanged(int) ),
           this, SLOT( setWebcamSkipFrames(int ) ) );

  _sliderVideoSkipFrames->setRange(0,10);
  connect( _sliderVideoSkipFrames, SIGNAL( valueChanged(int) ),
           this, SLOT( setVideoSkipFrames(int ) ) );

  _sliderImageFPS->setValue(24);
  _sliderImageFPS->setToolTip("24 fps");
  connect( _sliderImageFPS, SIGNAL(valueChanged(int)),
           this, SLOT(setImageFPS(int)));
  _zeroFPS = false;

  _sliderVideoFPS->setValue(24);
  _sliderVideoFPS->setToolTip("24 fps");
  connect( _sliderVideoFPS, SIGNAL(valueChanged(int)),
           this, SLOT(setVideoFPS(int)));

  _cbVideoFileLoop->setChecked(true);
  _videoFile.setLoop(true);
  connect( _cbVideoFileLoop, SIGNAL(toggled(bool)),
           this, SLOT(onVideoFileLoop(bool)));

  _webcamParamsWidget->setVisible( _source == Webcam );
  _imageParamsWidget->setVisible( _source == StillImage );
  _videoParamsWidget->setVisible( _source == Video );

  connect( _pbOpenImageFile, SIGNAL(clicked()),
           this, SLOT(onOpenImageFile()) );
  connect( _pbOpenVideoFile, SIGNAL(clicked()),
           this, SLOT(onOpenVideoFile()) );

  // Image filter for the "Save as..." dialog
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  QList<QByteArray>::iterator it = formats.begin();
  QList<QByteArray>::iterator end = formats.end();
  _imageFilters = "Image file (";
  while ( it != end ) {
    _imageFilters += QString("*.") + QString(*it).toLower() + " ";
    ++it;
  }
  _imageFilters.chop(1);
  _imageFilters += ")";

  _startStopAction = new QAction(this);
  _startStopAction->setToolTip("Launch processing (Ctrl+P)");
  _startStopAction->setShortcut(QKeySequence("Ctrl+P"));
  _startStopAction->setShortcutContext(Qt::ApplicationShortcut);
  _startStopAction->setCheckable(true);

  _tbPlay->setDefaultAction(_startStopAction);
  changePlayButtonAppearence(false);
  connect( _fullScreenWidget,SIGNAL(spaceBarPressed()),
           _startStopAction, SLOT(toggle()));
  connect( _startStopAction, SIGNAL(toggled(bool)),
           this, SLOT(changePlayButtonAppearence(bool)));
  connect( _startStopAction, SIGNAL(toggled(bool)),
           this, SLOT(onButtonPlay(bool)));

  connect(_commandParamsWidget,SIGNAL(valueChanged()),
          this,SLOT(onCommandParametersChanged()));
  connect(_fullScreenWidget->commandParamsWidget(),SIGNAL(valueChanged()),
          this,SLOT(onCommandParametersChanged()));

  if ( ! settings.value("showRightPanel",true).toBool() )
    _rightPanel->hide();

  if ( _comboWebcam->count() ) {
    _webcam.setCameraIndex(_comboWebcam->currentData().toInt());
    // Update actual source capture size
    _webcam.start();
    _webcam.stop();
  }
  updateWindowTitle();
}

MainWindow::~MainWindow()
{
  QSettings settings;
  for ( int i = 0; i < _cameraDefaultResolutionsIndexes.size(); ++i ) {
    settings.setValue(QString("WebcamSource/DefaultResolutionCam%1").arg(i),
                      WebcamSource::webcamResolutions(i).at(_cameraDefaultResolutionsIndexes[i]));
  }
  if ( _filterThread ) {
    _filterThread->stop();
    _filterThread->wait();
    delete _filterThread;
  }
  delete _fullScreenWidget;
}

void
MainWindow::setCurrentPreset(QDomNode node)
{
  QDomNode command =  node.namedItem("command");
  _commandEditor->setPlainText( command.firstChild().toText().data().trimmed() );
  _currentPresetNode = node;
  if ( _displayMode == FullScreen ) {
    _fullScreenWidget->commandParamsWidget()->saveValuesInDOM();
    _fullScreenWidget->commandParamsWidget()->build(node);
  } else {
    _commandParamsWidget->saveValuesInDOM();
    _commandParamsWidget->build(node);
  }
}

void
MainWindow::showOneSourceImage()
{
  _currentSource->capture();
  IplImage * image = _currentSource->image();
  if (  image ) {
    _imageView->imageMutex().lock();
    QSize size( image->width, image->height );
    if ( _imageView->image().size() != size ) {
      _imageView->image() = QImage( size, QImage::Format_RGB888 );
    }
    ImageConverter::convert( image, &(_imageView->image()) );
    _imageView->imageMutex().unlock();
    _imageView->checkSize();
    _imageView->repaint();
  }
}

void
MainWindow::about()
{
  DialogAbout * d = new DialogAbout( this );
  d->exec();
  delete d;
}

void
MainWindow::visitGMIC()
{
  QDesktopServices::openUrl(QUrl("http://gmic.eu/"));
}

void
MainWindow::license()
{
  DialogLicense * d = new DialogLicense( this );
  d->exec();
  delete d;
}

QString
MainWindow::getPreset( const QString & name )
{
  QDomNodeList list =  _presets.elementsByTagName("preset");
  for ( int i = 0; i < list.count(); ++i ) {
    QDomNode n = list.at(i);
    if ( n.attributes().namedItem( "name" ).nodeValue() == name ) {
      QDomNode command=  n.namedItem("command");
      return command.firstChild().toText().data().trimmed();
    }
  }
  return QString();
}

void
MainWindow::onImageAvailable()
{
  if ( _displayMode == InWindow ) {
    _imageView->checkSize();
    _imageView->repaint();
  } else {
    _fullScreenWidget->imageView()->checkSize();
    _fullScreenWidget->imageView()->repaint();
  }
}

void
MainWindow::play()
{
  int pm = _cbPreviewMode->itemData(_cbPreviewMode->currentIndex()).toInt();
  FilterThread::PreviewMode  previewMode = static_cast<FilterThread::PreviewMode>(pm);

  ImageView * view = (_displayMode==InWindow)?_imageView:_fullScreenWidget->imageView();

  switch (_source) {
  case Webcam:
    _webcam.start();
    _filterThread = new FilterThread( _webcam,
                                      _commandEditor->toPlainText(),
                                      &view->image(),
                                      &view->imageMutex(),
                                      previewMode,
                                      _sliderWebcamSkipFrames->value(),
                                      -1,
                                      0 );
    break;
  case StillImage:
    _filterThread = new FilterThread( _stillImage,
                                      _commandEditor->toPlainText(),
                                      &view->image(),
                                      &view->imageMutex(),
                                      previewMode,
                                      0,
                                      _sliderImageFPS->value(),
                                      &_filterThreadSemaphore );
    break;
  case Video:
    _filterThread = new FilterThread( _videoFile,
                                      _commandEditor->toPlainText(),
                                      &view->image(),
                                      &view->imageMutex(),
                                      previewMode,
                                      _sliderVideoSkipFrames->value(),
                                      _sliderVideoFPS->value(),
                                      0 );
    break;
  }
  connect( _filterThread, SIGNAL( imageAvailable() ),
           this, SLOT( onImageAvailable() ) );
  connect( _filterThread, SIGNAL( finished()),
           this, SLOT( onFilterThreadFinished()) );
  connect( _filterThread, SIGNAL( endOfCapture()),
           this, SLOT( onEndOfSource()) );
  if ( _displayMode == FullScreen )
    _filterThread->setArguments(_fullScreenWidget->commandParamsWidget()->valueString());
  else
    _filterThread->setArguments(_commandParamsWidget->valueString());
  if ( _source == StillImage )
    _filterThreadSemaphore.release();
  _filterThread->start();
}

void
MainWindow::stop(bool restart)
{
  if ( _filterThread ) {
    _filterThread->stop();
    _filterThreadSemaphore.release();
    _filterThread->wait();
    _filterThread = 0;
    _webcam.stop();
  }
  if ( restart ) {
    play();
  }
}

void
MainWindow::onEndOfSource()
{
  _startStopAction->setChecked(false);
}

void
MainWindow::onFilterThreadFinished()
{
  delete sender();
}

void
MainWindow::onCommandParametersChanged()
{
  if ( _filterThread ) {
    if (_displayMode == FullScreen )
      _filterThread->setArguments(_fullScreenWidget->commandParamsWidget()->valueString());
    else
      _filterThread->setArguments(_commandParamsWidget->valueString());
    if ( _source == StillImage && _zeroFPS ) _filterThreadSemaphore.release();
  }
}

void MainWindow::enterFullScreenMode()
{
  if ( _displayMode == FullScreen ) { // F5 shortcut while already fullscreen
    leaveFullScreenMode();
    return;
  }
  _displayMode = FullScreen;
  bool running = _filterThread && _filterThread->isRunning();
  stop(false);
  _commandParamsWidget->saveValuesInDOM();
  _fullScreenWidget->imageView()->image() = _imageView->image();
  _fullScreenWidget->imageView()->zoomFitBest();
  _fullScreenWidget->commandParamsWidget()->build(_currentPresetNode);
  _fullScreenWidget->showFullScreen();
  if ( running ) play();
}

void
MainWindow::leaveFullScreenMode()
{
  _fullScreenWidget->close();
  _displayMode = InWindow;
  bool running = _filterThread && _filterThread->isRunning();
  stop(false);
  _fullScreenWidget->commandParamsWidget()->saveValuesInDOM();
  _commandParamsWidget->build(_currentPresetNode);
  if ( running ) play();
}

void
MainWindow::onButtonPlay(bool on)
{
  if ( !on && _filterThread ) {
    stop(false);
    changePlayButtonAppearence(false);
  }
  if ( on && !_filterThread ){
    if ( (_source == Video && _videoFile.filename().isEmpty()) ||
         (_source == StillImage && _stillImage.filename().isEmpty() ) ) {
      QMessageBox::information(this,"Information","No input file.\nPlease select one first.");
      _startStopAction->setChecked(false);
    } else {
      play();
      changePlayButtonAppearence(true);
    }
  }
}

void
MainWindow::onComboSourceChanged(int i)
{
  bool running = _filterThread && _filterThread->isRunning();
  if ( running ) stop(false);
  _source = static_cast<Source>(_comboSource->itemData(i).toInt());
  switch (_source) {
  case Webcam:
    _currentSource = &_webcam;
    break;
  case StillImage:
    _currentSource = &_stillImage;
    break;
  case Video:
    _currentSource = &_videoFile;
    break;
  }
  _webcamParamsWidget->setVisible( _source == Webcam );
  _imageParamsWidget->setVisible( _source == StillImage);
  _videoParamsWidget->setVisible( _source == Video );
  updateWindowTitle();
  if ( _source == StillImage && _stillImage.filename().isEmpty() )
    onOpenImageFile();
  if ( _source == Video && _videoFile.filename().isEmpty() )
    onOpenVideoFile();
  if ( running )
    play();
  else
    showOneSourceImage();
}

void
MainWindow::onOpenImageFile()
{
  QString filename;
  filename = QFileDialog::getOpenFileName(this,
                                          "Select an image file",
                                          _stillImage.filePath().isEmpty()?_videoFile.filePath():_stillImage.filePath(),
                                          "Image files (*.bmp *.gif *.jpg *.png *.pbm *.pgm *.ppm *.xbm *.xpm *.svg)");
  if (filename.isEmpty()) return;
  if ( _source == StillImage && _filterThread ) {
    stop(false);
    if (_stillImage.loadImage(filename) ) {
      updateWindowTitle();
    }
    play();
  } else {
    if (_stillImage.loadImage(filename) ) {
      updateWindowTitle();
      showOneSourceImage();
    }
  }
}

void
MainWindow::onOpenVideoFile()
{
  QString filename;
  filename = QFileDialog::getOpenFileName(this,"Select a video file",
                                          _videoFile.filePath().isEmpty()?_stillImage.filePath():_videoFile.filePath(),
                                          "Video files (*.avi *.mpg)");
  if (filename.isEmpty()) return;
  if ( _source == Video && _filterThread ) {
    stop(false);
    if (_videoFile.loadVideoFile(filename) ) {
      updateWindowTitle();
      play();
    }
  } else {
    if (_videoFile.loadVideoFile(filename) ) {
      updateWindowTitle();
      showOneSourceImage();
    }
  }
}

void
MainWindow::updateWindowTitle()
{
  QString name;
  switch (_source) {
  case Webcam:
    setWindowTitle( QString("ZArt %1 (Webcam %2x%3)")
                    .arg((ZART_VERSION))
                    .arg(_currentSource->width())
                    .arg(_currentSource->height()));
    break;
  case StillImage:
    name = QFileInfo(_stillImage.filename()).fileName();
    if ( name.isEmpty() )
      setWindowTitle( QString("ZArt %1 (No input file)")
                      .arg((ZART_VERSION)) );
    else
      setWindowTitle( QString("ZArt %1 (%2 %3x%4)")
                      .arg((ZART_VERSION))
                      .arg(name)
                      .arg(_currentSource->width())
                      .arg(_currentSource->height()) );
    break;
  case Video:
    name = QFileInfo(_videoFile.filename()).fileName();
    if ( name.isEmpty() )
      setWindowTitle( QString("ZArt %1 (No input file)")
                      .arg((ZART_VERSION)) );
    else
      setWindowTitle( QString("ZArt %1 (%2 %3x%4)")
                      .arg((ZART_VERSION))
                      .arg(name)
                      .arg(_currentSource->width())
                      .arg(_currentSource->height()) );
    break;
  }
}

void
MainWindow::onVideoFileLoop(bool on)
{
  _videoFile.setLoop(on);
}

void
MainWindow::changePlayButtonAppearence(bool on)
{
  if ( on ) {
#if QT_VERSION >= 0x040600
    _startStopAction->setIcon( QIcon::fromTheme("media-playback-stop",
                                                QIcon(":/images/media-playback-stop.png")) );
#else
    _startStopAction->setIcon( QIcon(":/images/media-playback-stop.png") );
#endif
    _startStopAction->setToolTip("Stop processing (Ctrl+P)");
  } else {
#if QT_VERSION >= 0x040600
    _startStopAction->setIcon( QIcon::fromTheme("media-playback-start",
                                                QIcon(":/images/media-playback-start.png")) );
#else
    _startStopAction->setIcon( QIcon(":/images/media-playback-start.png") );
#endif
    _startStopAction->setToolTip("Launch processing (Ctrl+P)");
  }
}

void
MainWindow::imageViewMouseEvent( QMouseEvent * event )
{
  int buttons = 0;
  if ( event->buttons() & Qt::LeftButton ) buttons |= 1;
  if ( event->buttons() & Qt::RightButton ) buttons |= 2;
  if ( event->buttons() & Qt::MidButton ) buttons |= 4;
  if (_filterThread)
    _filterThread->setMousePosition( event->x(), event->y(), buttons );
  if ( _source == StillImage && _zeroFPS ) _filterThreadSemaphore.release();
}

void
MainWindow::commandModified()
{
  if ( _filterThread && _filterThread->isRunning() ) {
    stop(true);
  }
}

void
MainWindow::presetClicked( QTreeWidgetItem * item, int  )
{
  if ( item->childCount() ) return;
  TreeWidgetPresetItem * presetItem = dynamic_cast<TreeWidgetPresetItem*>(item);
  if ( presetItem )
    setCurrentPreset( presetItem->node() );
  _tabParams->setCurrentIndex(1);
  int previewMode = _cbPreviewMode->itemData(_cbPreviewMode->currentIndex()).toInt();
  if ( previewMode == FilterThread::Original ) {
    _cbPreviewMode->setCurrentIndex(0);
    onPreviewModeChanged(0);
  }
  commandModified();
}

void
MainWindow::snapshot()
{
  if ( _filterThread ) _startStopAction->setChecked(false);
  QString filename = QFileDialog::getSaveFileName( this,
                                                   "Save image as...",
                                                   _currentDir,
                                                   _imageFilters,
                                                   0,
                                                   0 );
  if ( ! filename.isEmpty() ) {
    QFileInfo info( filename );
    _currentDir = info.filePath();
    QImageWriter writer( filename );
    _imageView->imageMutex().lock();
    writer.write( _imageView->image() );
    _imageView->imageMutex().unlock();
  }
}

void
MainWindow::setWebcamSkipFrames(int i)
{
  _sliderWebcamSkipFrames->setToolTip(QString("%1").arg(i));
  QToolTip::showText(_sliderWebcamSkipFrames->mapToGlobal(QPoint(0,0)),QString("%1").arg(i),_sliderWebcamSkipFrames);
  if ( _filterThread )
    _filterThread->setFrameSkip( i );
}

void MainWindow::setVideoSkipFrames(int i)
{
  _sliderVideoSkipFrames->setToolTip(QString("%1").arg(i));
  QToolTip::showText(_sliderVideoSkipFrames->mapToGlobal(QPoint(0,0)),QString("%1").arg(i),_sliderVideoSkipFrames);
  if ( _filterThread )
    _filterThread->setFrameSkip( i );
}

void
MainWindow::setImageFPS(int fps)
{
  _sliderImageFPS->setToolTip(QString("%1 fps").arg(fps));
  QToolTip::showText(_sliderImageFPS->mapToGlobal(QPoint(0,0)),QString("%1 fps").arg(fps),_sliderImageFPS);
  if ( _filterThread ) {
    _filterThread->setFPS(fps);
  }
  if ( _source == StillImage && _zeroFPS ) _filterThreadSemaphore.release();
  _zeroFPS = !fps;
}

void
MainWindow::setVideoFPS(int fps)
{
  _sliderVideoFPS->setToolTip(QString("%1 fps").arg(fps));
  QToolTip::showText(_sliderVideoFPS->mapToGlobal(QPoint(0,0)),QString("%1 fps").arg(fps),_sliderVideoFPS);
  if ( _filterThread )
    _filterThread->setFPS(fps);
}

void
MainWindow::onWebcamComboChanged( int index )
{
  index = _comboWebcam->itemData(index).toInt();
  if ( _source == Webcam && _filterThread && _filterThread->isRunning() ) {
    stop(false);
    updateCameraResolutionCombo();
    WebcamSource::setDefaultCaptureSize(_comboCamResolution->currentData().toSize());
    _webcam.setCameraIndex( index );
    play();
  } else {
    updateCameraResolutionCombo();
    WebcamSource::setDefaultCaptureSize(_comboCamResolution->currentData().toSize());
    _webcam.setCameraIndex( index );
  }
}

void
MainWindow::onWebcamResolutionComboChanged( int i )
{
  int currentCam = _comboWebcam->currentIndex();
  _cameraDefaultResolutionsIndexes[currentCam] = i;
  QSize resolution = _comboCamResolution->currentData().toSize();
  if ( _source == Webcam && _filterThread && _filterThread->isRunning() ) {
    stop(false);
    WebcamSource::setDefaultCaptureSize(resolution);
    play();
  } else {
    WebcamSource::setDefaultCaptureSize(resolution);
    // Update actual source capture size
    _webcam.start();
    _webcam.stop();
  }
  updateWindowTitle();
}

void
MainWindow::setPresets(const QDomElement & domE)
{
  _treeGPresets->clear();
  _fullScreenWidget->treeWidget()->clear();
  _presetsCount = 0;
  addPresets( domE, 0 , 0);
  _treeGPresets->setHeaderLabel(QString("Presets (%1)").arg(_presetsCount));
  _fullScreenWidget->treeWidget()->setHeaderLabel(QString("Presets (%1)").arg(_presetsCount));
}

void
MainWindow::addPresets( const QDomElement & domE,
                        TreeWidgetPresetItem * parentRegular,
                        TreeWidgetPresetItem * parentFullscreen )
{
  for( QDomNode node = domE.firstChild(); !node.isNull(); node = node.nextSibling() ) {
    QString name = node.attributes().namedItem( "name" ).nodeValue();
    if ( node.nodeName() == QString("preset") ) {
      // QString text = n.firstChild().toText().data();
      QStringList strList;
      strList << name;
      if ( ! parentRegular ) {
        Q_ASSERT( ! parentFullscreen );
        _treeGPresets->addTopLevelItem( new TreeWidgetPresetItem( strList, node ) );
        _fullScreenWidget->treeWidget()->addTopLevelItem( new TreeWidgetPresetItem( strList, node ) );
      } else {
        new TreeWidgetPresetItem( parentRegular, strList, node );
        new TreeWidgetPresetItem( parentFullscreen, strList, node );
      }
      ++_presetsCount;
    } else if ( node.nodeName() == QString("preset_group") ) {
      QStringList strList;
      strList << name;
      TreeWidgetPresetItem * parent1 = new TreeWidgetPresetItem( strList );
      TreeWidgetPresetItem * parent2 = new TreeWidgetPresetItem( strList );
      _treeGPresets->addTopLevelItem( parent1 );
      _fullScreenWidget->treeWidget()->addTopLevelItem( parent2 );
      addPresets( node.toElement(), parent1, parent2 );
    }
  }
}

void
MainWindow::setPresetsFile( const QString & file )
{
  QString filename = file;
  if ( filename.isEmpty() )  {
    QSettings settings;
    QString s = settings.value("PresetsFile").toString();
    QString dir = ".";
    if ( QFileInfo(s).exists() )
      dir = QFileInfo(s).absolutePath();
    filename = QFileDialog::getOpenFileName( this,
                                             "Open a presets file",
                                             dir,
                                             "Preset files (*.xml)" );
  }
  if ( ! filename.isEmpty() ) {
    QSettings settings;
    settings.setValue( "PresetsFile", filename );
    settings.setValue( "Presets", "File" );

    QFile presetsTreeFile( filename );
    QString error;
    presetsTreeFile.open( QIODevice::ReadOnly );
    _presets.setContent( &presetsTreeFile, false, &error );
    presetsTreeFile.close();
    setPresets(_presets.elementsByTagName("document").at(0).toElement());
  } else {
    _builtInPresetsAction->setChecked( true );
  }
}

void
MainWindow::onUseBuiltinPresets(bool on)
{
  if ( on ) {
    QFile presetsTreeFile( ":/presets.xml" );
    QString error;
    presetsTreeFile.open( QIODevice::ReadOnly );
    _presets.setContent( &presetsTreeFile, false, &error );
    presetsTreeFile.close();
    setPresets(_presets.elementsByTagName("document").at(0).toElement());
    QSettings().setValue( "Presets", "Built-in" );
  }
}

void
MainWindow::onReloadPresets()
{
  if ( _builtInPresetsAction->isChecked() ) {
    onUseBuiltinPresets(true);
    return;
  }
  QSettings settings;
  QString filename = settings.value( "PresetsFile" ).toString();
  setPresetsFile(filename);
}

void
MainWindow::savePresetsFile()
{
  QString filename = QFileDialog::getSaveFileName( this,
                                                   "Save presets file",
                                                   ".",
                                                   "Preset files (*.xml)" );
  if ( ! filename.isEmpty() ) {
    QFile presetsFile( filename );
    presetsFile.open( QIODevice::WriteOnly );
    presetsFile.write( _presets.toByteArray() );
    presetsFile.close();
  }
}

void
MainWindow::onPreviewModeChanged( int index )
{
  int mode = _cbPreviewMode->itemData(index).toInt();
  if ( _filterThread )
    _filterThread->setPreviewMode(static_cast<FilterThread::PreviewMode>(mode));
}

void
MainWindow::onRightPanel( bool on )
{
  if ( on && !_rightPanel->isVisible()) {
    _rightPanel->show();
    QSettings().setValue("showRightPanel",true);
    return;
  }
  if ( !on && _rightPanel->isVisible()) {
    _rightPanel->hide();
    QSettings().setValue("showRightPanel",false);
    return;
  }
}

void
MainWindow::updateCameraResolutionCombo()
{
  disconnect(_comboCamResolution,SIGNAL(currentIndexChanged(int)), this, 0);
  int index = _comboWebcam->currentIndex();
  _comboCamResolution->clear();
  const QList<QSize> & resolutions = WebcamSource::webcamResolutions(index);
  QList<QSize>::const_iterator it = resolutions.begin();
  while ( it != resolutions.end() ) {
    _comboCamResolution->addItem(QString("%1 x %2").arg(it->width()).arg(it->height()),*it);
    ++it;
  }
  _comboCamResolution->setCurrentIndex(_cameraDefaultResolutionsIndexes[index]);
  connect(_comboCamResolution,SIGNAL(currentIndexChanged(int)),
          this,SLOT(onWebcamResolutionComboChanged(int)));
}

void
MainWindow::onRefreshCameraResolutions()
{
  WebcamSource::clearSavedSettings();
  onDetectCameras();
}

void
MainWindow::onDetectCameras()
{
  if ( _source == Webcam && _filterThread ) {
    stop();
  }
  centralWidget()->setEnabled(false);
  statusBar()->showMessage("Updating camera resolutions list...");
  menuBar()->setEnabled(false);
  QList<int> camList = WebcamSource::getWebcamList();
  WebcamSource::retrieveWebcamResolutions(camList,0,statusBar());
  initGUIFromCameraList(camList);
  statusBar()->showMessage(QString());
  centralWidget()->setEnabled(true);
  menuBar()->setEnabled(true);
}

void
MainWindow::initGUIFromCameraList(const QList<int> & camList)
{
  disconnect(_comboWebcam,SIGNAL(currentIndexChanged(int)),this, 0);
  disconnect( _comboSource, SIGNAL(currentIndexChanged(int)),this, 0);

  QSettings settings;

  _comboSource->clear();
  _comboWebcam->clear();

  if ( camList.size() == 0 ) {
    _tabParams->setCurrentIndex(1);
    _comboSource->addItem("Image",QVariant(StillImage));
    _comboSource->addItem("Video file",QVariant(Video));
#if QT_VERSION >= 0x040600
    _comboSource->setItemIcon(0,QIcon::fromTheme("image-x-generic"));
    _comboSource->setItemIcon(1,QIcon::fromTheme("video-x-generic"));
#endif
    if ( _source == Webcam ) {
      _source = StillImage;
      _currentSource = &_stillImage;
    }

  } else {
    _tabParams->setCurrentIndex(0);
    _comboSource->addItem("Webcam",QVariant(Webcam));
    _comboSource->addItem("Image",QVariant(StillImage));
    _comboSource->addItem("Video file",QVariant(Video));
#if QT_VERSION >= 0x040600
    _comboSource->setItemIcon(0,QIcon::fromTheme("camera-web"));
    _comboSource->setItemIcon(1,QIcon::fromTheme("image-x-generic"));
    _comboSource->setItemIcon(2,QIcon::fromTheme("video-x-generic"));
#endif
    _comboWebcam->setEnabled(camList.size() > 1);
    _cameraDefaultResolutionsIndexes.clear();
    for ( int iCam = 0; iCam < camList.size(); ++iCam ) {
      _comboWebcam->addItem( QString("Webcam %1").arg(camList[iCam]),QVariant(camList[iCam]) );
      QSize size = settings.value(QString("WebcamSource/DefaultResolutionCam%1").arg(iCam),QSize()).toSize();
      if ( size.isValid() && WebcamSource::webcamResolutions(iCam).contains(size) ) {
        _cameraDefaultResolutionsIndexes.push_back(WebcamSource::webcamResolutions(iCam).indexOf(size));
      } else {
        _cameraDefaultResolutionsIndexes.push_back(WebcamSource::webcamResolutions(iCam).size()-1);
      }
    }
    _comboWebcam->setCurrentIndex(0);
    connect(_comboWebcam,SIGNAL(currentIndexChanged(int)),
            this, SLOT(onWebcamComboChanged(int)));
    onWebcamComboChanged(0);
  }
  connect( _comboSource, SIGNAL(currentIndexChanged(int)),
           this, SLOT(onComboSourceChanged(int)));
  onComboSourceChanged(0);
}

