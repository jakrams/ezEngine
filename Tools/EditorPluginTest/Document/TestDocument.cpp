#include <PCH.h>
#include <EditorPluginTest/Document/TestDocument.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorFramework/EditorApp.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestDocument, ezDocumentBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTestDocument::ezTestDocument(const char* szDocumentPath) : ezDocumentBase(szDocumentPath)
{
  m_pObjectManager = new ezTestObjectManager(this);

}

ezTestDocument::~ezTestDocument()
{
  delete m_pObjectManager;
}

ezStatus ezTestDocument::InternalSaveDocument()
{
  return ezDocumentBase::InternalSaveDocument();
}

