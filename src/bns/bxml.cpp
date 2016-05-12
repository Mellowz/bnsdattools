#include "bxml.h"

BXML_CONTENT::BXML_CONTENT()
{
    Signature = NULL;
    Padding = NULL;
    OriginalPath = NULL;
}

BXML_CONTENT::~BXML_CONTENT()
{
    wxDELETEA(Signature);
    wxDELETEA(Padding);
    wxDELETEA(OriginalPath);
}

void BXML_CONTENT::Read(wxInputStream* iStream, BXML_TYPE iType)
{
    if(iType == BXML_PLAIN)
    {
        wxDELETEA(Signature);
        Signature = new wxByte[8]{'L', 'M', 'X', 'B', 'O', 'S', 'L', 'B'};
        Version = 3;
        FileSize = 85;
        wxDELETEA(Padding);
        Padding = new wxByte[64];
        memset(Padding, 0, 64);
        Unknown = true;
        OriginalPathLength = 0;
        wxDELETEA(OriginalPath);

        // NOTE: keep whitespace text nodes (to be compliant with the whitespace TEXT_NODES in bns xml)
        // no we don't keep them, we remove them because it is cleaner
        if(!Nodes.Load(*iStream, wxT("UTF-8")/*, wxXMLDOC_KEEP_WHITESPACE_NODES*/))
        {
            wxPrintf(wxT("ERROR Loading XML Stream\n"));
        }

        // get original path from first comment node
        wxXmlNode* node = Nodes.GetRoot()->GetChildren();
        if(node && node->GetType() == wxXML_COMMENT_NODE)
        {
            wxMBConvUTF16 enc;
            wxString Text = node->GetContent();
            OriginalPathLength = Text.Len();
            OriginalPath = (wxByte*)memcpy(new wxByte[2*OriginalPathLength], Text.mb_str(enc), 2*OriginalPathLength);
            BCRYPT::Xor(OriginalPath, 2*OriginalPathLength);
        }
        else
        {
            OriginalPath = new wxByte[2*OriginalPathLength];
        }
    }

    if(iType == BXML_BINARY)
    {
        wxDELETEA(Signature);
        Signature = new wxByte[8];
        iStream->Read(Signature, 8);
        iStream->Read(&Version, 4);
        iStream->Read(&FileSize, 4);
        wxDELETEA(Padding);
        Padding = new wxByte[64];
        iStream->Read(Padding, 64);
        iStream->Read(&Unknown, 1);
        iStream->Read(&OriginalPathLength, 4);
        wxDELETEA(OriginalPath);
        OriginalPath = new wxByte[2*OriginalPathLength];
        iStream->Read(OriginalPath, 2*OriginalPathLength);

        AutoID = 1;
        ReadNode(iStream);

        // add original path as first comment node
        wxMBConvUTF16 enc;
        wxByte* Path = (wxByte*)memcpy(new wxByte[2*OriginalPathLength + 2], OriginalPath, 2*OriginalPathLength);
        Path[2*OriginalPathLength] = 0;
        Path[2*OriginalPathLength + 1] = 0;
        BCRYPT::Xor(Path, 2*OriginalPathLength);
        wxXmlNode* node = new wxXmlNode(wxXML_COMMENT_NODE, wxEmptyString, wxString((char*)Path, enc));
        node->SetNext(Nodes.GetRoot()->GetChildren());
        Nodes.GetRoot()->SetChildren(node);

        if(FileSize != iStream->TellI())
        {
            wxPrintf(wxString::Format(wxT("ERROR: FILESIZE MISMATCH (%i -> %i)\n"), FileSize, iStream->TellI()));
        }
    }
}

void BXML_CONTENT::Write(wxOutputStream* oStream, BXML_TYPE oType)
{
    if(oType == BXML_PLAIN)
    {
        // NOTE: disable auto indentation (whitespace TEXT_NODES from bns xml will be used if present)
        if(!Nodes.Save(*oStream/*, wxXML_NO_INDENTATION*/))
        {
            wxPrintf(wxT("ERROR Saving XML Stream\n"));
        }
    }

    if(oType == BXML_BINARY)
    {
        oStream->Write(Signature, 8);
        oStream->Write(&Version, 4);
        oStream->Write(&FileSize, 4);
        oStream->Write(Padding, 64);
        oStream->Write(&Unknown, 1);
        oStream->Write(&OriginalPathLength, 4);
        oStream->Write(OriginalPath, 2*OriginalPathLength);

        AutoID = 1;
        WriteNode(oStream);

        FileSize = oStream->TellO();
        oStream->SeekO(12, wxFromStart);
        oStream->Write(&FileSize, 4);
    }
}

