#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>

#include <xsi_command.h>
#include <xsi_comapihandler.h>
#include <xsi_argument.h>
#include <xsi_factory.h>
#include <xsi_uitoolkit.h>
#include <xsi_longarray.h>
#include <xsi_doublearray.h>
#include <xsi_math.h>
#include <xsi_customproperty.h>
#include <xsi_ppglayout.h>
#include <xsi_ppgeventcontext.h>
#include <xsi_menu.h>
#include <xsi_model.h>
#include <xsi_griddata.h>
#include <xsi_gridwidget.h>
#include <xsi_selection.h>
#include <xsi_project.h>
#include <xsi_port.h>
#include <xsi_portgroup.h>
#include <xsi_inputport.h>
#include <xsi_outputport.h>
#include <xsi_utils.h>
#include <xsi_matrix4.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_customoperator.h>
#include <xsi_operatorcontext.h>
#include <xsi_parameter.h>
#include <xsi_polygonmesh.h>
#include <xsi_value.h>
#include <xsi_matrix4f.h>
#include <xsi_primitive.h>

#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"
#include "FabricDFGBaseInterface.h"
#include "FabricDFGTools.h"

std::map <unsigned int, _opUserData *>  _opUserData::s_instances;
std::vector<_portMapping>               _opUserData::s_portmap_newOp;

