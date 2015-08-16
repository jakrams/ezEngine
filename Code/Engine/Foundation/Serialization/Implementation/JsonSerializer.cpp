#include <Foundation/PCH.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>

struct CompareConstChar
{
  /// \brief Returns true if a is less than b
  EZ_FORCE_INLINE bool Less(const char* a, const char* b) const
  {
    return ezStringUtils::Compare(a, b) < 0;
  }

  /// \brief Returns true if a is equal to b
  EZ_FORCE_INLINE bool Equal(const char* a, const char* b) const
  {
    return ezStringUtils::IsEqual(a, b);
  }
};

void ezAbstractGraphJsonSerializer::Write(ezStreamWriterBase& stream, const ezAbstractObjectGraph* pGraph, ezStandardJSONWriter::WhitespaceMode mode)
{
  const auto& Nodes = pGraph->GetAllNodes();

  ezExtendedJSONWriter writer;
  writer.SetOutputStream(&stream);
  writer.SetWhitespaceMode(mode);

  writer.BeginObject();
  {
    ezMap<const char*, const ezVariant*, CompareConstChar> SortedProperties;

    writer.BeginArray("Objects");

    for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
    {
      writer.BeginObject();
      {
        const auto& node = itNode.Value();

        writer.AddVariableUuid("#", node.GetGuid());
        writer.AddVariableString("t", node.GetType());

        if (node.GetNodeName() != nullptr)
          writer.AddVariableString("n", node.GetNodeName());

        writer.BeginArray("p");
        {
          for (const auto& prop : node.GetProperties())
            SortedProperties[prop.m_szPropertyName] = &prop.m_Value;

          for (auto it = SortedProperties.GetIterator(); it.IsValid(); ++it)
          {
            writer.BeginObject();

            writer.AddVariableString("n", it.Key());
            writer.AddVariableVariant("v", *it.Value());

            writer.EndObject();
          }

          SortedProperties.Clear();
        }
        writer.EndArray();
      }
      writer.EndObject();
    }

    writer.EndArray();
  }
  writer.EndObject();
}


void ezAbstractGraphJsonSerializer::Read(ezStreamReaderBase& stream, ezAbstractObjectGraph* pGraph)
{
  ezExtendedJSONReader reader;
  reader.Parse(stream);

  ezVariant* pObjects;

  if (!reader.GetTopLevelObject().TryGetValue("Objects", pObjects))
  {
    EZ_REPORT_FAILURE("JSON file does not contain an 'Objects' node at root level");
    return;
  }

  EZ_ASSERT_DEV(pObjects->IsA<ezVariantArray>(), "'Objects' node is not of type array");

  const ezVariantArray& ObjArray = pObjects->Get<ezVariantArray>();

  for (const auto& object : ObjArray)
  {
    EZ_ASSERT_DEV(object.IsA<ezVariantDictionary>(), "'Objects' array contains elements that are not dictionaries");
    
    const auto& ObjDict = object.Get<ezVariantDictionary>();

    ezVariant* pGuid = nullptr;
    ezVariant* pType = nullptr;
    ezVariant* pProp = nullptr;
    ezVariant* pNodeName = nullptr;
    ObjDict.TryGetValue("#", pGuid);
    ObjDict.TryGetValue("t", pType);
    ObjDict.TryGetValue("p", pProp);
    ObjDict.TryGetValue("n", pNodeName);

    if (pGuid == nullptr || pType == nullptr || pProp == nullptr ||
        !pGuid->IsA<ezUuid>() || !pType->IsA<ezString>() || !pProp->IsA<ezVariantArray>())
    {
      EZ_REPORT_FAILURE("'Objects' array contains invalid elements");
      continue;
    }

    const char* szNodeName = nullptr;
    if (pNodeName != nullptr && pNodeName->IsA<ezString>())
      szNodeName = pNodeName->Get<ezString>();

    auto* pNode = pGraph->AddNode(pGuid->Get<ezUuid>(), pType->Get<ezString>(), szNodeName);
    
    const ezVariantArray& Properties = pProp->Get<ezVariantArray>();
    for (const auto& prop : Properties)
    {
      if (!prop.IsA<ezVariantDictionary>())
      {
        EZ_REPORT_FAILURE("Property is not an object");
        continue;
      }

      const ezVariantDictionary& PropDict = prop.Get<ezVariantDictionary>();

      ezVariant* pName = nullptr;
      ezVariant* pValue = nullptr;
      PropDict.TryGetValue("n", pName);
      PropDict.TryGetValue("v", pValue);

      if (pName == nullptr || pValue == nullptr || !pName->IsA<ezString>())
      {
        EZ_REPORT_FAILURE("Property does not have valid name and value");
        continue;
      }

      pNode->AddProperty(pName->Get<ezString>(), *pValue);
    }

  }
}




