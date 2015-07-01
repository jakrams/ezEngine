#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <CoreUtils/Geometry/OBJLoader.h>
#include <Foundation/IO/OSFile.h>
#include <QTimer>
#include <QPushButton>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <qlayout.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Core/World/GameObject.h>
#include <QKeyEvent>
#include <Foundation/Time/Time.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>



void ezSceneDocumentWindow::UpdateGizmoVisibility()
{
  ezSceneDocument* pSceneDoc = static_cast<ezSceneDocument*>(GetDocument());

  m_TranslateGizmo.SetVisible(false);
  m_RotateGizmo.SetVisible(false);
  m_ScaleGizmo.SetVisible(false);
  m_DragToPosGizmo.SetVisible(false);

  if (pSceneDoc->GetSelectionManager()->GetSelection().IsEmpty() || pSceneDoc->GetActiveGizmo() == ActiveGizmo::None)
    return;

  switch (pSceneDoc->GetActiveGizmo())
  {
  case ActiveGizmo::Translate:
    m_TranslateGizmo.SetVisible(true);
    break;
  case ActiveGizmo::Rotate:
    m_RotateGizmo.SetVisible(true);
    break;
  case ActiveGizmo::Scale:
    m_ScaleGizmo.SetVisible(true);
    break;
  case ActiveGizmo::DragToPosition:
    m_DragToPosGizmo.SetVisible(true);
    break;
  }

  UpdateGizmoPosition();
}


void ezSceneDocumentWindow::UpdateGizmoSelectionList()
{
  // Get the list of all objects that are manipulated
  // and store their original transformation

  m_GizmoSelection.Clear();

  auto hType = ezRTTI::FindTypeByName("ezGameObject");

  auto pSelMan = GetDocument()->GetSelectionManager();
  const auto& Selection = pSelMan->GetSelection();
  for (ezUInt32 sel = 0; sel < Selection.GetCount(); ++sel)
  {
    if (!Selection[sel]->GetTypeAccessor().GetType()->IsDerivedFrom(hType))
      continue;

    // ignore objects, whose parent is already selected as well, so that transformations aren't applied
    // multiple times on the same hierarchy
    if (pSelMan->IsParentSelected(Selection[sel]))
      continue;

    SelectedGO sgo;
    sgo.m_Object = Selection[sel]->GetGuid();
    sgo.m_vTranslation = Selection[sel]->GetTypeAccessor().GetValue("GlobalPosition").ConvertTo<ezVec3>();
    sgo.m_vScaling = Selection[sel]->GetTypeAccessor().GetValue("GlobalScaling").ConvertTo<ezVec3>();
    sgo.m_Rotation = Selection[sel]->GetTypeAccessor().GetValue("GlobalRotation").ConvertTo<ezQuat>();

    m_GizmoSelection.PushBack(sgo);
  }
}

void ezSceneDocumentWindow::UpdateGizmoPosition()
{
  const auto& LatestSelection = GetDocument()->GetSelectionManager()->GetSelection().PeekBack();

  if (LatestSelection->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezGameObject>())
  {
    const ezTransform tGlobal = ezSceneDocument::QueryGlobalTransform(LatestSelection);

    const ezVec3 vPivotPoint = tGlobal.m_Rotation * LatestSelection->GetEditorTypeAccessor().GetValue("Pivot").ConvertTo<ezVec3>();

    ezMat4 mt;

    if (GetSceneDocument()->GetGizmoWorldSpace())
    {
      mt.SetTranslationMatrix(tGlobal.m_vPosition + vPivotPoint);
    }
    else
    {
      mt = ezMat4(tGlobal.m_Rotation, tGlobal.m_vPosition + vPivotPoint);
      mt.SetScalingFactors(ezVec3(1.0f));
    }

    m_TranslateGizmo.SetTransformation(mt);
    m_RotateGizmo.SetTransformation(mt);
    m_ScaleGizmo.SetTransformation(mt);
    m_DragToPosGizmo.SetTransformation(mt);
  }

}


void ezSceneDocumentWindow::TransformationGizmoEventHandler(const ezGizmoBase::BaseEvent& e)
{
  switch (e.m_Type)
  {
  case ezGizmoBase::BaseEvent::Type::BeginInteractions:
    {
      UpdateGizmoSelectionList();

      GetDocument()->GetCommandHistory()->BeginTemporaryCommands();

    }
    break;

  case ezGizmoBase::BaseEvent::Type::EndInteractions:
    {
      GetDocument()->GetCommandHistory()->EndTemporaryCommands(false);

      m_GizmoSelection.Clear();
    }
    break;

  case ezGizmoBase::BaseEvent::Type::Interaction:
    {
      const ezMat4 mTransform = e.m_pGizmo->GetTransformation();

      m_bInGizmoInteraction = true;
      GetDocument()->GetCommandHistory()->StartTransaction();

      bool bCancel = false;

      ezSetObjectPropertyCommand cmd;
      cmd.m_bEditorProperty = false;

      if (e.m_pGizmo == &m_TranslateGizmo)
      {
        cmd.SetPropertyPath("GlobalPosition");

        const ezVec3 vTranslate = m_TranslateGizmo.GetTranslationResult();

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          cmd.m_Object = obj.m_Object;
          cmd.m_NewValue = obj.m_vTranslation + vTranslate;

          if (GetDocument()->GetCommandHistory()->AddCommand(cmd).m_Result.Failed())
          {
            bCancel = true;
            break;
          }
        }
      }

      if (e.m_pGizmo == &m_RotateGizmo)
      {
        cmd.SetPropertyPath("GlobalRotation");

        const ezQuat qRotation = m_RotateGizmo.GetRotationResult();

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          cmd.m_Object = obj.m_Object;
          cmd.m_NewValue = qRotation * obj.m_Rotation;

          if (GetDocument()->GetCommandHistory()->AddCommand(cmd).m_Result.Failed())
          {
            bCancel = true;
            break;
          }
        }
      }

      if (e.m_pGizmo == &m_ScaleGizmo)
      {
        cmd.SetPropertyPath("LocalScaling");

        const ezVec3 vScale = m_ScaleGizmo.GetScalingResult();

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          cmd.m_Object = obj.m_Object;
          cmd.m_NewValue = obj.m_vScaling.CompMult(vScale);

          if (GetDocument()->GetCommandHistory()->AddCommand(cmd).m_Result.Failed())
          {
            bCancel = true;
            break;
          }
        }
      }

      if (e.m_pGizmo == &m_DragToPosGizmo)
      {
        cmd.SetPropertyPath("GlobalPosition");

        const ezVec3 vTranslate = m_DragToPosGizmo.GetTranslationResult();

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          cmd.m_Object = obj.m_Object;
          cmd.m_NewValue = obj.m_vTranslation + vTranslate;

          if (GetDocument()->GetCommandHistory()->AddCommand(cmd).m_Result.Failed())
          {
            bCancel = true;
            break;
          }
        }
      }

      GetDocument()->GetCommandHistory()->EndTransaction(bCancel);
      m_bInGizmoInteraction = false;
    }
    break;
  }

}



