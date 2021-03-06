#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_desktop.h>
#include <xsi_status.h>

#include "FabricDFGPlugin.h"
#include "FabricDFGWidget.h"
#include "FabricDFGOperators.h"
#include "FabricDFGUICmdHandlerDCC.h"

#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QBoxLayout>
#include <QtGui/QPalette>

#ifdef _WIN32
  #include <windows.h>
  #include <io.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>

#include <DFG/DFGCombinedWidget.h>

using namespace XSI;

class FabricDFGWidget : public DFG::DFGCombinedWidget
{
 public:

  FabricDFGWidget(QWidget *parent) : DFGCombinedWidget(parent)
  {
  }

  virtual void onUndo()
  {
    Application().ExecuteCommand(L"Undo", CValueArray(), CValue());
  }

  virtual void onRedo()
  {
    Application().ExecuteCommand(L"Redo", CValueArray(), CValue());
  }

  static void log(void *userData, const char *message, unsigned int length)
  {
    std::string mess = std::string("[CANVAS] ") + (message ? message : "");
    feLog(userData, mess.c_str(), mess.length());
  }

  bool eventFilter(QObject *watched, QEvent *event)
  {
    if (event->type() == QEvent::KeyPress)
    {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
      switch (keyEvent->key())
      {
        case 89:  // redo.
        {
          Application().ExecuteCommand(L"Redo", CValueArray(), CValue());
          return true;
        } 
        case 90:  // undo.
        {
          Application().ExecuteCommand(L"Undo", CValueArray(), CValue());
          return true;
        } 
        default:
        {
          break;
        }
      }
    }
    return false;
  }
};

struct _windowData
{
  QBoxLayout      *qtLayout;
  FabricDFGWidget *qtDFGWidget;
  QDialog         *qtDialog;
  QDialog         *qtDock;

  _windowData(void)
  {
    qtLayout      = NULL;
    qtDFGWidget   = NULL;
    qtDialog      = NULL;
    qtDock        = NULL;
  }
};

const char *GetOpenCanvasErrorDescription(OPENCANVAS_RETURN_VALS in_errID)
{
  static const char str_SUCCESS     [] = "success";
  static const char str_ALREADY_OPEN[] = "did not open Canvas, because there already is an open Canvas window";
  static const char str_NULL_POINTER[] = "failed to open Canvas: a pointer is NULL";
  static const char str_UNKNOWN     [] = "failed to open Canvas: unknown error";
  switch (in_errID)
  {
    case OPENCANVAS_RETURN_VALS::SUCCESS:       return str_SUCCESS;
    case OPENCANVAS_RETURN_VALS::ALREADY_OPEN:  return str_ALREADY_OPEN;
    case OPENCANVAS_RETURN_VALS::NULL_POINTER:  return str_NULL_POINTER;
    default:                                    return str_UNKNOWN;
  }
}

OPENCANVAS_RETURN_VALS OpenCanvas(_opUserData *pud, const char *winTitle)
{
  // static flag to ensure that not more than one Canvas is open at the same time.
  static bool s_canvasIsOpen = false;

  // check.
  if (s_canvasIsOpen)
  {
    Application().LogMessage(L"Not opening Canvas... reason: there already is an open Canvas window.", siWarningMsg);
    return OPENCANVAS_RETURN_VALS::ALREADY_OPEN;
  }

  // init Qt app, if necessary.
  FabricInitQt();

  // check.
  if (!qApp)                    return OPENCANVAS_RETURN_VALS::NULL_POINTER;
  if (!pud)                     return OPENCANVAS_RETURN_VALS::NULL_POINTER;
  if (!pud->GetBaseInterface()) return OPENCANVAS_RETURN_VALS::NULL_POINTER;

  // set flag to block any further canvas.
  s_canvasIsOpen = true;

  // declare and fill window data structure.
  _windowData winData;
  try
  {
    winData.qtDock        = new QDialog         (NULL);
    winData.qtDialog      = new QDialog         (winData.qtDock);
    winData.qtDFGWidget   = new FabricDFGWidget (winData.qtDialog);
    winData.qtLayout      = new QVBoxLayout     (winData.qtDialog);

    winData.qtDialog->setWindowTitle(winTitle ? winTitle : "Canvas");
    winData.qtDialog->installEventFilter(winData.qtDFGWidget);
    winData.qtLayout->addWidget(winData.qtDFGWidget); 
    winData.qtLayout->setContentsMargins(0, 0, 0, 0);

    // parent the qtDock dialog to the Softimage main window.
    #ifdef _WIN32
      SetParent((HWND)winData.qtDock->winId(), (HWND)Application().GetDesktop().GetApplicationWindowHandle());
    #endif

    // disable mouse and keyboard input of the Softimage main window.
    #ifdef _WIN32
      EnableWindow((HWND)Application().GetDesktop().GetApplicationWindowHandle(), false);
    #endif

    // set config (i.e. colors and stuff) for the DFG widget.
    DFG::DFGConfig config;
    config.graphConfig.useOpenGL                = false;
    config.graphConfig.headerBackgroundColor    . setRgb(157, 156, 155);
    config.graphConfig.mainPanelBackgroundColor . setRgb(107, 107, 107);
    config.graphConfig.sidePanelBackgroundColor . setRgb(137, 136, 135);

    // init the DFG widget.
    winData.qtDFGWidget->init(*pud->GetBaseInterface()->getClient(),
                               pud->GetBaseInterface()->getManager(),
                               pud->GetBaseInterface()->getHost(),
                               pud->GetBaseInterface()->getBinding(),
                               "",
                               pud->GetBaseInterface()->getBinding().getExec(),
                               pud->GetBaseInterface()->getCmdHandler(),
                               false,
                               config
                             );

    // adjust some colors that can't be set using DFG::DFGConfig.
    winData.qtDFGWidget->getTreeWidget()    ->setStyleSheet("background-color: rgb(172,171,170); border: 1px solid black;");
    winData.qtDFGWidget->getDfgValueEditor()->setStyleSheet("background-color: rgb(182,181,180);");
    winData.qtDFGWidget->getDfgLogWidget()  ->setStyleSheet("background-color: rgb(160,160,160); border: 1px solid black;");
  }
  catch(std::exception &e)
  {
    feLogError(e.what() ? e.what() : "\"\"");
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // show/execute qtDialog (without showing the dock dialog qtDock).
  try
  {
    winData.qtDialog->exec();
  }
  catch(std::exception &e)
  {
    feLogError(e.what() ? e.what() : "\"\"");
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // clean up.
  winData.qtDock->close();
  try
  {
    delete winData.qtDFGWidget;
    delete winData.qtLayout;
    delete winData.qtDialog;
    delete winData.qtDock;
  }
  catch(std::exception &e)
  {
    feLogError(e.what() ? e.what() : "\"\"");
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // enable mouse and keyboard input of the Softimage main window.
  #ifdef _WIN32
    EnableWindow((HWND)Application().GetDesktop().GetApplicationWindowHandle(), true);
  #endif

  // reset or flag.
  s_canvasIsOpen = false;

  // done.
  return OPENCANVAS_RETURN_VALS::SUCCESS;
}

void FabricInitQt()
{
  if (!qApp)
  {
    Application().LogMessage(L"allocating an instance of QApplication", siCommentMsg);
    int argc = 0;
    new QApplication(argc, NULL);
  }
}
