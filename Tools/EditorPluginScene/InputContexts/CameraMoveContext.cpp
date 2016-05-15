#include <PCH.h>
#include <EditorPluginScene/InputContexts/CameraMoveContext.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <CoreUtils/Graphics/Camera.h>
#include <QKeyEvent>
#include <QDesktopWidget>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorPluginScene/Preferences/ScenePreferences.h>

static const float s_fMoveSpeed[31] =
{
  0.0078125f,
  0.01171875f,
  0.015625f,
  0.0234375f,
  0.03125f,

  0.046875f,
  0.0625f,
  0.09375f,
  0.125f,
  0.1875f,

  0.25f,
  0.375f,
  0.5f,
  0.75f,
  1.0f,

  1.5f,
  2.0f,
  3.0f,
  4.0f,
  6.0f,

  8.0f,
  12.0f,
  16.0f,
  24.0f,
  32.0f,

  48.0f,
  64.0f,
  96.0f,
  128.0f,
  192.0f,

  256.0f,
};

ezCameraMoveContext::ezCameraMoveContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, ezCameraMoveContextSettings* pSettings)
{
  EZ_ASSERT_DEV(pSettings != nullptr, "Need a valid settings object");

  m_pSettings = pSettings;
  m_pCamera = nullptr;

  m_bRun = false;
  m_bSlowDown = false;
  m_bMoveForwards = false;
  m_bMoveBackwards = false;
  m_bMoveRight = false;
  m_bMoveLeft = false;
  m_bMoveUp = false;
  m_bMoveDown = false;
  m_bMoveForwardsInPlane = false;
  m_bMoveBackwardsInPlane = false;

  m_LastUpdate = ezTime::Now();

  m_bRotateCamera = false;
  m_bMoveCamera = false;
  m_bMoveCameraInPlane = false;
  m_bTempMousePosition = false;
  m_bOrbitCamera = false;
  m_bSlideForwards = false;

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwnerWindow, pOwnerView);
}

void ezCameraMoveContext::FocusLost(bool bCancel)
{
  m_bRotateCamera = false;
  m_bMoveCamera = false;
  m_bMoveCameraInPlane = false;
  m_bOrbitCamera = false;
  m_bSlideForwards = false;

  ResetCursor();

  m_bRun = false;
  m_bSlowDown = false;
  m_bMoveForwards = false;
  m_bMoveBackwards = false;
  m_bMoveRight = false;
  m_bMoveLeft = false;
  m_bMoveUp = false;
  m_bMoveDown = false;
  m_bMoveForwardsInPlane = false;
  m_bMoveBackwardsInPlane = false;
}

void ezCameraMoveContext::LoadState()
{
  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetOwnerWindow()->GetDocument());
  SetMoveSpeed(pPreferences->m_iCameraSpeed);
}

void ezCameraMoveContext::UpdateContext()
{
  ezTime diff = ezTime::Now() - m_LastUpdate;
  m_LastUpdate = ezTime::Now();

  const double TimeDiff = ezMath::Min(diff.GetSeconds(), 0.1);

  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetOwnerWindow()->GetDocument());
  float fSpeedFactor = s_fMoveSpeed[pPreferences->m_iCameraSpeed] * TimeDiff;

  if (m_bRun)
    fSpeedFactor *= 5.0f;
  if (m_bSlowDown)
    fSpeedFactor *= 0.2f;

  if (m_bMoveForwards)
    m_pCamera->MoveLocally(fSpeedFactor, 0, 0);
  if (m_bMoveBackwards)
    m_pCamera->MoveLocally(-fSpeedFactor, 0, 0);
  if (m_bMoveRight)
    m_pCamera->MoveLocally(0, fSpeedFactor, 0);
  if (m_bMoveLeft)
    m_pCamera->MoveLocally(0, -fSpeedFactor, 0);
  if (m_bMoveUp)
    m_pCamera->MoveGlobally(ezVec3(0, 0, 1) * fSpeedFactor);
  if (m_bMoveDown)
    m_pCamera->MoveGlobally(ezVec3(0, 0, -1) * fSpeedFactor);
  if (m_bMoveForwardsInPlane)
  {
    /// \todo hard coded up direction
    ezVec3 vDir = m_pCamera->GetCenterDirForwards();
    vDir.z = 0.0f;
    vDir.NormalizeIfNotZero(ezVec3::ZeroVector());
    m_pCamera->MoveGlobally(vDir * fSpeedFactor);
  }
  if (m_bMoveBackwardsInPlane)
  {
    ezVec3 vDir = m_pCamera->GetCenterDirForwards();
    vDir.z = 0.0f;
    vDir.NormalizeIfNotZero(ezVec3::ZeroVector());
    m_pCamera->MoveGlobally(vDir * -fSpeedFactor);
  }
}

