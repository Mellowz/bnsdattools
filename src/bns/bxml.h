#ifndef BXML_H
#define BXML_H

#include <wx/xml/xml.h>
#include "bcrypt.h"
#include "bi18n.h"

enum BXML_TYPE
{
    BXML_PLAIN,
    BXML_BINARY,
    BXML_UNKNOWN
};

struct BXML_CONTENT
{
    public: BXML_CONTENT();
    public: ~BXML_CONTENT();

    public: void Read(wxInputStream* iStream, BXML_TYPE iType);
    public: void Write(wxOutputStream* oStream, BXML_TYPE oType);
    public: void Translate(BI18N* translator);

    private: void ReadNode(wxInputStream* iStream, wxXmlNode* parent = NULL);
    private: bool WriteNode(wxOutputStream* oStream, wxXmlNode* parent = NULL);

    wxByte* Signature;                  // 8 byte
    wxUint32 Version;                   // 4 byte
    wxUint32 FileSize;                  // 4 byte
    wxByte* Padding;                    // 64 byte
    bool Unknown;                       // 1 byte
    // TODO: add to CDATA ?
    wxUint32 OriginalPathLength;        // 4 byte
    wxByte* OriginalPath;               // 2*OriginalPathLength bytes

    wxUint32 AutoID;
    wxXmlDocument Nodes;
};

class BXML
{
    public: BXML();
    public: ~BXML();

    public: static BXML_TYPE DetectType(wxInputStream* iStream);
    public: static void Convert(wxInputStream* iStream, BXML_TYPE iType, wxOutputStream* oStream, BXML_TYPE oType);
    public: void Load(wxInputStream* iStream, BXML_TYPE iType);
    public: void Save(wxOutputStream* oStream, BXML_TYPE oType);
    public: void Translate(BI18N* translator);

    // private helper functions
    private: void ReadBinaryEntry(wxInputStream* iStream, wxXmlNode* parent = NULL);
    private: void WriteBinaryEntry(wxOutputStream* oStream, wxXmlNode* parent = NULL);

    // internal members
    private: BXML_CONTENT _content;
};

#endif // BXML_H
