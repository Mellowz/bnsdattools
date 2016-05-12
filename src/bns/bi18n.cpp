#include "bi18n.h"

BI18N::BI18N()
{
    //
}

BI18N::~BI18N()
{
    //
}

void BI18N::Load(wxInputStream* iStream, BI18N_TYPE iType)
{
    if(iType == BI18N_XML)
    {
        wxXmlDocument table;
        if(table.Load(*iStream, wxT("UTF-8")))
        {
            wxXmlNode* text = table.GetRoot()->GetChildren();
            wxString alias;
            wxString priority;
            wxString original;
            wxString translated;
            while(text)
            {
                alias = text->GetAttribute(wxT("alias"), wxEmptyString);
                priority = text->GetAttribute(wxT("priority"), wxT("0"));
                original = text->GetChildren()->GetNodeContent();
                translated = text->GetChildren()->GetNext()->GetNodeContent();
                if(!alias.IsEmpty())
                {
                    _trans_alias[alias] = translated;
                }
                if(_trans_text[original].IsEmpty() /*|| _trans_priority[original].IsEmpty() / NOTE: _trans_text[priority] is in sync with _trans_text[original]*/ || wxAtoi(_trans_priority[original]) < wxAtoi(priority))
                {
                    _trans_text[original] = translated;
                    _trans_priority[original] = priority;
                }
                text = text->GetNext();
            }
            //wxPrintf(wxT("Size: %i, %i=%i\n"), _trans_alias.size(), _trans_text.size(), _trans_priority.size());
        }
    }

    if(iType == BI18N_SQLITE)
    {
        //
    }
}

wxString BI18N::Translate(wxString original, wxString alias)
{
    wxString translated = wxEmptyString;
    if(!alias.IsEmpty())
    {
        translated = _trans_alias[alias];
    }
    if(translated.IsEmpty() && !original.IsEmpty())
    {
        translated = _trans_text[original];
    }
    return translated;
}