ezEditorInut ezCameraMoveContext::keyReleaseEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return ezEditorInut::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;
  m_bSlowDown = (e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0;

  switch (e->key())
  {
  case Qt::Key_W:
    m_bMoveForwards = false;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_S:
    m_bMoveBackwards = false;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_A:
    m_bMoveLeft = false;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_D:
    m_bMoveRight = false;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_Q:
    m_bMoveUp = false;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_E:
    m_bMoveDown = false;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_Left:
    m_bMoveLeft = false;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_Right:
    m_bMoveRight = false;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_Up:
    m_bMoveForwardsInPlane = false;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_Down:
    m_bMoveBackwardsInPlane = false;
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezCameraMoveContext::keyPressEvent(QKeyEvent* e)
{
  if (m_pCamera == nullptr)
    return ezEditorInut::MayBeHandledByOthers;

  if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
    return ezEditorInut::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;
  //m_bSlowDown = (e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0;

  switch (e->key())
  {
  case Qt::Key_Left:
    m_bMoveLeft = true;
    SetActiveInputContext(this);
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_Right:
    m_bMoveRight = true;
    SetActiveInputContext(this);
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_Up:
    m_bMoveForwardsInPlane = true;
    SetActiveInputContext(this);
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_Down:
    m_bMoveBackwardsInPlane = true;
    SetActiveInputContext(this);
    return ezEditorInut::WasExclusivelyHandled;
  }

  if (!m_bRotateCamera)
    return ezEditorInut::MayBeHandledByOthers;

  switch (e->key())
  {
  case Qt::Key_W:
    m_bMoveForwards = true;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_S:
    m_bMoveBackwards = true;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_A:
    m_bMoveLeft = true;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_D:
    m_bMoveRight = true;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_Q:
    m_bMoveUp = true;
    return ezEditorInut::WasExclusivelyHandled;
  case Qt::Key_E:
    m_bMoveDown = true;
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezCameraMoveContext::mousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedHeight || m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedWidth)
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bMoveCamera = true;
      m_LastMousePos = e->globalPos();
      m_bDidMoveMouse[1] = false;
      MakeActiveInputContext();
      return ezEditorInut::WasExclusivelyHandled;
    }
  }
  else
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bSlideForwards = false;
      m_bRotateCamera = false;

      m_fSlideForwardsDistance = (m_pSettings->m_vOrbitPoint - m_pCamera->GetPosition()).GetLength();

      if ((e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0)
        m_bSlideForwards = true;
      else
        m_bRotateCamera = true;

      m_LastMousePos = e->globalPos();
      m_bDidMoveMouse[1] = false;
      MakeActiveInputContext();
      return ezEditorInut::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_bOrbitCamera = false;
      m_bMoveCamera = false;

      if ((e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0)
        m_bOrbitCamera = true;
      else
        m_bMoveCamera = true;

      m_LastMousePos = e->globalPos();
      m_bDidMoveMouse[0] = false;
      MakeActiveInputContext();
      return ezEditorInut::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::MiddleButton)
    {
      m_bRotateCamera = false;
      m_bMoveCamera = false;
      m_bMoveCameraInPlane = false;

      if ((e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0)
      {
        // pan
        m_bRotateCamera = true;
        m_bMoveCamera = true;
      }
      else
        m_bMoveCameraInPlane = true;

      m_LastMousePos = e->globalPos();
      m_bDidMoveMouse[2] = false;
      MakeActiveInputContext();
      return ezEditorInut::WasExclusivelyHandled;
    }
  }

  return ezEditorInut::MayBeHandledByOthers;
}

void ezCameraMoveContext::ResetCursor()
{
  if (!m_bRotateCamera && !m_bMoveCamera && !m_bMoveCameraInPlane && !m_bOrbitCamera && !m_bSlideForwards)
  {
    if (m_bTempMousePosition)
    {
      m_bTempMousePosition = false;
      QCursor::setPos(m_OriginalMousePos);
    }

    GetOwnerView()->setCursor(QCursor(Qt::ArrowCursor));
    MakeActiveInputContext(false);
  }
}

void ezCameraMoveContext::SetBlankCursor()
{
  if (m_bRotateCamera || m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera || m_bSlideForwards)
  {
    GetOwnerView()->setCursor(QCursor(Qt::BlankCursor));
  }
}

void ezCameraMoveContext::SetCursorToWindowCenter(QPoint pos)
{
  /// \todo Move this functionality into the base class

  if (!m_bTempMousePosition)
  {
    m_OriginalMousePos = m_LastMousePos;
    m_bTempMousePosition = true;
  }

  QDesktopWidget dw;
  QRect rect = dw.screenGeometry(this->GetOwnerWindow());

  QPoint center;
  center.setX(rect.left() + rect.width() / 2);
  center.setY(rect.top() + rect.height() / 2);

  // Only move cursor if we reach the edge of this current screen.
  rect.adjust(20, 20, -20, -20);
  if (!rect.contains(pos))
  {
    m_LastMousePos = center;
    QCursor::setPos(center);
  }
  else
  {
    m_LastMousePos = pos;
  }
}

ezEditorInut ezCameraMoveContext::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedHeight || m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedWidth)
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bMoveCamera = false;

      ResetCursor();
      return ezEditorInut::WasExclusivelyHandled;
    }
  }
  else
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bRotateCamera = false;
      m_bSlideForwards = false;

      m_bMoveForwards = false;
      m_bMoveBackwards = false;
      m_bMoveLeft = false;
      m_bMoveRight = false;
      m_bMoveUp = false;
      m_bMoveDown = false;

      ResetCursor();
      return ezEditorInut::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_bMoveCamera = false;
      m_bOrbitCamera = false;
      ResetCursor();

      if (!m_bDidMoveMouse[0])
      {
        // not really handled, so make this context inactive and tell the surrounding code that it may pass
        // the event to the next handler
        return ezEditorInut::MayBeHandledByOthers;
      }

      return ezEditorInut::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::MiddleButton)
    {
      m_bRotateCamera = false;
      m_bMoveCamera = false;
      m_bMoveCameraInPlane = false;

      ResetCursor();
      return ezEditorInut::WasExclusivelyHandled;
    }
  }

  return ezEditorInut::MayBeHandledByOthers;
}

