#pragma once

#include <RtsGamePlugin/GameMode/GameMode.h>
#include <Utilities/DataStructures/ObjectSelection.h>

class RtsEditLevelMode : public RtsGameMode
{
public:
  RtsEditLevelMode();
  ~RtsEditLevelMode();

protected:
  virtual void OnActivateMode() override;
  virtual void OnDeactivateMode() override;
  virtual void RegisterInputActions() override;
  virtual void OnProcessInput() override;
  virtual void OnBeforeWorldUpdate() override;
};