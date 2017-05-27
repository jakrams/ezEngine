﻿#pragma once

#include <RecastPlugin/Basics.h>
#include <Core/World/WorldModule.h>

class dtCrowd;
class dtNavMesh;

class EZ_RECASTPLUGIN_DLL ezRecastWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();

public:
  ezRecastWorldModule(ezWorld* pWorld);
  ~ezRecastWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void SetNavMesh(dtNavMesh* pNavMesh);
  bool IsInitialized() const { return m_pCrowd != nullptr; }

  dtCrowd* m_pCrowd = nullptr;

private:
  void UpdateCrowd(const UpdateContext& ctxt);

  dtNavMesh* m_pNavMesh = nullptr;
};
