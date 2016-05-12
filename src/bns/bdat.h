#ifndef BDAT_H
#define BDAT_H

#include <wx/filename.h>
#include <wx/ffile.h>
#include <wx/mstream.h>
#include <wx/dynarray.h>
#include "bcrypt.h"
#include "bi18n.h"

enum BDAT_TYPE
{
    BDAT_XML = 1,
    BDAT_PLAIN,
    BDAT_BINARY,
    BDAT_UNKNOWN
};

struct BDAT_LOOKUPTABLE
{
    public: BDAT_LOOKUPTABLE();
    public: ~BDAT_LOOKUPTABLE();

    public: void Read(wxInputStream* iStream, BDAT_TYPE iType);
    public: void Write(wxOutputStream* oStream, BDAT_TYPE oType);

    wxUint32 Size; // only for private usage...
    wxByte* Data;
};

struct BDAT_FIELDTABLE
{
    public: BDAT_FIELDTABLE();
    public: ~BDAT_FIELDTABLE();

    public: void Read(wxInputStream* iStream, BDAT_TYPE iType);
    public: void Write(wxOutputStream* oStream, BDAT_TYPE oType);

    wxUint16 Unknown1;                  // 2 byte
    wxUint16 Unknown2;                  // 2 byte
    wxUint32 Size;                      // 4 byte
    wxByte* Data;                       // Size-8 bytes
};

struct BDAT_LOOSE
{
    public: BDAT_LOOSE();
    public: ~BDAT_LOOSE();

    public: void Read(wxInputStream* iStream, BDAT_TYPE iType);
    public: void Write(wxOutputStream* oStream, BDAT_TYPE oType);

    wxUint32 FieldCountUnfixed;         // 4 byte
    wxUint32 FieldCount;
    wxUint32 SizeFields;                // 4 byte
    wxUint32 SizeLookup;                // 4 byte
    wxByte Unknown;                     // 1 byte
    BDAT_FIELDTABLE* Fields;            // SizeFields bytes
    wxInt32 SizePadding;               // 0 byte (private use only)
    wxByte* Padding;                    // (private use only)
    BDAT_LOOKUPTABLE Lookup;            // SizeLookup bytes
};

struct BDAT_SUBARCHIVE
{
    public: BDAT_SUBARCHIVE();
    public: ~BDAT_SUBARCHIVE();

    public: void Read(wxInputStream* iStream, BDAT_TYPE iType);
    public: void Write(wxOutputStream* oStream, BDAT_TYPE oType);

    wxByte* Unknown;                    // 16 byte (shorts?)
    wxUint16 SizeCompressed;            // 2 byte
    //                                  // SizeCompressed bytes
    wxUint16 SizeDecompressed;          // 2 byte
    wxUint32 FieldLookupCount;          // 4 byte
    BDAT_FIELDTABLE* Fields;            // *
    BDAT_LOOKUPTABLE* Lookups;          // *
};

struct BDAT_ARCHIVE
{
    public: BDAT_ARCHIVE();
    public: ~BDAT_ARCHIVE();

    public: void Read(wxInputStream* iStream, BDAT_TYPE iType);
    public: void Write(wxOutputStream* oStream, BDAT_TYPE oType);

    wxUint32 SubArchiveCount;           // 4 byte
    wxUint16 Unknown;                   // 2 byte
    BDAT_SUBARCHIVE* SubArchives;       // *
};

struct BDAT_COLLECTION
{
    public: BDAT_COLLECTION();
    public: ~BDAT_COLLECTION();

    public: void Read(wxInputStream* iStream, BDAT_TYPE iType);
    public: void Write(wxOutputStream* oStream, BDAT_TYPE oType);

    bool Compressed;                    // 1 byte
    wxUint8 Deprecated;                 // 1 byte
    BDAT_ARCHIVE* Archive;              // *
    BDAT_LOOSE* Loose;                  // *
};

struct BDAT_LIST
{
    public: BDAT_LIST();
    public: ~BDAT_LIST();

    public: void Read(wxInputStream* iStream, BDAT_TYPE iType);
    public: void Write(wxOutputStream* oStream, BDAT_TYPE oType);

    wxByte Unknown1;                    // 1 byte
    wxUint16 ID;                        // 2 byte
    wxUint16 Unknown2;                  // 2 byte
    wxUint16 Unknown3;                  // 2 byte
    wxUint32 Size;                      // 4 byte
    BDAT_COLLECTION Collection;         // Size bytes
};

struct BDAT_HEAD
{
    public: BDAT_HEAD();
    public: ~BDAT_HEAD();

    public: void Read(wxInputStream* iStream, BDAT_TYPE iType);
    public: void Write(wxOutputStream* oStream, BDAT_TYPE oType);

    /*
     * read/write Data only when the iStream file is not a complement file,
     * otherwise the data is only available in the corresponding 'parent' datafile.bin
     * the way to detect if this file is a complement is unknown, so we can use some hacks:
     *   Size_1 exceeds the filesize -> complement file
     *   ListCount < 200 -> complement file
     */
    bool Complement;
    wxUint32 Size_1;                    // 4 byte
    wxUint32 Size_2;                    // 4 byte
    wxUint32 Size_3;                    // 4 byte
    wxByte* Padding;                    // 62 byte
    wxByte* Data;                       // Size_1 || Size_2 bytes ?
    /* Data
    {
        wxUint32 Size_A;
        wxUint32 Size_B;
        wxUint32 Size_C;
        ...
    }*/
};

struct BDAT_CONTENT
{
    public: BDAT_CONTENT();
    public: ~BDAT_CONTENT();

    public: void Read(wxInputStream* iStream, BDAT_TYPE iType);
    public: void Write(wxOutputStream* oStream, BDAT_TYPE oType);

    wxByte* Signature;                  // 8 byte
    wxUint32 Version;                   // 4 byte
    wxByte* Unknown;                    // 9 byte
    wxUint32 ListCount;                 // 4 byte
    BDAT_HEAD HeadList;                 // *
    BDAT_LIST* Lists;                   // *
};



class BDAT
{
    public: BDAT();
    public: virtual ~BDAT();

    private: void DetectIndices();
    public: static BDAT_TYPE DetectType(wxInputStream* iStream);
    public: static void Convert(wxInputStream* iStream, BDAT_TYPE iType, wxOutputStream* oStream, BDAT_TYPE oType);
    public: static void Dump(wxInputStream* iStream, BDAT_TYPE iType, wxFileName oDirectory, BDAT_TYPE oType = BDAT_PLAIN);
    public: void Load(wxInputStream* iStream, BDAT_TYPE iType);
    public: void Save(wxOutputStream* oStream, BDAT_TYPE oType);
    public: void DumpXML(wxString directory);
    public: void DumpFaq(wxString file);
    public: void DumpGeneral(wxString file);
    public: void DumpCommand(wxString file);
    public: void TranslateFaq(BI18N* translator);
    public: void TranslateGeneral(BI18N* translator);
    public: void TranslateCommand(BI18N* translator);

    private: BDAT_CONTENT _content;
    private: wxInt32 _indexFaqs;
    private: wxInt32 _indexCommons;
    private: wxInt32 _indexCommands;
};

#endif // BDAT_H
