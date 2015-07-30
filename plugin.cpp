#include <xsi_application.h>
#include <xsi_customoperator.h>
#include <xsi_projectitem.h>
#include <xsi_pluginregistrar.h>
#include <xsi_uitoolkit.h>
#include <xsi_plugin.h>

#include "plugin.h"
#include "FabricDFGBaseInterface.h"
#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"

#include "FabricSplicePlugin.h"
#include "FabricSpliceICENodes.h"

#include <FabricUI/DFG/DFGUICmd/DFGUICmds.h>

#include <QtGui/QApplication>

using namespace XSI;

#define REGISTER_DFGUICMD(inreg, CMD) \
  { \
    std::wstring cmdName( \
      FabricUI::DFG::DFGUICmd_##CMD::CmdName().begin(), \
      FabricUI::DFG::DFGUICmd_##CMD::CmdName().end() \
      ); \
    in_reg.RegisterCommand( cmdName.c_str(), cmdName.c_str() ); \
  }

// load plugin.
SICALLBACK XSILoadPlugin(PluginRegistrar& in_reg)
{
  // check if the environment variable FABRIC_DFG_PATH exists.
  {
    // get the environment variable's value.
    CString envVarName = L"FABRIC_DFG_PATH";
    char *envVarValue  = envVarValue = getenv(envVarName.GetAsciiString());

    // no value found?
    if (!envVarValue || envVarValue[0] == '\0')
    {
      // log error.
      #ifdef _WIN32
        CString fabric_dfg_path = L"<Fabric-Installation-Path>\\DFG\\Presets";
      #else
        CString fabric_dfg_path = L"<Fabric-Installation-Path>/DFG/Presets";
      #endif
      CString t1 = L"The environment variable " + envVarName + L" is not set!";
      CString t2 = L"Please make sure that " + envVarName + L" is set and points to \"" + fabric_dfg_path + L"\".";
      Application().LogMessage(L"[Fabric]: " + t1, siErrorMsg);
      Application().LogMessage(L"[Fabric]: " + t2, siErrorMsg);
    }
  }

  // set plugin's name, version and author.
  in_reg.PutAuthor(L"Fabric Engine");
  in_reg.PutName  (L"Fabric Engine Plugin");
  in_reg.PutVersion(FabricCore::GetVersionMaj(), FabricCore::GetVersionMin());

  // register the (old) Fabric Splice.
  {
    // rendering.
    in_reg.RegisterDisplayCallback(L"SpliceRenderPass");

    // properties.
    in_reg.RegisterProperty(L"SpliceInfo");
  
    // dialogs.
    in_reg.RegisterProperty(L"SpliceEditor");
    in_reg.RegisterProperty(L"ImportSpliceDialog");

    // operators.
    in_reg.RegisterOperator(L"SpliceOp");

    // commands.
    in_reg.RegisterCommand(L"fabricSplice",             L"fabricSplice");
    in_reg.RegisterCommand(L"fabricSpliceManipulation", L"fabricSpliceManipulation");
    in_reg.RegisterCommand(L"proceedToNextScene",       L"proceedToNextScene");

    // tools.
    in_reg.RegisterTool(L"fabricSpliceTool");

    // menu.
    in_reg.RegisterMenu(siMenuMainTopLevelID, "Fabric:Splice", true, true);

    // events.
    in_reg.RegisterEvent(L"FabricSpliceNewScene",       siOnEndNewScene);
    in_reg.RegisterEvent(L"FabricSpliceCloseScene",     siOnCloseScene);
    in_reg.RegisterEvent(L"FabricSpliceOpenBeginScene", siOnBeginSceneOpen);
    in_reg.RegisterEvent(L"FabricSpliceOpenEndScene",   siOnEndSceneOpen);
    in_reg.RegisterEvent(L"FabricSpliceSaveScene",      siOnBeginSceneSave);    //
    in_reg.RegisterEvent(L"FabricSpliceSaveAsScene",    siOnBeginSceneSaveAs);  //
    in_reg.RegisterEvent(L"FabricSpliceTerminate",      siOnTerminate);
    in_reg.RegisterEvent(L"FabricSpliceSaveScene2",     siOnBeginSceneSave2);   //
    in_reg.RegisterEvent(L"FabricSpliceImport",         siOnEndFileImport);     //
    in_reg.RegisterEvent(L"FabricSpliceBeginExport",    siOnBeginFileExport);   //
    in_reg.RegisterEvent(L"FabricSpliceEndExport",      siOnEndFileExport);     //
    in_reg.RegisterEvent(L"FabricSpliceValueChange",    siOnValueChange);       //

    // ice nodes.
    Register_spliceGetData(in_reg);
  }

  // register the (new) Fabric DFG/Canvas.
  {
    // operators.
    in_reg.RegisterOperator(L"dfgSoftimageOp");

    // commands.
    in_reg.RegisterCommand(L"dfgSoftimageOpApply",  L"dfgSoftimageOpApply");
    in_reg.RegisterCommand(L"dfgImportJSON",        L"dfgImportJSON");
    in_reg.RegisterCommand(L"dfgExportJSON",        L"dfgExportJSON");
    in_reg.RegisterCommand(L"dfgOpenCanvas",        L"dfgOpenCanvas");
    in_reg.RegisterCommand(L"dfgSelectConnected",   L"dfgSelectConnected");
    in_reg.RegisterCommand(L"dfgLogStatus",         L"dfgLogStatus");

    // commands for DFGUICmdHandler.
    REGISTER_DFGUICMD( in_reg, AddBackDrop );
    REGISTER_DFGUICMD( in_reg, AddFunc );
    REGISTER_DFGUICMD( in_reg, AddGet );
    REGISTER_DFGUICMD( in_reg, AddGraph );
    REGISTER_DFGUICMD( in_reg, AddPort );
    REGISTER_DFGUICMD( in_reg, AddSet );
    REGISTER_DFGUICMD( in_reg, AddVar );
    REGISTER_DFGUICMD( in_reg, Connect );
    REGISTER_DFGUICMD( in_reg, Disconnect );
    REGISTER_DFGUICMD( in_reg, ExplodeNode );
    REGISTER_DFGUICMD( in_reg, ImplodeNodes );
    REGISTER_DFGUICMD( in_reg, InstPreset );
    REGISTER_DFGUICMD( in_reg, MoveNodes );
    REGISTER_DFGUICMD( in_reg, Paste );
    REGISTER_DFGUICMD( in_reg, RenamePort );
    REGISTER_DFGUICMD( in_reg, RemoveNodes );
    REGISTER_DFGUICMD( in_reg, RemovePort );
    REGISTER_DFGUICMD( in_reg, ResizeBackDrop );
    REGISTER_DFGUICMD( in_reg, SetCode );
    REGISTER_DFGUICMD( in_reg, SetNodeComment );
    REGISTER_DFGUICMD( in_reg, SetNodeTitle );
    REGISTER_DFGUICMD( in_reg, SetRefVarPath );

    // menu.
    in_reg.RegisterMenu(siMenuMainTopLevelID,       L"Fabric:DFG", true, true);
  }

  // done.
  return CStatus::OK;
}

