
#include "JDatabundle.h"


void JDatabundle::SetUniqueName(std::string unique_name) {

    m_unique_name = unique_name;
    m_has_short_name = false;

    // Check if unique name follows short name format
    auto split = unique_name.find(':');
    auto front = unique_name.substr(0, split);
    if (front == m_type_name) {
        if (split != std::string::npos) {
            // "MyTypeName:MyShortName" => m_short_name = "MyShortName"
            m_has_short_name = true;
            m_short_name = unique_name.substr(split+1);
        }
        else if (m_type_name.length() == unique_name.length()) {
            // "MyTypeName" => m_short_name = ""
            m_has_short_name = true;
            m_short_name = "";
        }
        // "MyTypeNameMyShortName" does NOT yield a valid shortname
    }
}

void JDatabundle::SetShortName(std::string short_name) {
    m_short_name = short_name;
    if (m_short_name.empty()) {
        m_unique_name = m_type_name;
    }
    else {
        m_unique_name = m_type_name + ":" + short_name;
    }
    m_has_short_name = true;
}

void JDatabundle::SetTypeName(std::string type_name) {
    m_type_name = type_name; 
    if (m_has_short_name) {
        if (m_short_name.empty()) {
            m_unique_name = type_name;
        }
        else {
            m_unique_name = type_name + ":" + m_short_name;
        }
    }
}
