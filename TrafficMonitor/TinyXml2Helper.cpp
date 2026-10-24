#include "stdafx.h"
#include "TinyXml2Helper.h"
#include "Common.h"

bool CTinyXml2Helper::LoadXmlFile(tinyxml2::XMLDocument& doc, const wchar_t* file_path)
{
    //Since the XMLDocument::LoadFile function does not support Unicode, the file content is read here and the XMLDocument::Parse function is called to parse it.
    size_t length;
    const char* xml_contents = CCommon::GetFileContent(file_path, length);
    auto err = doc.Parse(xml_contents, length);
    delete[] xml_contents;
    return err == tinyxml2::XML_SUCCESS;
}

void CTinyXml2Helper::IterateChildNode(tinyxml2::XMLElement* ele, std::function<void(tinyxml2::XMLElement*)> fun)
{
    if (ele == nullptr)
        return;

    tinyxml2::XMLElement* child = ele->FirstChildElement();
    if (child == nullptr)
        return;
    fun(child);
    while (true)
    {
        child = child->NextSiblingElement();
        if (child != nullptr)
            fun(child);
        else
            break;
    }
}

const char * CTinyXml2Helper::ElementAttribute(tinyxml2::XMLElement * ele, const char * attr)
{
    if (ele != nullptr)
    {
        const char* str = ele->Attribute(attr);
        if (str != nullptr)
            return str;
    }
    return "";
}

const char* CTinyXml2Helper::ElementName(tinyxml2::XMLElement* ele)
{
    if (ele != nullptr)
    {
        const char* str = ele->Name();
        if (str != nullptr)
            return str;
    }
    return "";
}

const char* CTinyXml2Helper::ElementText(tinyxml2::XMLElement* ele)
{
    if (ele != nullptr)
    {
        const char* str = ele->GetText();
        if (str != nullptr)
            return str;
    }
    return "";
}

bool CTinyXml2Helper::StringToBool(const char* str)
{
    string str_text{ str };
    return (!str_text.empty() && str_text != "0");
}