// unload plugin.
SICALLBACK XSIUnloadPlugin(const PluginRegistrar& in_reg)
{
  CString strPluginName;
  strPluginName = in_reg.GetName();
  Application().LogMessage(strPluginName + L" has been unloaded.", siVerboseMsg);

  // Qt.
  if (qApp)
    delete qApp;

  // done.
  return CStatus::OK;
}

// _________________________
// siEvent helper functions.
// -------------------------

CStatus helpFnct_siEventOpenSave(CRef &ctxt, int openSave)
{
  Context context(ctxt);

  /*
      openSave == 0: store DFG JSON in op's persistenceData parameter (e.g. before saving a scene).
      openSave == 1: set DFG JSON from op's persistenceData parameter (e.g. after loading a scene).
  */

  std::map <unsigned int, _opUserData *> &s_instances = *_opUserData::GetStaticMapOfInstances();
  for (std::map<unsigned int, _opUserData *>::iterator it = s_instances.begin(); it != s_instances.end(); it++)
  {
    // get pointer at _opUserData.
    _opUserData *pud = it->second;
    if (!pud) continue;

    // get the object ID of the dfgSoftimageOp operator to which the user data belongs to.
    unsigned int opObjID = it->first;

    // check if the user data has a base interface.
    if (!pud->GetBaseInterface())
    { Application().LogMessage(L"user data has no base interface", siWarningMsg);
      continue; }

    // get the operator from the object ID.
    ProjectItem pItem = Application().GetObjectFromID(opObjID);
    if (!pItem.IsValid())
    { Application().LogMessage(L"Application().GetObjectFromID(opObjID) failed", siWarningMsg);
      continue; }
    CustomOperator op(pItem.GetRef());
    if (!op.IsValid())
    { Application().LogMessage(L"CustomOperator op(pItem.GetRef()) failed", siWarningMsg);
      continue; }
    if (op.GetType() != L"dfgSoftimageOp")
    { Application().LogMessage(L"op.GetType() returned \"" + op.GetType() + L"\" instead of \"dfgSoftimageOp\"", siWarningMsg);
      continue; }

    // store JSON in parameter persistenceData.
    if (openSave == 0)
    {
      Application().LogMessage(L"storing DFG JSON for dfgSoftimageOp(opObjID = " + CString(op.GetObjectID()) + L")");
      try
      {
        CString dfgJSON = pud->GetBaseInterface()->getJSON().c_str();

        if (op.PutParameterValue(L"persistenceData", dfgJSON) != CStatus::OK)
        { Application().LogMessage(L"op.PutParameterValue(L\"persistenceData\") failed!", siWarningMsg);
          continue; }
      }
      catch (FabricCore::Exception e)
      {
        std::string s = std::string("failed: ") + (e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
        feLogError(s);
      }
    }

    // store JSON in parameter persistenceData.
    else if (openSave == 1)
    {
      Application().LogMessage(L"setting DFG JSON from dfgSoftimageOp(opObjID = " + CString(op.GetObjectID()) + L")");
      try
      {
        CString dfgJSON = op.GetParameterValue(L"persistenceData");
        if (dfgJSON.IsEmpty())
        { Application().LogMessage(L"op.GetParameterValue(L\"persistenceData\") returned an empty value.", siWarningMsg);
          continue; }

        pud->GetBaseInterface()->setFromJSON(dfgJSON.GetAsciiString());
      }
      catch (FabricCore::Exception e)
      {
        std::string s = std::string("failed: ") + (e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
        feLogError(s);
      }
    }

    // do nothing.
    else
    {
      continue;
    }
  }

  // done.
  // /note: we return 1 (i.e. "true") instead of CStatus::OK or else the event gets aborted).
  return 1;
}