void BXML_CONTENT::Translate(BI18N* translator)
{
    // detect type of xml (subtitle/survey)
    wxString type = Nodes.GetRoot()->GetAttribute(wxT("type"), wxEmptyString);
    wxString translated;

    if(type.IsSameAs(wxT("text")))
    {
        wxXmlNode* child = Nodes.GetRoot()->GetChildren();
        while(child)
        {
            if(child->GetName().IsSameAs(wxT("record")))
            {
                translated = translator->Translate(child->GetChildren()->GetContent(), child->GetAttribute(wxT("alias"), wxEmptyString));
                if(!translated.IsEmpty())
                {
                    child->GetChildren()->SetContent(translated);
                }
            }
            child = child->GetNext();
        }
    }

    if(type.IsSameAs(wxT("petition-faq-list")))
    {
        wxXmlNode* survey = Nodes.GetRoot()->GetChildren();
        wxXmlNode* question;
        wxXmlNode* answer;
        wxString alias;
        wxString question_num;
        wxString answer_num;
        while(survey)
        {
            if(survey->GetName().IsSameAs(wxT("surveyQuestion")))
            {
                alias = survey->GetAttribute(wxT("alias"), wxEmptyString);
                translated = translator->Translate(survey->GetAttribute(wxT("greeting"), wxEmptyString), alias);
                if(!translated.IsEmpty())
                {
                    survey->DeleteProperty(wxT("greeting"));
                    survey->AddAttribute(wxT("greeting"), translated);
                }
                translated = translator->Translate(survey->GetAttribute(wxT("title"), wxEmptyString));
                if(!translated.IsEmpty())
                {
                    survey->DeleteProperty(wxT("title"));
                    survey->AddAttribute(wxT("title"), translated);
                }
                question = survey->GetChildren();
                while(question)
                {
                    if(question->GetName().IsSameAs(wxT("question")))
                    {
                        question_num = question->GetAttribute(wxT("num"), wxEmptyString);
                        translated = translator->Translate(question->GetAttribute(wxT("desc"), wxEmptyString), alias + wxT("_") + question_num);
                        if(!translated.IsEmpty())
                        {
                            question->DeleteProperty(wxT("desc"));
                            question->AddAttribute(wxT("desc"), translated);
                        }
                        answer = question->GetChildren();
                        while(answer)
                        {
                            if(answer->GetName().IsSameAs(wxT("questionExample")))
                            {
                                answer_num = answer->GetAttribute(wxT("num"), wxEmptyString);
                                translated = translator->Translate(answer->GetAttribute(wxT("desc"), wxEmptyString), alias + wxT("_") + question_num + wxT("_") + answer_num);
                                if(!translated.IsEmpty())
                                {
                                    answer->DeleteProperty(wxT("desc"));
                                    answer->AddAttribute(wxT("desc"), translated);
                                }
                            }
                            answer = answer->GetNext();
                        }
                    }
                    question = question->GetNext();
                }
            }
            survey = survey->GetNext();
        }
    }
}

