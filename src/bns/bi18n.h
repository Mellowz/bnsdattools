#ifndef BI18N_H
#define BI18N_H

#include <wx/xml/xml.h>
#include <wx/hashmap.h>

#ifndef STRINGHASHMAP
#define STRINGHASHMAP
    WX_DECLARE_STRING_HASH_MAP(wxString, StringHashMap);
#endif

enum BI18N_TYPE
{
    BI18N_XML,
    BI18N_SQLITE,
    BI18N_UNKNOWN
};

class BI18N
{
    public: BI18N();
    public: ~BI18N();

    public: void Load(wxInputStream* iStream, BI18N_TYPE iType);
    public: wxString Translate(wxString original, wxString alias = wxEmptyString);

    // a alias based translation table
    private: StringHashMap _trans_alias;
    // a text based translation table (used when alias based translation failed)
    private: StringHashMap _trans_text;
    // a table to determine the priority of the related entry in the _trans_text table
    private: StringHashMap _trans_priority;
};

#endif // BI18N_H
