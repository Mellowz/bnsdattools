#ifndef BPKG_H
#define BPKG_H

#include <wx/wfstream.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include "bxml.h"
#include "bdat.h"

// TODO: use sophisticated structures to modularize the compress/decompress functions

struct BPKG_FTE
{
    BPKG_FTE();
    ~BPKG_FTE();

    public: void Read(wxInputStream* iStream);
    public: void Write(wxOutputStream* oStream);

    wxUint32 FilePathLength;
    wxByte* FilePath;
    wxByte Unknown_001;
    bool IsCompressed;
    bool IsEncrypted;
    wxByte Unknown_002;
    wxUint32 FileDataSizeUnpacked;
    wxUint32 FileDataSizeSheared; // without padding for AES
    wxUint32 FileDataSizeStored;
    wxUint32 FileDataOffset; // (relative) offset
    wxByte* Padding;
};

/*
struct BPKG_HEAD
{
    BPKG_HEAD();
    ~BPKG_HEAD();

    public: void Read(wxInputStream* iStream);
    public: void Write(wxOutputStream* oStream);

    wxByte* Signature;                              // 8 byte
    wxUint32 Version;                               // 4 byte
    wxByte* Unknown_001;                            // 9 byte
    wxUint32 FileCount;                             // 4 byte
    bool IsCompressed;                              // 1 byte
    bool IsEncrypted;                               // 1 byte
    wxByte* Unknown_002;                            // 62 byte
    wxUint32 FileTableSizePacked;                   // 4 byte
    wxUint32 FileTableSizeUnpacked;                 // 4 byte
    //buffer_packed;                                // FileTableSizePacked bytes
    wxUint32 OffsetGlobal;                          // 4 byte
};
*/

class BPKG
{
    public: BPKG();
    public: ~BPKG();

    public: static void Extract(wxInputStream* iStream, wxFileName oDirectory, bool convert = false, wxString* redirect = NULL);
    public: static void Compress(wxFileName iDirectory, wxOutputStream* oStream, wxUint32 compression = 6, wxString* redirect = NULL);

    private: static wxByte* Unpack(wxByte* buffer, wxUint32 sizeStored, wxUint32 sizeSheared, wxUint32 sizeUnpacked, bool isEncrypted, bool isCompressed);
    private: static wxByte* Pack(wxByte* buffer, wxUint32 sizeUnpacked, wxUint32* sizeSheared, wxUint32* sizeStored, bool encrypt, bool compress, wxUint32 compressionLevel = 1);
};

#endif // BPKG_H