void BXML_CONTENT::ReadNode(wxInputStream* iStream, wxXmlNode* parent)
{
    wxMBConvUTF16 enc;

    wxXmlNode* node = NULL;

    wxUint32 Type = 1;
    if(parent)
    {
        iStream->Read(&Type, 4);
    }

    if(Type == 1)
    {
        node = new wxXmlNode(wxXML_ELEMENT_NODE, wxEmptyString);

        wxUint32 ParameterCount;
        iStream->Read(&ParameterCount, 4);
        for(wxUint32 i=0; i<ParameterCount; i++)
        {
            wxUint32 NameLength;
            iStream->Read(&NameLength, 4);
            wxByte* Name = new wxByte[2*NameLength + 2];
            Name[2*NameLength] = 0;
            Name[2*NameLength + 1] = 0;
            iStream->Read(Name, 2*NameLength);
            BCRYPT::Xor(Name, 2*NameLength);

            wxUint32 ValueLength;
            iStream->Read(&ValueLength, 4);
            wxByte* Value = new wxByte[2*ValueLength + 2];
            Value[2*ValueLength] = 0;
            Value[2*ValueLength + 1] = 0;
            iStream->Read(Value, 2*ValueLength);
            BCRYPT::Xor(Value, 2*ValueLength);

            node->AddAttribute(wxString((char*)Name, enc), wxString((char*)Value, enc));

            wxDELETEA(Name);
            wxDELETEA(Value);
        }
    }

    if(Type == 2)
    {
        node = new wxXmlNode(wxXML_TEXT_NODE, wxEmptyString);

        wxUint32 TextLength;
        iStream->Read(&TextLength, 4);
        wxByte* Text = new wxByte[2*TextLength + 2];
        Text[2*TextLength] = 0;
        Text[2*TextLength + 1] = 0;
        iStream->Read(Text, 2*TextLength);
        BCRYPT::Xor(Text, 2*TextLength);

        node->SetContent(wxString((char*)Text, enc).Trim(true).Trim(false));

        wxDELETEA(Text);
    }

    if(Type > 2)
    {
        wxPrintf(wxString::Format(wxT("ERROR: XML NODE TYPE [%i] UNKNOWN\n"), Type));
    }

    bool Closed;
    iStream->Read(&Closed, 1);
    wxUint32 TagLength;
    iStream->Read(&TagLength, 4);
    wxByte* Tag = new wxByte[2*TagLength + 2];
    Tag[2*TagLength] = 0;
    Tag[2*TagLength + 1] = 0;
    iStream->Read(Tag, 2*TagLength);
    BCRYPT::Xor(Tag, 2*TagLength);

    node->SetName(wxString((char*)Tag, enc));

    wxUint32 ChildCount;
    iStream->Read(&ChildCount, 4);
    iStream->Read(&AutoID, 4);
    AutoID++;

    for(wxUint32 i=0; i<ChildCount; i++)
    {
        ReadNode(iStream, node);
    }

    if(parent)
    {
        //if(Type == 1 || (Type == 2 && !node->IsWhitespaceOnly()))
        if(Type != 2 || node->GetContent() != wxEmptyString)
        {
            parent->AddChild(node);
        }
    }
    else
    {
        Nodes.SetRoot(node);
    }
}

