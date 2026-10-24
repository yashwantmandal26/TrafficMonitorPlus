#pragma once
#include "tinyxml2/tinyxml2.h"
#include <string>
#include <functional>

class CTinyXml2Helper
{
public:

    //Read XML content from file
    static bool LoadXmlFile(tinyxml2::XMLDocument& doc, const wchar_t* file_path);

    //Traverse an XML node
    //fun: a function object, called when a node is traversed
    static void IterateChildNode(tinyxml2::XMLElement* ele, std::function<void(tinyxml2::XMLElement*)> fun);

    //Get the attributes of a node (the return value will not be a null pointer, if not found, an empty string will be returned)
    static const char* ElementAttribute(tinyxml2::XMLElement* ele, const char* attr);

    //Get the name of a node (the return value will not be a null pointer, if it cannot be found, it will return an empty string)
    static const char* ElementName(tinyxml2::XMLElement* ele);

    //Get the text of a node (the return value will not be a null pointer, if it cannot be found, it will return an empty string)
    static const char* ElementText(tinyxml2::XMLElement* ele);

    static bool StringToBool(const char* str);
};