using namespace XSI;

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Init(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"dfgSoftimageOp_Init()";
  if (FabricDFGPlugin_BETA) Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  Context ctxt(in_ctxt);
  CustomOperator op(ctxt.GetSource());

  // create user data.
  _opUserData *pud = new _opUserData(op.GetObjectID());
  ctxt.PutUserData((ULLONG)pud);

  // done.
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Term(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"dfgSoftimageOp_Term()";
  if (FabricDFGPlugin_BETA) Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  Context ctxt(in_ctxt);
  _opUserData *pud = (_opUserData *)((ULLONG)ctxt.GetUserData());

  // clean up.
  if (pud)
  {
    delete pud;
  }

  // done.
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Define(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"dfgSoftimageOp_Define()";
  if (FabricDFGPlugin_BETA) Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  Context ctxt(in_ctxt);
  CustomOperator op = ctxt.GetSource();
  Factory oFactory = Application().GetFactory();
  CRef oPDef;

  // create default parameter(s).
  oPDef = oFactory.CreateParamDef(L"FabricActive",        CValue::siBool,   siPersistable | siAnimatable | siKeyable, L"", L"", true, CValue(), CValue(), CValue(), CValue());
  op.AddParameter(oPDef, Parameter());
  oPDef = oFactory.CreateParamDef(L"persistenceData",     CValue::siString, siPersistable | siReadOnly,               L"", L"", "", "", "", "", "");
  op.AddParameter(oPDef, Parameter());
  oPDef = oFactory.CreateParamDef(L"verbose",             CValue::siBool,   siPersistable,                            L"", L"", false, CValue(), CValue(), CValue(), CValue());
  op.AddParameter(oPDef, Parameter());

  // create grid parameter for the list of DFG ports.
  op.AddParameter(oFactory.CreateGridParamDef("dfgPorts"), Parameter());

  // create exposed DFG parameters.
  CString exposedDFGParams = L"";
  {
    for (int i=0;i<_opUserData::s_portmap_newOp.size();i++)
    {
      _portMapping &pmap = _opUserData::s_portmap_newOp[i];

      if (pmap.mapType != DFG_PORT_MAPTYPE::XSI_PARAMETER)
        continue;

      CValue::DataType dt = CValue::siIUnknown;
      if      (pmap.dfgPortDataType == L"Boolean")  dt = CValue::siBool;

      else if (pmap.dfgPortDataType == L"Float32")  dt = CValue::siFloat;
      else if (pmap.dfgPortDataType == L"Float64")  dt = CValue::siDouble;

      else if (pmap.dfgPortDataType == L"SInt8")    dt = CValue::siInt1;
      else if (pmap.dfgPortDataType == L"SInt16")   dt = CValue::siInt2;
      else if (pmap.dfgPortDataType == L"SInt32")   dt = CValue::siInt4;
      else if (pmap.dfgPortDataType == L"SInt64")   dt = CValue::siInt8;

      else if (pmap.dfgPortDataType == L"UInt8")    dt = CValue::siUInt1;
      else if (pmap.dfgPortDataType == L"UInt16")   dt = CValue::siUInt2;
      else if (pmap.dfgPortDataType == L"UInt32")   dt = CValue::siUInt4;
      else if (pmap.dfgPortDataType == L"UInt64")   dt = CValue::siUInt8;

      if (dt == CValue::siIUnknown)
      { Application().LogMessage(L"The DFG port \"" + pmap.dfgPortName + "\" cannot be exposed as a XSI Parameter (data type \"" + pmap.dfgPortDataType + "\" not yet supported)" , siWarningMsg);
        continue; }

      Application().LogMessage(L"adding parameter \"" + pmap.dfgPortName + "\"" , siInfoMsg);
      oPDef = oFactory.CreateParamDef(pmap.dfgPortName, dt, siPersistable | siAnimatable | siKeyable, L"", L"", CValue(), CValue(), CValue(), 0, 1);
      op.AddParameter(oPDef, Parameter());
      exposedDFGParams += pmap.dfgPortName + L";";
    }
  }

  // add internal string parameter with all the exposed DFG port names.
  // (this is used in the layout function).
  oPDef = oFactory.CreateParamDef(L"exposedDFGParams", CValue::siString, siReadOnly + siPersistable, L"", L"", exposedDFGParams, "", "", "", "");
  op.AddParameter(oPDef, Parameter());

  // set default values.
  op.PutAlwaysEvaluate(false);
  op.PutDebug(0);

  // done.
  return CStatus::OK;
}

int dfgSoftimageOp_UpdateGridData_dfgPorts(CustomOperator &op)
{
  // get grid object.
  GridData grid((CRef)op.GetParameter("dfgPorts").GetValue());

  // get operator's user data and check if there are any DFG ports.
  std::vector<_portMapping> pmap;
  CString       err;
  if (!GetOperatorPortMapping(op, pmap, err))
  { 
    // clear grid data.
    grid = GridData();
    return 0;
  }

  // set amount, name and type of the grid columns.
  grid.PutRowCount(pmap.size());
  grid.PutColumnCount(4);
  grid.PutColumnLabel(0, "Name");         grid.PutColumnType(0, siColumnStandard);
  grid.PutColumnLabel(1, "Type");         grid.PutColumnType(1, siColumnStandard);
  grid.PutColumnLabel(2, "Mode");         grid.PutColumnType(2, siColumnStandard);
  grid.PutColumnLabel(3, "Type/Target");  grid.PutColumnType(3, siColumnStandard);

  // set data.
  for (int i=0;i<pmap.size();i++)
  {
    // name.
    CString name = pmap[i].dfgPortName;

    // type (= resolved data type).
    CString type = pmap[i].dfgPortDataType;

    // mode (= DFGPortType).
    CString mode;
    switch (pmap[i].dfgPortType)
    {
      case DFG_PORT_TYPE::IN:   mode = L"In";         break;
      case DFG_PORT_TYPE::OUT:  mode = L"Out";        break;
      default:                  mode = L"undefined";  break;
    }

    // target.
    CString target;
    switch (pmap[i].mapType)
    {
      case DFG_PORT_MAPTYPE::INTERNAL:        target = L"Internal";       break;
      case DFG_PORT_MAPTYPE::XSI_PARAMETER:   target = L"XSI Parameter";  break;
      case DFG_PORT_MAPTYPE::XSI_PORT:      { target = L"XSI Port";
                                              if (pmap[i].mapTarget.IsEmpty())  target += "  ( - )";
                                              else                              target += L" ( -> " + pmap[i].mapTarget + L" )";
                                            }                             break;
      case DFG_PORT_MAPTYPE::XSI_ICE_PORT:    target = L"XSI ICE Port";   break;
      default:                                target = L"unknown";        break;
    }
    
    // set grid data.
    grid.PutRowLabel( i, L"  " + CString(i) + L"  ");
    grid.PutCell(0,   i, name);
    grid.PutCell(1,   i, type);
    grid.PutCell(2,   i, mode);
    grid.PutCell(3,   i, target);
  }

  // return amount of rows.
  return pmap.size();
}

void dfgSoftimageOp_DefineLayout(PPGLayout &oLayout, CustomOperator &op)
{
  // init.
  oLayout.Clear();
  oLayout.PutAttribute(siUIDictionary, L"None");
  PPGItem pi;

  // button size (Open Canvas).
  const LONG btnCx = 100;
  const LONG btnCy = 28;

  // button size (Refresh).
  const LONG btnRx = 93;
  const LONG btnRy = 28;

  // button size (tools).
  const LONG btnTx = 112;
  const LONG btnTy = 24;

  // button size (tools - select connected).
  const LONG btnTSCx = 90;
  const LONG btnTSCy = 22;

  // update the grid data.
  const int dfgPortsNumRows = dfgSoftimageOp_UpdateGridData_dfgPorts(op);

  // get the names of all DFG ports that are available as XSI parameters.
  CStringArray exposedDFGParams = CString(op.GetParameterValue(L"exposedDFGParams")).Split(L";");

  // Tab "Main"
  {
    oLayout.AddTab(L"Main");
    {
      oLayout.AddGroup(L"", false);
        oLayout.AddRow();
          oLayout.AddItem(L"FabricActive", L"Execute Fabric DFG");
          oLayout.AddSpacer(0, 0);
          pi = oLayout.AddButton(L"BtnOpenCanvas", L"Open Canvas");
          pi.PutAttribute(siUICX, btnCx);
          pi.PutAttribute(siUICY, btnCy);
        oLayout.EndRow();
      oLayout.EndGroup();
      oLayout.AddSpacer(0, 12);

      oLayout.AddGroup(L"Parameters");
        bool hasParams = (exposedDFGParams.GetCount() > 0);
        for (int i=0;i<exposedDFGParams.GetCount();i++)
        {
          oLayout.AddItem(exposedDFGParams[i]);
        }
        if (exposedDFGParams.GetCount() <= 0)
        {
          oLayout.AddSpacer(0, 8);
          oLayout.AddStaticText(L"   ... no Parameters available.");
          oLayout.AddSpacer(0, 16);
        }
      oLayout.EndGroup();
    }
  }

  // Tab "Ports and Tools"
  {
    oLayout.AddTab(L"Ports and Tools");
    {
      oLayout.AddGroup(L"", false);
        oLayout.AddRow();
          oLayout.AddItem(L"FabricActive", L"Execute Fabric DFG");
          oLayout.AddSpacer(0, 0);
          pi = oLayout.AddButton(L"BtnOpenCanvas", L"Open Canvas");
          pi.PutAttribute(siUICX, btnCx);
          pi.PutAttribute(siUICY, btnCy);
        oLayout.EndRow();
      oLayout.EndGroup();
      oLayout.AddSpacer(0, 12);

      oLayout.AddGroup(L"DFG Ports");
        oLayout.AddRow();
          oLayout.AddSpacer(0, 0);
          pi = oLayout.AddButton(L"BtnUpdatePPG", L"Update UI");
          pi.PutAttribute(siUICX, btnRx);
          pi.PutAttribute(siUICY, btnRy);
        oLayout.EndRow();
        oLayout.AddSpacer(0, 6);
        if (dfgPortsNumRows)
        {
          pi = oLayout.AddItem("dfgPorts", "dfgPorts", siControlGrid);
          pi.PutAttribute(siUINoLabel,              true);
          pi.PutAttribute(siUIWidthPercentage,      100);
          pi.PutAttribute(siUIGridLockRowHeader,    false);
          pi.PutAttribute(siUIGridSelectionMode,    siSelectionCell);
          pi.PutAttribute(siUIGridReadOnlyColumns,  "1:1:1:1:1");
        }
        else
        {
          oLayout.AddStaticText(L"   ... no Ports available.");
          oLayout.AddSpacer(0, 8);
        }
      oLayout.EndGroup();
      oLayout.AddSpacer(0, 4);

      oLayout.AddGroup(L" ");

        oLayout.AddGroup(L"Port Definitions and Connections");
          oLayout.AddRow();
            oLayout.AddGroup(L"", false);
              pi = oLayout.AddButton(L"BtnPortsDefineTT", L"Define Type/Target");
              pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0);
              pi.PutAttribute(siUICX, btnTx);
              pi.PutAttribute(siUICY, btnTy);
            oLayout.EndGroup();
            oLayout.AddGroup(L"", false);
              pi = oLayout.AddButton(L"BtnPortConnect", L"Connect");
              pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0);
              pi.PutAttribute(siUICX, btnTx);
              pi.PutAttribute(siUICY, btnTy);
              pi = oLayout.AddButton(L"BtnPortDisconnect", L"Disconnect");
              pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0);
              pi.PutAttribute(siUICX, btnTx);
              pi.PutAttribute(siUICY, btnTy);
            oLayout.EndGroup();
          oLayout.EndRow();
        oLayout.EndGroup();
        oLayout.AddSpacer(0, 8);

        oLayout.AddGroup(L"Select connected Objects");
          oLayout.AddRow();
            pi = oLayout.AddButton(L"BtnSelConnectAll", L"All");
            pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0);
            pi.PutAttribute(siUICX, btnTSCx);
            pi.PutAttribute(siUICY, btnTSCy);
            pi = oLayout.AddButton(L"BtnSelConnectIn",  L"Inputs");
            pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0);
            pi.PutAttribute(siUICX, btnTSCx);
            pi.PutAttribute(siUICY, btnTSCy);
            pi = oLayout.AddButton(L"BtnSelConnectOut", L"Outputs");
            pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0);
            pi.PutAttribute(siUICX, btnTSCx);
            pi.PutAttribute(siUICY, btnTSCy);
          oLayout.EndRow();
        oLayout.EndGroup();
        oLayout.AddSpacer(0, 8);

        oLayout.AddGroup(L"File");
          oLayout.AddRow();
            pi = oLayout.AddButton(L"BtnImportJSON", L"Import JSON");
            pi.PutAttribute(siUICX, btnTx);
            pi.PutAttribute(siUICY, btnTy);
            pi = oLayout.AddButton(L"BtnExportJSON", L"Export JSON");
            pi.PutAttribute(siUICX, btnTx);
            pi.PutAttribute(siUICY, btnTy);
          oLayout.EndRow();
        oLayout.EndGroup();
        oLayout.AddSpacer(0, 8);

        oLayout.AddGroup(L"Miscellaneous");
          oLayout.AddRow();
            pi = oLayout.AddButton(L"BtnLogDFGInfo", L"Log DFG Info");
            pi.PutAttribute(siUICX, btnTx);
            pi.PutAttribute(siUICY, btnTy);
            pi = oLayout.AddButton(L"BtnLogDFGJSON", L"Log DFG JSON");
            pi.PutAttribute(siUICX, btnTx);
            pi.PutAttribute(siUICY, btnTy);
          oLayout.EndRow();
        oLayout.EndGroup();

      oLayout.EndGroup();
      oLayout.AddSpacer(0, 4);
    }
  }

  // Tab "Advanced"
  {
    oLayout.AddTab(L"Advanced");
    {
      oLayout.AddItem(L"verbose",             L"Verbose");
      oLayout.AddItem(L"persistenceData",     L"persistenceData");
      oLayout.AddItem(L"mute",                L"Mute");
      oLayout.AddItem(L"alwaysevaluate",      L"Always Evaluate");
      oLayout.AddItem(L"debug",               L"Debug");
      oLayout.AddItem(L"Name",                L"Name");
    }
  }
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_PPGEvent(const CRef &in_ctxt)
{
  /*  note:

      if the value of a parameter changes but the UI is not shown then this
      code will not execute. Also this code is not re-entrant, so any changes
      to parameters inside this code will not result in further calls to this function.
  */

  // init.
  PPGEventContext ctxt(in_ctxt);
  CustomOperator op(ctxt.GetSource());
  PPGEventContext::PPGEvent eventID = ctxt.GetEventID();
  UIToolkit toolkit = Application().GetUIToolkit();

  // process event.
  if (eventID == PPGEventContext::siOnInit)
  {
    dfgSoftimageOp_DefineLayout(op.GetPPGLayout(), op);
    ctxt.PutAttribute(L"Refresh", true);
  }
  else if (eventID == PPGEventContext::siButtonClicked)
  {
    CString btnName = ctxt.GetAttribute(L"Button").GetAsText();
    if (btnName.IsEmpty())
    {
    }
    else if (btnName == L"BtnOpenCanvas")
    {
      CString title = L"Canvas - " + op.GetParent3DObject().GetName();

      Application().LogMessage(L"invoke OpenCanvas()");
      OpenCanvas(_opUserData::GetUserData(op.GetObjectID()), title.GetAsciiString());
      Application().LogMessage(L"done with OpenCanvas()");

    }
    else if (btnName == L"BtnPortsDefineTT")
    {
      CString err;
      if (!GetOperatorPortMapping(op, _opUserData::s_portmap_newOp, err))
      { Application().LogMessage(L"GetOperatorPortMapping() failed, err = \"" + err + L"\"", siErrorMsg);
        _opUserData::s_portmap_newOp.clear();
        return CStatus::OK; }
      if (_opUserData::s_portmap_newOp.size())
      {
        //
        if (Dialog_DefinePortMapping(_opUserData::s_portmap_newOp) <= 0)
        { _opUserData::s_portmap_newOp.clear();
          return CStatus::OK; }

        //
        CString dfgJSON;
        _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
        if (   pud
            && pud->GetBaseInterface())
        {
          try
          {
            std::string json = pud->GetBaseInterface()->getJSON();
            dfgJSON = json.c_str();
          }
          catch (FabricCore::Exception e)
          {
            feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
          }
        }

        dfgTool_ExecuteCommand(L"dfgSoftimageOpApply", op.GetParent3DObject().GetFullName(), dfgJSON, true);

        dfgTool_ExecuteCommand(L"DeleteObj", op.GetFullName());
      }
    }
    else if (   btnName == L"BtnPortConnect"
             || btnName == L"BtnPortDisconnect")
    {
      LONG ret;

      // get grid and its widget.
      GridData   g((CRef)op.GetParameter("dfgPorts").GetValue());
      if (!g.IsValid())
      { Application().LogMessage(L"failed to get grid data.", siWarningMsg);
        return CStatus::OK; }
      GridWidget w = g.GetGridWidget();
      if (!w.IsValid())
      { Application().LogMessage(L"failed to get grid widget.", siWarningMsg);
        return CStatus::OK; }

      // get selected row.
      int selRow = -1;
      for (int j=0;j<g.GetRowCount();j++)
        for (int i=0;i<g.GetColumnCount();i++)
          if (w.IsCellSelected(i, j))
          {
            if (selRow < 0) { selRow = j;
                              break; }
            else            { toolkit.MsgBox(L"More than one DFG port selected.\n\nPlease select a single port and try again.", siMsgOkOnly, "dfgSoftimageOp", ret);
                              return CStatus::OK; }
          }
      if (selRow < 0)
      { toolkit.MsgBox(L"No DFG port selected.\n\nPlease select a single port and try again.", siMsgOkOnly, "dfgSoftimageOp", ret);
        return CStatus::OK; }

      // get the name of the selected port as well as its port mapping.
      CString selPortName = g.GetCell(0, selRow);
      _portMapping pmap;
      {
        std::vector<_portMapping> tmp;
        CString err;
        GetOperatorPortMapping(op, tmp, err);
        for (int i=0;i<tmp.size();i++)
          if (selPortName == tmp[i].dfgPortName)
          {
            pmap = tmp[i];
            break;
          }
      }
      if (pmap.dfgPortName != selPortName)
      { toolkit.MsgBox(L"Failed to find selected port.", siMsgOkOnly | siMsgExclamation, "dfgSoftimageOp", ret);
        return CStatus::OK; }
      if (   pmap.dfgPortType != DFG_PORT_TYPE::IN
          && pmap.dfgPortType != DFG_PORT_TYPE::OUT)
      { toolkit.MsgBox(L"Selected port has unsupported type (neither \"In\" nor \"Out\").", siMsgOkOnly | siMsgExclamation, "dfgSoftimageOp", ret);
        return CStatus::OK; }
      if (pmap.mapType != DFG_PORT_MAPTYPE::XSI_PORT)
      { toolkit.MsgBox(L"Selected port Type/Target is not \"XSI Port\".", siMsgOkOnly, "dfgSoftimageOp", ret);
        return CStatus::OK; }

      // find the port group.
      PortGroup portgroup;
      CRefArray pgRef = op.GetPortGroups();
      for (int i=0;i<pgRef.GetCount();i++)
      {
        PortGroup pg(pgRef[i]);
        if (   pg.IsValid()
            && pg.GetName() == pmap.dfgPortName)
          {
            portgroup.SetObject(pgRef[i]);
            break;
          }
      }
      if (!portgroup.IsValid())
      { toolkit.MsgBox(L"Unable to find matching port group.", siMsgOkOnly | siMsgExclamation, "dfgSoftimageOp", ret);
        return CStatus::OK; }

      // pick target.
      CRef targetRef;
      if (btnName == L"BtnPortConnect")
      {
        CValueArray args(7);
        args[0] = siGenericObjectFilter;
        args[1] = L"Pick";
        args[2] = L"Pick";
        args[5] = 0;
        if (Application().ExecuteCommand(L"PickElement", args, CValue()) == CStatus::Fail)
        { Application().LogMessage(L"PickElement failed.", siWarningMsg);
          return CStatus::OK; }
        if ((LONG)args[4] == 0)
        { Application().LogMessage(L"canceled by user.", siWarningMsg);
          return CStatus::OK; }
        targetRef = args[3];
      }

      // check/correct target's siClassID and CRef.
      if (btnName == L"BtnPortConnect")
      {
        siClassID portClassID = GetSiClassIdFromResolvedDataType(pmap.dfgPortDataType);
        if (targetRef.GetClassID() != portClassID)
        {
          bool err = true;

          // kinematics?
          if (portClassID == siKinematicStateID)
          {
            CRef tmp;
            tmp.Set(targetRef.GetAsText() + L".kine.global");
            if (tmp.IsValid())
            {
              targetRef = tmp;
              err = false;
            }
          }

          // polygon mesh?
          if (portClassID == siPolygonMeshID)
          {
            CRef tmp;
            tmp.Set(targetRef.GetAsText() + L".polymsh");
            if (tmp.IsValid())
            {
              targetRef = tmp;
              err = false;
            }
          }

          //
          if (err)
          { Application().LogMessage(L"the picked element has the type \"" + targetRef.GetClassIDName() + L"\", but the port needs the type \"" + GetSiClassIdDescription(portClassID, CString()) + L"\".", siErrorMsg);
            return CStatus::OK; }
        }
      }

      // disconnect any existing connection.
      while (portgroup.GetInstanceCount() > 0)
      {
        if (op.DisconnectGroup(portgroup.GetIndex(), 0, true) != CStatus::OK)
        { Application().LogMessage(L"op.DisconnectGroup() failed.", siWarningMsg);
          break; }
      }

      // connect.
      if (btnName == L"BtnPortConnect")
      {
        Application().LogMessage(L"connecting \"" + targetRef.GetAsText() + L"\".", siInfoMsg);
        LONG instance;
        if (op.ConnectToGroup(portgroup.GetIndex(), targetRef, instance) != CStatus::OK)
        { Application().LogMessage(L"failed to connect \"" + targetRef.GetAsText() + "\"", siErrorMsg);
          return CStatus::OK; }
      }

      // refresh layout.
      dfgSoftimageOp_DefineLayout(op.GetPPGLayout(), op);
      ctxt.PutAttribute(L"Refresh", true);
    }
    else if (btnName == L"BtnSelConnectAll")
    {
      dfgTool_ExecuteCommand(L"dfgSelectConnected", op.GetUniqueName(), (LONG)0);
    }
    else if (btnName == L"BtnSelConnectIn")
    {
      dfgTool_ExecuteCommand(L"dfgSelectConnected", op.GetUniqueName(), (LONG)-1);
    }
    else if (btnName == L"BtnSelConnectOut")
    {
      dfgTool_ExecuteCommand(L"dfgSelectConnected", op.GetUniqueName(), (LONG)+1);
    }
    else if (btnName == L"BtnImportJSON")
    {
      //LONG ret;
      //toolkit.MsgBox(L"Importing DFG Presets is not possible via the property page.\n\nPlease use the menu \"Fabric:DFG -> Import JSON\"\nor the custom command \"dfgImportJSON\" instead.", siMsgOkOnly | siMsgInformation, "dfgSoftimageOp", ret);

      // open file browser.
      CString fileName;
      if (!dfgTool_FileBrowserJSON(false, fileName))
      { Application().LogMessage(L"aborted by user.", siWarningMsg);
        return CStatus::OK; }

      // execute command.
      dfgTool_ExecuteCommand(L"dfgImportJSON", op.GetUniqueName(), fileName);

      //
      dfgSoftimageOp_DefineLayout(op.GetPPGLayout(), op);
      ctxt.PutAttribute(L"Refresh", true);
    }
    else if (btnName == L"BtnExportJSON")
    {
      // open file browser.
      CString fileName;
      if (!dfgTool_FileBrowserJSON(true, fileName))
      { Application().LogMessage(L"aborted by user.", siWarningMsg);
        return CStatus::OK; }

      // export.
      dfgTool_ExecuteCommand(L"dfgExportJSON", op.GetUniqueName(), fileName);
    }
    else if (btnName == L"BtnUpdatePPG")
    {
      dfgSoftimageOp_DefineLayout(op.GetPPGLayout(), op);
      ctxt.PutAttribute(L"Refresh", true);
    }
    else if (btnName == L"BtnLogDFGInfo")
    {
      try
      {
        // init/check.
        _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
        if (!pud)                                 { Application().LogMessage(L"no user data found!",      siErrorMsg);  return CStatus::OK; }
        if (!pud->GetBaseInterface())             { Application().LogMessage(L"no base interface found!", siErrorMsg);  return CStatus::OK; }
        if (!pud->GetBaseInterface()->getGraph()) { Application().LogMessage(L"no graph found!",          siErrorMsg);  return CStatus::OK; }

        // log intro.
        Application().LogMessage(L"\"" + op.GetRef().GetAsText() + L"\" (ObjectID = " + CString(op.GetObjectID()) + L")", siInfoMsg);
        Application().LogMessage(L"{", siInfoMsg);

        // log (DFG ports).
        FabricServices::DFGWrapper::ExecPortList ports = pud->GetBaseInterface()->getGraph()->getPorts();
        Application().LogMessage(L"    amount of DFG ports: " + CString((LONG)ports.size()), siInfoMsg);
        for (int i=0;i<ports.size();i++)
        {
          FabricServices::DFGWrapper::ExecPort &port = *ports[i];
          char s[16];
          sprintf(s, "%03ld", i);
          CString t;
          t  = L"        " + CString(s) + L". ";
          if      (port.getOutsidePortType() == FabricCore::DFGPortType_In)  t += L"type = \"In\",  ";
          else if (port.getOutsidePortType() == FabricCore::DFGPortType_Out) t += L"type = \"Out\", ";
          else                                                               t += L"type = \"IO\",  ";
          t += L"name = \"" + CString(port.getName()) + L"\", ";
          t += L"resolved data type = \"" + CString(port.getResolvedType()) + L"\"";
          Application().LogMessage(t, siInfoMsg);
        }

        // log (XSI port groups).
        LONG sumNumPortsInGroups = 0;
        CRefArray pGroups = op.GetPortGroups();
        Application().LogMessage(L" ", siInfoMsg);
        Application().LogMessage(L"    amount of XSI port groups: " + CString(pGroups.GetCount()), siInfoMsg);
        for (int i=0;i<pGroups.GetCount();i++)
        {
          PortGroup pg(pGroups[i]);

          char s[16];
          sprintf(s, "%03ld", i);
          CString t;
          t  = L"        " + CString(s) + L". ";
          t += L"name = \"" + pg.GetName() + L"\",  ";
          t += L"index = " + CString(pg.GetIndex()) + L",  ";
          t += L"num instances = " + CString(pg.GetInstanceCount()) + L",  ";
          LONG numPorts = pg.GetPorts().GetCount();
          t += L"num ports = " + CString(numPorts) + L",  ";
          Application().LogMessage(t, siInfoMsg);

          sumNumPortsInGroups += numPorts;
        }

        // log (XSI ports).
        CRefArray inPorts  = op.GetInputPorts();
        CRefArray outPorts = op.GetOutputPorts();
        LONG numPorts = inPorts.GetCount() + outPorts.GetCount();
        Application().LogMessage(L" ", siInfoMsg);
        Application().LogMessage(L"    amount of XSI ports: " + CString(numPorts) + L"(" + CString(inPorts.GetCount()) + L" In and " + CString(outPorts.GetCount()) + L" Out)", siInfoMsg);
        for (int i=0;i<inPorts.GetCount();i++)
        {
          InputPort port(inPorts[i]);
          char s[16];
          sprintf(s, "% 3ld", i);
          CString t;
          t  = L"  " + CString(s) + L". ";
          t += L"type = \"In\",  ";
          t += L"name = \"" + port.GetName() + L"\", ";
          t += L"target = \"" + port.GetTarget().GetAsText() + L"\", ";
          t += L"target classID = \"" + port.GetTarget().GetClassIDName() + L"\"";
          Application().LogMessage(t, siInfoMsg);
        }
        for (int i=0;i<outPorts.GetCount();i++)
        {
          OutputPort port(outPorts[i]);
          char s[16];
          sprintf(s, "% 3ld", i + inPorts.GetCount());
          CString t;
          t  = L"  " + CString(s) + L". ";
          t += L"type = \"Out\", ";
          t += L"name = \"" + port.GetName() + L"\", ";
          t += L"target = \"" + port.GetTarget().GetAsText() + L"\", ";
          t += L"target classID = \"" + port.GetTarget().GetClassIDName() + L"\"";
          Application().LogMessage(t, siInfoMsg);
        }
      }
      catch (FabricCore::Exception e)
      {
        feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
      }
    }
    else if (btnName == L"BtnLogDFGJSON")
    {
      dfgTool_ExecuteCommand(L"dfgExportJSON", op.GetUniqueName(), L"console");
    }
  }
  else if (eventID == PPGEventContext::siTabChange)
  {
  }
  else if (eventID == PPGEventContext::siParameterChange)
  {
  }

  // done.
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Update(CRef &in_ctxt)
{
  // init.
  OperatorContext ctxt(in_ctxt);
  CustomOperator op(ctxt.GetSource());
  _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
  if (!pud)                                     { Application().LogMessage(L"no user data found!", siErrorMsg);
                                                  return CStatus::OK; }
  if (!pud->GetBaseInterface())                 { Application().LogMessage(L"no base interface found!", siErrorMsg);
                                                  return CStatus::OK; }
  if (!pud->GetBaseInterface()->getBinding())   { Application().LogMessage(L"no binding found!", siErrorMsg);
                                                  return CStatus::OK; }
  if (!pud->GetBaseInterface()->getGraph())     { Application().LogMessage(L"no graph found!", siErrorMsg);
                                                  return CStatus::OK; }
  // init log.
  CString functionName = L"dfgSoftimageOp_Update(opObjID = " + CString(op.GetObjectID()) + L")";
  const bool verbose = (bool)ctxt.GetParameterValue(L"verbose");
  if (verbose)  Application().LogMessage(functionName + L" called #" + CString(pud->updateCounter), siInfoMsg);
  pud->updateCounter++;

  // check the FabricActive parameter.
  if (!(bool)ctxt.GetParameterValue(L"FabricActive"))
    return CStatus::OK;

  // get the output port that is currently being evaluated.
  OutputPort outputPort(ctxt.GetOutputPort());
  if (verbose) Application().LogMessage(functionName + L": evaluating output port \"" + outputPort.GetName() + L"\"");

  // get pointers/refs at binding, graph & co.
  BaseInterface                                   *baseInterface  = pud->GetBaseInterface();
  FabricCore::Client                              *client         = pud->GetBaseInterface()->getClient();
  FabricServices::DFGWrapper::Binding             *binding        = pud->GetBaseInterface()->getBinding();
  FabricServices::DFGWrapper::GraphExecutablePtr   graph          = pud->GetBaseInterface()->getGraph();

  // Fabric Engine (step 1): loop through all the DFG's input ports and set
  //                         their values from the matching XSI ports or parameters.
  {
    try
    {
      FabricServices::DFGWrapper::ExecPortList ports = graph->getPorts();
      for (int i=0;i<ports.size();i++)
      {
        bool done = false;

        // get/check DFG port.
        FabricServices::DFGWrapper::ExecPortPtr port = ports[i];
        if (port->getOutsidePortType() != FabricCore::DFGPortType_In)
          continue;

        // find a matching XSI port.
        if (!done)
        {
          CValue xsiPortValue = op.GetInputValue(CString(port->getName()), CString(port->getName()));
          if (!xsiPortValue.IsEmpty())
          {
            done = true;

            //
            if (verbose) Application().LogMessage(functionName + L": transfer xsi port data to dfg port \"" + CString(port->getName()) + L"\"");

            if      (CString(port->getResolvedType()) == L"")
            {
              // do nothing.
            }
            else if (CString(port->getResolvedType()) == L"Mat44")
            {
              if (xsiPortValue.m_t == CValue::siRef)
              {
                KinematicState ks(xsiPortValue);
                if (ks.IsValid())
                {
                  // put the XSI port's value into a std::vector.
                  MATH::CMatrix4 m = ks.GetTransform().GetMatrix4();
                  std::vector <double> val(16);
                  val[ 0] = m.GetValue(0, 0); // row 0.
                  val[ 1] = m.GetValue(1, 0);
                  val[ 2] = m.GetValue(2, 0);
                  val[ 3] = m.GetValue(3, 0);
                  val[ 4] = m.GetValue(0, 1); // row 1.
                  val[ 5] = m.GetValue(1, 1);
                  val[ 6] = m.GetValue(2, 1);
                  val[ 7] = m.GetValue(3, 1);
                  val[ 8] = m.GetValue(0, 2); // row 2.
                  val[ 9] = m.GetValue(1, 2);
                  val[10] = m.GetValue(2, 2);
                  val[11] = m.GetValue(3, 2);
                  val[12] = m.GetValue(0, 3); // row 3.
                  val[13] = m.GetValue(1, 3);
                  val[14] = m.GetValue(2, 3);
                  val[15] = m.GetValue(3, 3);

                  // set the DFG port from the std::vector.
                  BaseInterface::SetValueOfPortMat44(*client, *binding, port, val);
                }
              }
            }

            //
            else if (CString(port->getResolvedType()) == L"PolygonMesh")
            {
              Application().LogMessage(L"input PolygonMesh ports not yet implemented.", siWarningMsg);
              if (xsiPortValue.m_t == CValue::siRef)
              {
                // todo: set DFG port's polygon mesh from XSI port's polygon mesh.
              }
            }
          }
        }

        // find a matching XSI parameter.
        if (!done)
        {
          Parameter xsiParam = op.GetParameter(port->getName());
          if (xsiParam.IsValid())
          {
            done = true;

            //
            if (verbose) Application().LogMessage(functionName + L": transfer xsi parameter data to dfg port \"" + CString(port->getName()) + L"\"");

            //
            CValue xsiValue = xsiParam.GetValue();

            //
            std::string port__resolvedType = port->getResolvedType();
            if      (   port__resolvedType == "Boolean")    {
                                                              bool val = (bool)xsiValue;
                                                              BaseInterface::SetValueOfPortBoolean(*client, *binding, port, val);
                                                            }
            else if (   port__resolvedType == "SInt8"
                     || port__resolvedType == "SInt16"
                     || port__resolvedType == "SInt32"
                     || port__resolvedType == "SInt64" )    {
                                                              int val = (int)(LONG)xsiValue;
                                                              BaseInterface::SetValueOfPortSInt(*client, *binding, port, val);
                                                            }
            else if (   port__resolvedType == "UInt8"
                     || port__resolvedType == "UInt16"
                     || port__resolvedType == "UInt32"
                     || port__resolvedType == "UInt64" )    {
                                                              unsigned int val = (unsigned int)(ULONG)xsiValue;
                                                              BaseInterface::SetValueOfPortUInt(*client, *binding, port, val);
                                                            }
            else if (   port__resolvedType == "Float32"
                     || port__resolvedType == "Float64" )   {
                                                              double val = (double)xsiValue;
                                                              BaseInterface::SetValueOfPortFloat(*client, *binding, port, val);
                                                            }
            else
            {
              Application().LogMessage(L"the port \"" + CString(port->getName()) + L"\" has the unsupported data type \"" + CString(port__resolvedType.c_str()) + L"\"", siWarningMsg)  ;
            }
          }
        }
      }
    }
    catch (FabricCore::Exception e)
    {
      std::string s = functionName.GetAsciiString() + std::string("(step 1): ") + (e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
      feLogError(s);
    }
  }

  // Fabric Engine (step 2): execute the DFG.
  {
    try
    {
      binding->execute();
    }
    catch (FabricCore::Exception e)
    {
      std::string s = functionName.GetAsciiString() + std::string("(step 2): ") + (e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
      feLogError(s);
    }
  }

  // Fabric Engine (step 3): set the current XSI output port from the matching DFG output port.
  {
    try
    {
      if (baseInterface->HasOutputPort(outputPort.GetName().GetAsciiString()))
      {
        if (verbose) Application().LogMessage(functionName + L": transfer dfg port data to xsi port \"" + outputPort.GetName() + L"\"");
        FabricServices::DFGWrapper::ExecPortPtr port = graph->getPort(outputPort.GetName().GetAsciiString());
        if (port.isNull())
          Application().LogMessage(functionName + L": graph->getPort() == NULL", siWarningMsg);
        else
        {
          if      (outputPort.GetTarget().GetClassID() == siUnknownClassID)
          {
            // do nothing.
          }
          else if (outputPort.GetTarget().GetClassID() == siKinematicStateID)
          {
            std::vector <double> val;
            if (BaseInterface::GetPortValueMat44(port, val) != 0)
              Application().LogMessage(functionName + L": BaseInterface::GetPortValueMat44(port) failed.", siWarningMsg);
            else
            {
              KinematicState kineOut(ctxt.GetOutputTarget());
              MATH::CTransformation t;
              MATH::CMatrix4 m;
              m.SetValue(0, 0, val[ 0]); // row 0.
              m.SetValue(1, 0, val[ 1]);
              m.SetValue(2, 0, val[ 2]);
              m.SetValue(3, 0, val[ 3]);
              m.SetValue(0, 1, val[ 4]); // row 1.
              m.SetValue(1, 1, val[ 5]);
              m.SetValue(2, 1, val[ 6]);
              m.SetValue(3, 1, val[ 7]);
              m.SetValue(0, 2, val[ 8]); // row 2.
              m.SetValue(1, 2, val[ 9]);
              m.SetValue(2, 2, val[10]);
              m.SetValue(3, 2, val[11]);
              m.SetValue(0, 3, val[12]); // row 3.
              m.SetValue(1, 3, val[13]);
              m.SetValue(2, 3, val[14]);
              m.SetValue(3, 3, val[15]);
              t.SetMatrix4(m);
              kineOut.PutTransform(t);
            }
          }
          else if (outputPort.GetTarget().GetClassID() == siPrimitiveID)
          {
            PolygonMesh xsiPolymesh(Primitive(outputPort.GetTarget()).GetGeometry());
            if (!xsiPolymesh.IsValid())
              Application().LogMessage(L"PolygonMesh(Primitive(outputPort.GetTarget()).GetGeometry()) failed", siErrorMsg);
            else
            {
              _polymesh polymesh;

              int ret = polymesh.setFromDFGPort(port);
              if (ret)
                Application().LogMessage(functionName + L": failed to get mesh from DFG port \"" + CString(port->getName()) + L"\" (returned " + CString(ret) + L")", siWarningMsg);
              else
              {
                if (verbose)
                  Application().LogMessage(L"DFG port PolygonMesh: " + CString((LONG)polymesh.numVertices) + L" vertices, " + CString((LONG)polymesh.numPolygons) + L" polygons, " + CString((LONG)polymesh.numSamples) + L" nodes.");

                MATH::CVector3Array vertices(polymesh.numVertices);
                CLongArray          polygons(polymesh.numPolygons + polymesh.numSamples);
                float *pv = polymesh.vertPositions.data();
                for (LONG i=0;i<polymesh.numVertices;i++,pv+=3)
                  vertices[i].Set(pv[0], pv[1], pv[2]);
                LONG *dst = (LONG *)polygons.GetArray();
                uint32_t *src = polymesh.polyVertices.data();
                for (LONG i=0;i<polymesh.numPolygons;i++)
                {
                  LONG num = polymesh.polyNumVertices[i];
                  *dst = num;
                  dst++;
                  for (LONG j=0;j<num;j++,src++, dst++)
                    *dst = *src;
                }
                if (xsiPolymesh.Set(vertices, polygons) != CStatus::OK)
                  Application().LogMessage(L"xsiPolymesh.Set(vertices, polygons) failed", siErrorMsg);
                else
                {

                }
              }
            }
          }
          else
          {
            Application().LogMessage(L"XSI output port \"" + outputPort.GetName() + L"\" has the unsupported classID \"" + GetSiClassIdDescription(outputPort.GetTarget().GetClassID(), CString()) + L"\"", siWarningMsg);
          }
        }
      }
    }
    catch (FabricCore::Exception e)
    {
      std::string s = functionName.GetAsciiString() + std::string("(step 3): ") + (e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
      feLogError(s);
    }
  }
  
  // done.
  return CStatus::OK;
}

// returns: -1: error.
//           0: canceled by user or no changes made.
//           1: success.
int Dialog_DefinePortMapping(std::vector<_portMapping> &io_pmap)
{
  // nothing to do?
  if (io_pmap.size() == 0)
    return 0;

  // create the temporary custom property.
  CustomProperty cp = Application().GetActiveSceneRoot().AddProperty(L"CustomProperty", false, L"dfgDefinePortMapping");
  if (!cp.IsValid())
  { Application().LogMessage(L"failed to create custom property!", siErrorMsg);
    return -1; }
  
  // declare/init return value.
  int ret = 1;

  // add parameters.
  if (ret)
  {
    for (int i=0;i<io_pmap.size();i++)
    {
      _portMapping &pmap = io_pmap[i];

      // read-only grid with port name, type, etc.
      {
        Parameter gridParam = cp.AddGridParameter(L"grid" + CString(i));
        GridData grid((CRef)gridParam.GetValue());

        grid.PutRowCount(1);
        grid.PutColumnCount(3);
        grid.PutColumnLabel(0, "Name");         grid.PutColumnType(0, siColumnStandard);
        grid.PutColumnLabel(1, "Type");         grid.PutColumnType(1, siColumnStandard);
        grid.PutColumnLabel(2, "Mode");         grid.PutColumnType(2, siColumnStandard);
        grid.PutReadOnly(true);

        CString name = pmap.dfgPortName;
        CString type = pmap.dfgPortDataType;
        CString mode;
        switch (pmap.dfgPortType)
        {
          case DFG_PORT_TYPE::IN:   mode = L"In";         break;
          case DFG_PORT_TYPE::OUT:  mode = L"Out";        break;
          default:                  mode = L"undefined";  break;
        }

        grid.PutRowLabel( 0, L"  " + CString(i) + L"  ");
        grid.PutCell(0,   0, L" " + name);
        grid.PutCell(1,   0, L" " + type);
        grid.PutCell(2,   0, L" " + mode);
      }

      // other params.
      {
        // map type.
        cp.AddParameter(L"MapType" + CString(i), CValue::siInt4, 0, L"Type/Target", L"", pmap.mapType, Parameter());
      }
    }
  }

  // define layout.
  if (ret)
  {
    PPGItem pi;
    PPGLayout oLayout = cp.GetPPGLayout();
    oLayout.Clear();
    for (int i=0;i<io_pmap.size();i++)
    {
      _portMapping &pmap = io_pmap[i];

      oLayout.AddGroup(pmap.dfgPortName);
        oLayout.AddRow();
          oLayout.AddGroup(L"", false, 50);
            pi = oLayout.AddItem(L"grid" + CString(i));
            pi.PutAttribute(siUINoLabel,              true);
            pi.PutAttribute(siUIGridSelectionMode,    siSelectionNone);
            pi.PutAttribute(siUIGridHideColumnHeader, false);
            pi.PutAttribute(siUIGridLockColumnWidth,  true);
            pi.PutAttribute(siUIGridColumnWidths,     "20:128:64:40");
          oLayout.EndGroup();
          oLayout.AddGroup(L"", false, 5);
            oLayout.AddStaticText(L" ");
          oLayout.EndGroup();
          oLayout.AddGroup(L"", false);
          {
            CValueArray cvaMapType;
            cvaMapType.Add( L"Internal" );
            cvaMapType.Add( DFG_PORT_MAPTYPE::INTERNAL );
            if (pmap.dfgPortType == DFG_PORT_TYPE::IN)
            {
              if (   pmap.dfgPortDataType == L"Boolean"

                  || pmap.dfgPortDataType == L"Float32"
                  || pmap.dfgPortDataType == L"Float64"

                  || pmap.dfgPortDataType == L"SInt8"
                  || pmap.dfgPortDataType == L"SInt16"
                  || pmap.dfgPortDataType == L"SInt32"
                  || pmap.dfgPortDataType == L"SInt64"

                  || pmap.dfgPortDataType == L"UInt8"
                  || pmap.dfgPortDataType == L"UInt16"
                  || pmap.dfgPortDataType == L"UInt32"
                  || pmap.dfgPortDataType == L"UInt64")
              {
                cvaMapType.Add( L"XSI Parameter" );
                cvaMapType.Add( DFG_PORT_MAPTYPE::XSI_PARAMETER );
              }
              if (   pmap.dfgPortDataType == L"Boolean"

                  || pmap.dfgPortDataType == L"Float32"
                  || pmap.dfgPortDataType == L"Float64"

                  || pmap.dfgPortDataType == L"SInt8"
                  || pmap.dfgPortDataType == L"SInt16"
                  || pmap.dfgPortDataType == L"SInt32"
                  || pmap.dfgPortDataType == L"SInt64"

                  || pmap.dfgPortDataType == L"UInt8"
                  || pmap.dfgPortDataType == L"UInt16"
                  || pmap.dfgPortDataType == L"UInt32"
                  || pmap.dfgPortDataType == L"UInt64"

                  || pmap.dfgPortDataType == L"Mat44"

                  || pmap.dfgPortDataType == L"PolygonMesh")
              {
                cvaMapType.Add( L"XSI Port" );
                cvaMapType.Add( DFG_PORT_MAPTYPE::XSI_PORT );
              }
            }
            else if (pmap.dfgPortType == DFG_PORT_TYPE::OUT)
            {
              if (   pmap.dfgPortDataType == L"Boolean"

                  || pmap.dfgPortDataType == L"Float32"
                  || pmap.dfgPortDataType == L"Float64"

                  || pmap.dfgPortDataType == L"SInt8"
                  || pmap.dfgPortDataType == L"SInt16"
                  || pmap.dfgPortDataType == L"SInt32"
                  || pmap.dfgPortDataType == L"SInt64"

                  || pmap.dfgPortDataType == L"UInt8"
                  || pmap.dfgPortDataType == L"UInt16"
                  || pmap.dfgPortDataType == L"UInt32"
                  || pmap.dfgPortDataType == L"UInt64"

                  || pmap.dfgPortDataType == L"Mat44"

                  || pmap.dfgPortDataType == L"PolygonMesh")
              {
                cvaMapType.Add( L"XSI Port" );
                cvaMapType.Add( DFG_PORT_MAPTYPE::XSI_PORT );
              }
            }

            oLayout.AddEnumControl(L"MapType" + CString(i), cvaMapType, CString(), siControlRadio);
          }
          oLayout.EndGroup();
        oLayout.EndRow();
      oLayout.EndGroup();
    }
  }

  // inspect the custom property as a modal dialog.
  if (ret)
  {
    CValue r;
    CValueArray args;
    args.Add(cp.GetRef().GetAsText());
    args.Add(L"");
    args.Add(L"Define Port Mapping");
    args.Add(siModal);
    args.Add(false);
    if (Application().ExecuteCommand(L"InspectObj", args, r) != CStatus::OK)
    { Application().LogMessage(L"InspectObj failed", siErrorMsg);
      ret = -1; }
    if (r == true)  // canceled by user.
      ret = 0;
  }

  // update io_pm from the custom property.
  if (ret)
  {
    ret = 0;

    for (int i=0;i<io_pmap.size();i++)
    {
      _portMapping &pmap = io_pmap[i];

      DFG_PORT_MAPTYPE mem = pmap.mapType;
      pmap.mapType = (DFG_PORT_MAPTYPE)(int)cp.GetParameterValue(L"MapType" + CString(i));
      if (pmap.mapType != mem)
        ret = 1;
    }
  }

  // clean up.
  {
    // delete the custom property.
    CValueArray args;
    args.Add(cp.GetRef().GetAsText());
    if (Application().ExecuteCommand(L"DeleteObj", args, CValue()) != CStatus::OK)
      Application().LogMessage(L"DeleteObj failed", siWarningMsg);
  }

  // done.
  return ret;
}