void ezCameraMoveContext::SetOrbitPoint(const ezVec3& vPos)
{
  m_pSettings->m_vOrbitPoint = vPos;
}

ezEditorInut ezCameraMoveContext::mouseMoveEvent(QMouseEvent* e)
{
  // do nothing, unless this is an active context
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  // store that the mouse has been moved since the last click
  for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(m_bDidMoveMouse); ++i)
    m_bDidMoveMouse[i] = true;

  // send a message to clear any highlight
  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  if (m_pCamera == nullptr)
    return ezEditorInut::MayBeHandledByOthers;

  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetOwnerWindow()->GetDocument());

  float fBoost = 1.0f;
  float fRotateBoost = 1.0f;

  if (m_bRun)
    fBoost = 5.0f;
  if (m_bSlowDown)
  {
    fBoost = 0.1f;
    fRotateBoost = 0.2f;
  }

  if (m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedHeight || m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedWidth)
  {
    float fDistPerPixel = 0;

    if (m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedHeight)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().height();
    
    if (m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedWidth)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().width();

    if (m_bMoveCamera)
    {
      const QPointF diff = e->globalPos() - m_LastMousePos;
      m_LastMousePos = e->globalPos();

      float fMoveUp = diff.y() * fDistPerPixel;
      float fMoveRight = -diff.x() * fDistPerPixel;

      m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

      return ezEditorInut::WasExclusivelyHandled;
    }
  }
  else
  {
    SetBlankCursor();

    const float fAspectRatio = (float)GetOwnerView()->size().width() / (float)GetOwnerView()->size().height();
    const ezAngle fFovX = m_pCamera->GetFovX(fAspectRatio);
    const ezAngle fFovY = m_pCamera->GetFovY(fAspectRatio);

    const float fMouseScale = 4.0f;

    const float fMouseMoveSensitivity = 0.002f * s_fMoveSpeed[pPreferences->m_iCameraSpeed] * fBoost;
    const float fMouseRotateSensitivityX = (fFovX.GetRadian() / (float)GetOwnerView()->size().width()) * fRotateBoost * fMouseScale;
    const float fMouseRotateSensitivityY = (fFovY.GetRadian() / (float)GetOwnerView()->size().height()) * fRotateBoost * fMouseScale;

    if (m_bRotateCamera && m_bMoveCamera) // left & right mouse button -> pan
    {
      const QPointF diff = e->globalPos() - m_LastMousePos;

      float fMoveUp = -diff.y() * fMouseMoveSensitivity;
      float fMoveRight = diff.x() * fMouseMoveSensitivity;

      m_pSettings->m_vOrbitPoint += m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

      SetCursorToWindowCenter(e->globalPos());
      return ezEditorInut::WasExclusivelyHandled;
    }

    if (m_bRotateCamera)
    {
      const QPointF diff = e->globalPos() - m_LastMousePos;

      float fRotateHorizontal = diff.x() * fMouseRotateSensitivityX;
      float fRotateVertical = diff.y() * fMouseRotateSensitivityY;

      m_pCamera->RotateLocally(ezAngle::Radian(0), ezAngle::Radian(fRotateVertical), ezAngle::Radian(0));
      m_pCamera->RotateGlobally(ezAngle::Radian(0), ezAngle::Radian(0), ezAngle::Radian(fRotateHorizontal));

      SetCursorToWindowCenter(e->globalPos());
      return ezEditorInut::WasExclusivelyHandled;
    }

    if (m_bMoveCamera)
    {
      const QPointF diff = e->globalPos() - m_LastMousePos;

      float fMoveRight = diff.x() * fMouseMoveSensitivity;
      float fMoveForward = -diff.y() * fMouseMoveSensitivity;

      m_pSettings->m_vOrbitPoint += m_pCamera->MoveLocally(fMoveForward, fMoveRight, 0);

      SetCursorToWindowCenter(e->globalPos());

      return ezEditorInut::WasExclusivelyHandled;
    }

    if (m_bMoveCameraInPlane)
    {
      const QPointF diff = e->globalPos() - m_LastMousePos;

      float fMoveRight = diff.x() * fMouseMoveSensitivity;
      float fMoveForward = -diff.y() * fMouseMoveSensitivity;

      m_pSettings->m_vOrbitPoint += m_pCamera->MoveLocally(0, fMoveRight, 0);

      ezVec3 vDir = m_pCamera->GetCenterDirForwards();
      vDir.z = 0.0f;
      vDir.NormalizeIfNotZero(ezVec3::ZeroVector());

      m_pSettings->m_vOrbitPoint += vDir * fMoveForward;
      m_pCamera->MoveGlobally(vDir * fMoveForward);

      SetCursorToWindowCenter(e->globalPos());

      return ezEditorInut::WasExclusivelyHandled;
    }

    if (m_bOrbitCamera)
    {
      const QPointF diff = e->globalPos() - m_LastMousePos;

      float fMoveRight = -diff.x() * fMouseMoveSensitivity;
      float fMoveUp = diff.y() * fMouseMoveSensitivity;

      const float fDistance = (m_pSettings->m_vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

      m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

      ezVec3 vDir = m_pSettings->m_vOrbitPoint - m_pCamera->GetCenterPosition();
      if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
      {
        vDir.Set(0.01f, 0, 0);
      }

      if (ezMath::Abs(vDir.GetNormalized().Dot(ezVec3(0, 0, 1))) < 0.999f)
        m_pCamera->LookAt(m_pSettings->m_vOrbitPoint - vDir, m_pSettings->m_vOrbitPoint, ezVec3(0.0f, 0.0f, 1.0f));
      else
        m_pSettings->m_vOrbitPoint += m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

      SetCursorToWindowCenter(e->globalPos());

      return ezEditorInut::WasExclusivelyHandled;
    }

    if (m_bSlideForwards)
    {
      const QPointF diff = e->globalPos() - m_LastMousePos;

      float fMove = diff.y() * fMouseMoveSensitivity * m_fSlideForwardsDistance * 0.1f;

      m_pCamera->MoveLocally(fMove, 0, 0);
      //m_pSettings->m_vOrbitPoint +=

      SetCursorToWindowCenter(e->globalPos());

      return ezEditorInut::WasExclusivelyHandled;

    }
  }

  return ezEditorInut::MayBeHandledByOthers;
}