bool BXML_CONTENT::WriteNode(wxOutputStream* oStream, wxXmlNode* parent)
{
    //wxCSConv enc(wxFONTENCODING_UTF16);
    wxMBConvUTF16 enc;

    wxXmlNode* node = NULL;

    wxUint32 Type = 1;
    if(parent)
    {
        node = parent;
        Type = node->GetType();
        if(Type == wxXML_ELEMENT_NODE)
        {
            Type = 1;
        }
        if(Type == wxXML_TEXT_NODE)
        {
            Type = 2;
        }
        if(Type == wxXML_COMMENT_NODE)
        {
            return false;
        }
        oStream->Write(&Type, 4);
    }
    else
    {
        node = Nodes.GetRoot();
    }

    if(Type == 1)
    {
        wxUint32 OffsetAttributeCount = oStream->TellO();
        wxUint32 AttributeCount = 0;
        oStream->Write(&AttributeCount, 4);

        wxXmlProperty* attribute = node->GetAttributes();
        while(attribute)
        {
            wxString Name = attribute->GetName();
            wxUint32 NameLength = Name.Len();
            oStream->Write(&NameLength, 4);
            wxByte* NameBuffer = (wxByte*)memcpy(new wxByte[2*NameLength], Name.mb_str(enc), 2*NameLength);
            BCRYPT::Xor(NameBuffer, 2*NameLength);
            oStream->Write(NameBuffer, 2*NameLength);
            wxDELETEA(NameBuffer);

            wxString Value = attribute->GetValue();
            wxUint32 ValueLength = Value.Len();
            oStream->Write(&ValueLength, 4);
            wxByte* ValueBuffer = (wxByte*)memcpy(new wxByte[2*ValueLength], Value.mb_str(enc), 2*ValueLength);
            BCRYPT::Xor(ValueBuffer, 2*ValueLength);
            oStream->Write(ValueBuffer, 2*ValueLength);
            wxDELETEA(ValueBuffer);

            attribute = attribute->GetNext();
            AttributeCount++;
        }

        wxUint32 OffsetCurrent = oStream->TellO();
        oStream->SeekO(OffsetAttributeCount, wxFromStart);
        oStream->Write(&AttributeCount, 4);
        oStream->SeekO(OffsetCurrent, wxFromStart);
    }

    if(Type == 2)
    {
        wxString Text = node->GetContent();
        wxUint32 TextLength = Text.Len();
        oStream->Write(&TextLength, 4);
        wxByte* TextBuffer = (wxByte*)memcpy(new wxByte[2*TextLength], Text.mb_str(enc), 2*TextLength);
        BCRYPT::Xor(TextBuffer, 2*TextLength);
        oStream->Write(TextBuffer, 2*TextLength);

        wxDELETEA(TextBuffer);
    }

    if(Type > 2)
    {
        wxPrintf(wxString::Format(wxT("ERROR: XML NODE TYPE [%i] UNKNOWN\n"), Type));
    }

    bool Closed = true;
    oStream->Write(&Closed, 1);
    wxString Tag = node->GetName();
    wxUint32 TagLength = Tag.Len();
    oStream->Write(&TagLength, 4);
    wxByte* TagBuffer = (wxByte*)memcpy(new wxByte[2*TagLength], Tag.mb_str(enc), 2*TagLength);
    BCRYPT::Xor(TagBuffer, 2*TagLength);
    oStream->Write(TagBuffer, 2*TagLength);
    wxDELETEA(TagBuffer);

    wxUint32 OffsetChildCount = oStream->TellO();
    wxUint32 ChildCount = 0;
    oStream->Write(&ChildCount, 4);
    oStream->Write(&AutoID, 4);
    AutoID++;

    wxXmlNode* child = node->GetChildren();
    while(child)
    {
        if(WriteNode(oStream, child))
        {
            ChildCount++;
        }
        child = child->GetNext();
    }

    wxUint32 OffsetCurrent = oStream->TellO();
    oStream->SeekO(OffsetChildCount, wxFromStart);
    oStream->Write(&ChildCount, 4);
    oStream->SeekO(OffsetCurrent, wxFromStart);

    return true;
}



BXML::BXML()
{
    //
}

BXML::~BXML()
{
    //
}

BXML_TYPE BXML::DetectType(wxInputStream* iStream)
{
    wxUint32 offset = iStream->TellI();
    iStream->SeekI(0);
    wxByte* Signature = new wxByte[8];
    iStream->Read(Signature, 8);
    iStream->SeekI(offset);

    BXML_TYPE result = BXML_UNKNOWN;

    if(
        Signature[0] == '<' &&
        Signature[1] == '?' &&
        Signature[2] == 'x' &&
        Signature[3] == 'm' &&
        Signature[4] == 'l'
    )
    {
        result = BXML_PLAIN;
    }

    if(
        Signature[7] == 'B' &&
        Signature[6] == 'L' &&
        Signature[5] == 'S' &&
        Signature[4] == 'O' &&
        Signature[3] == 'B' &&
        Signature[2] == 'X' &&
        Signature[1] == 'M' &&
        Signature[0] == 'L'
    )
    {
        result = BXML_BINARY;
    }

    wxDELETEA(Signature);

    return result;
}

void BXML::Convert(wxInputStream* iStream, BXML_TYPE iType, wxOutputStream* oStream, BXML_TYPE oType)
{
    if(iType == BXML_PLAIN && oType == BXML_BINARY)
    {
        BXML bns_xml;
        bns_xml.Load(iStream, iType);
        bns_xml.Save(oStream, oType);
    }
    else if(iType == BXML_BINARY && oType == BXML_PLAIN)
    {
        BXML bns_xml;
        bns_xml.Load(iStream, iType);
        bns_xml.Save(oStream, oType);
    }
    else
    {
        iStream->Read(*oStream);
    }
}

void BXML::Load(wxInputStream* iStream, BXML_TYPE iType)
{
    _content.Read(iStream, iType);
}

void BXML::Save(wxOutputStream* oStream, BXML_TYPE oType)
{
    _content.Write(oStream, oType);
}

void BXML::Translate(BI18N* translator)
{
    _content.Translate(translator);
}
