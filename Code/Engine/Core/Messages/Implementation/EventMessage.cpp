#include <CorePCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezEventMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEventMessage, 1, ezRTTIDefaultAllocator<ezEventMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_CHECK_AT_COMPILETIME(sizeof(ezEventMessageSender<ezEventMessage>) == 8);

namespace ezInternal
{
  void EventMessageSenderHelper::SendEventMessage(ezComponent* pSenderComponent, ezComponentHandle hReceiver, ezEventMessage& msg)
  {
    ezWorld* pWorld = pSenderComponent->GetWorld();
    ezComponent* pReceiverComponent = nullptr;
    if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
    {
      pReceiverComponent->SendMessage(msg);
    }
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    else if (msg.GetDebugMessageRouting())
    {
      ezLog::Warning("ezEventMessageSender::SendMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

  void EventMessageSenderHelper::SendEventMessage(const ezComponent* pSenderComponent, ezComponentHandle hReceiver, ezEventMessage& msg)
  {
    const ezWorld* pWorld = pSenderComponent->GetWorld();
    const ezComponent* pReceiverComponent = nullptr;
    if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
    {
      pReceiverComponent->SendMessage(msg);
    }
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    else if (msg.GetDebugMessageRouting())
    {
      ezLog::Warning("ezEventMessageSender::SendMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

  void EventMessageSenderHelper::PostEventMessage(
    const ezComponent* pSenderComponent, ezComponentHandle hReceiver, const ezEventMessage& msg, ezTime delay, ezObjectMsgQueueType::Enum queueType)
  {
    if (!hReceiver.IsInvalidated())
    {
      const ezWorld* pWorld = pSenderComponent->GetWorld();
      pWorld->PostMessage(hReceiver, msg, delay, queueType);
    }
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    else if (msg.GetDebugMessageRouting())
    {
      ezLog::Warning("ezEventMessageSender::PostMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }
} // namespace ezInternal



EZ_STATICLINK_FILE(Core, Core_Messages_Implementation_EventMessage);