void ezCameraMoveContext::SetMoveSpeed(ezInt32 iSpeed)
{
  if (GetOwnerWindow()->GetDocument() != nullptr)
  {
    ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetOwnerWindow()->GetDocument());
    pPreferences->m_iCameraSpeed = ezMath::Clamp(iSpeed, 0, 30);
  }
}

ezEditorInut ezCameraMoveContext::wheelEvent(QWheelEvent* e)
{
  if (m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera || m_bRotateCamera)
    return ezEditorInut::WasExclusivelyHandled; // ignore it, but others should not handle it either

  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetOwnerWindow()->GetDocument());

  if (m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedHeight || m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedWidth)
  {
    float fBoost = 1.0f;
    const float fTick = 1.4f;
    const float fBoostFactor = 1.5f;

    if (e->modifiers() == Qt::KeyboardModifier::ShiftModifier)
      fBoost *= fBoostFactor;
    //if (e->modifiers() == Qt::KeyboardModifier::AltModifier)
    //  fBoost *= 1.0f / fBoostFactor;

    if (e->delta() > 0)
    {
      m_pCamera->SetCameraMode(m_pCamera->GetCameraMode(), m_pCamera->GetFovOrDim() * ezMath::Pow(1.0f / fTick, fBoost), m_pCamera->GetNearPlane(), m_pCamera->GetFarPlane());
    }
    else
    {
      m_pCamera->SetCameraMode(m_pCamera->GetCameraMode(), m_pCamera->GetFovOrDim() * ezMath::Pow(fTick, fBoost), m_pCamera->GetNearPlane(), m_pCamera->GetFarPlane());
    }

    // handled, independent of whether we are the active context or not
    return ezEditorInut::WasExclusivelyHandled;
  }
  else
  {
    if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
    {

      if (e->delta() > 0)
      {
        SetMoveSpeed(pPreferences->m_iCameraSpeed + 1);
      }
      else
      {
        SetMoveSpeed(pPreferences->m_iCameraSpeed - 1);
      }

      // handled, independent of whether we are the active context or not
      return ezEditorInut::WasExclusivelyHandled;
    }

    {
      float fBoost = 0.25f;

      if (e->modifiers() == Qt::KeyboardModifier::ShiftModifier)
        fBoost *= 5.0f;
      //if (e->modifiers() == Qt::KeyboardModifier::AltModifier)
      //  fBoost *= 0.1f;

      if (e->delta() > 0)
      {
        m_pCamera->MoveLocally(s_fMoveSpeed[pPreferences->m_iCameraSpeed] * fBoost, 0, 0);
      }
      else
      {
        m_pCamera->MoveLocally(-s_fMoveSpeed[pPreferences->m_iCameraSpeed] * fBoost, 0, 0);
      }

      // handled, independent of whether we are the active context or not
      return ezEditorInut::WasExclusivelyHandled;
    }
  }
}

void ezCameraMoveContext::SetCamera(ezCamera* pCamera)
{
  if (m_pCamera == pCamera)
    return;

  m_pCamera = pCamera;
}

