#include "bpkg.h"



BPKG_FTE::BPKG_FTE()
{
    FilePath = NULL;
    Padding = NULL;
}

BPKG_FTE::~BPKG_FTE()
{
    wxDELETEA(FilePath);
    wxDELETEA(Padding);
}

void BPKG_FTE::Read(wxInputStream* iStream)
{
    //
}

void BPKG_FTE::Write(wxOutputStream* oStream)
{
    //
}


/*
BPKG_HEAD::BPKG_HEAD()
{
    Signature = NULL;
    Unknown_001 = NULL;
    Unknown_002 = NULL;
}

BPKG_HEAD::~BPKG_HEAD()
{
    wxDELETEA(Signature);
    wxDELETEA(Unknown_001);
    wxDELETEA(Unknown_002);
}

void BPKG_HEAD::Read(wxInputStream* iStream)
{
    Signature = new wxByte[8];
    iStream->Read(Signature, 8);
    iStream->Read(&Version, 4);
    Unknown_001 = new wxByte[9];
    iStream->Read(Unknown_001, 9);
    iStream->Read(&FileCount, 4);
    iStream->Read(&IsCompressed, 1);
    iStream->Read(&IsEncrypted, 1);
    Unknown_002 = new wxByte[62];
    iStream->Read(Unknown_002, 62);
    iStream->Read(&FileTableSizePacked, 4);
    iStream->Read(&FileTableSizeUnpacked, 4);

    //buffer_packed = new wxByte[FileTableSizePacked];
    //iStream->Read(buffer_packed, FileTableSizePacked);

    OffsetGlobal;
}

void BPKG_HEAD::Write(wxOutputStream* oStream)
{
    //
}
*/


BPKG::BPKG()
{
    //
}

BPKG::~BPKG()
{
    //
}

void BPKG::Extract(wxInputStream* iStream, wxFileName oDirectory, bool convert, wxString* redirect)
{
    wxMBConvUTF16 enc;
    wxFileName file_path;
    wxByte* buffer_packed;
    wxByte* buffer_unpacked;

    wxByte* Signature = new wxByte[8];
    iStream->Read(Signature, 8);
    wxUint32 Version;
    iStream->Read(&Version, 4);
    wxByte* Unknown_001 = new wxByte[5];
    iStream->Read(Unknown_001, 5);
    wxUint32 FileDataSizePacked;
    iStream->Read(&FileDataSizePacked, 4);
    wxUint32 FileCount;
    iStream->Read(&FileCount, 4);
    bool IsCompressed;
    iStream->Read(&IsCompressed, 1);
    bool IsEncrypted;
    iStream->Read(&IsEncrypted, 1);
    wxByte* Unknown_002 = new wxByte[62];
    iStream->Read(Unknown_002, 62);
    wxUint32 FileTableSizePacked;
    iStream->Read(&FileTableSizePacked, 4);
    wxUint32 FileTableSizeUnpacked;
    iStream->Read(&FileTableSizeUnpacked, 4);

    buffer_packed = new wxByte[FileTableSizePacked];
    iStream->Read(buffer_packed, FileTableSizePacked);
    wxUint32 OffsetGlobal;
    iStream->Read(&OffsetGlobal, 4);
    OffsetGlobal = iStream->TellI(); // overwrite global offset, in case stored value is wrong

    if(redirect)
    {
        *redirect = wxT("Reading File Entries...");
    }
    else
    {
        wxPrintf(wxT("\rReading File Entries..."));
    }

    wxByte* FileTableUnpacked = Unpack(buffer_packed, FileTableSizePacked, FileTableSizePacked, FileTableSizeUnpacked, IsEncrypted, IsCompressed);
    wxDELETEA(buffer_packed);
    wxMemoryInputStream mis(FileTableUnpacked, FileTableSizeUnpacked);

    if(!redirect)
    {
        wxPrintf(wxT("\n"));
    }

    BPKG_FTE FileTableEntry;

    for(wxUint32 i=0; i<FileCount; i++)
    {
        if(redirect)
        {
            *redirect = wxString::Format(wxT("%i/%i"), (i+1), FileCount);
        }
        else
        {
            wxPrintf(wxT("\rExtracting Files: %i/%i"), (i+1), FileCount);
        }
        //wxYield();

        mis.Read(&(FileTableEntry.FilePathLength), 4);
        FileTableEntry.FilePath = new wxByte[2*FileTableEntry.FilePathLength+2];
        FileTableEntry.FilePath[2*FileTableEntry.FilePathLength] = '\0'; // add terminating zeros for wxString constructor
        FileTableEntry.FilePath[2*FileTableEntry.FilePathLength+1] = '\0'; // add terminating zeros for wxString constructor
        mis.Read(FileTableEntry.FilePath, 2*FileTableEntry.FilePathLength);
        mis.Read(&(FileTableEntry.Unknown_001), 1);
        mis.Read(&(FileTableEntry.IsCompressed), 1);
        mis.Read(&(FileTableEntry.IsEncrypted), 1);
        mis.Read(&(FileTableEntry.Unknown_002), 1);
        mis.Read(&(FileTableEntry.FileDataSizeUnpacked), 4);
        mis.Read(&(FileTableEntry.FileDataSizeSheared), 4);
        mis.Read(&(FileTableEntry.FileDataSizeStored), 4);
        mis.Read(&(FileTableEntry.FileDataOffset), 4);
        FileTableEntry.Padding = new wxByte[60];
        mis.SeekI(60, wxFromCurrent);

        file_path = wxFileName(oDirectory.GetPathWithSep() + wxFileName(wxString((char*)FileTableEntry.FilePath, enc), wxPATH_WIN).GetFullPath());
        file_path.Mkdir(0777, wxPATH_MKDIR_FULL);

        buffer_packed = new wxByte[FileTableEntry.FileDataSizeStored];
        iStream->SeekI(OffsetGlobal + FileTableEntry.FileDataOffset, wxFromStart);
        iStream->Read(buffer_packed, FileTableEntry.FileDataSizeStored);
        buffer_unpacked = Unpack(buffer_packed, FileTableEntry.FileDataSizeStored, FileTableEntry.FileDataSizeSheared, FileTableEntry.FileDataSizeUnpacked, FileTableEntry.IsEncrypted, FileTableEntry.IsCompressed);

        wxFFileOutputStream fos(file_path.GetFullPath());
        if(convert && (file_path.GetExt() == wxT("xml") || file_path.GetExt() == wxT("x16")))
        {
            // decode bxml
            wxMemoryInputStream tmp(buffer_unpacked, FileTableEntry.FileDataSizeUnpacked);
            BXML::Convert(&tmp, BXML_BINARY, &fos, BXML_PLAIN);
        }
        else if(convert && file_path.GetExt() == wxT("bin"))
        {
            // decode bdat
            wxMemoryInputStream tmp(buffer_unpacked, FileTableEntry.FileDataSizeUnpacked);
            BDAT::Convert(&tmp, BDAT_BINARY, &fos, BDAT_PLAIN);
        }
        else
        {
            // extract raw
            fos.Write(buffer_unpacked, FileTableEntry.FileDataSizeUnpacked);
        }

        wxDELETEA(buffer_unpacked);
        wxDELETEA(buffer_packed);
    }

    wxDELETEA(FileTableUnpacked);
    wxDELETEA(Unknown_002);
    wxDELETEA(Unknown_001);
    wxDELETEA(Signature);
}

void BPKG::Compress(wxFileName iDirectory, wxOutputStream* oStream, wxUint32 compression, wxString* redirect)
{
    wxMBConvUTF16 enc;
    wxString FilePath;
    wxFileName file_path;
    wxByte* buffer_packed;
    wxByte* buffer_unpacked;

    wxArrayString files;
    wxDir::GetAllFiles(iDirectory.GetPath(), &files);
    wxUint32 FileCount = files.Count();

    BPKG_FTE FileTableEntry;
    wxMemoryOutputStream mosTable;
    wxMemoryOutputStream mosFiles;

    #ifndef __LINUX__
        wxString TempFile = wxStandardPaths::Get().GetTempDir() + wxFileName::GetPathSeparator() + wxT("bnsdat.tmp");
    #endif

    for(wxUint32 i=0; i<FileCount; i++)
    {
        if(redirect)
        {
            *redirect = wxString::Format(wxT("%i/%i"), (i+1), FileCount);
        }
        else
        {
            wxPrintf(wxT("\rCompressing Files: %i/%i"), (i+1), FileCount);
        }

        file_path = wxFileName(files[i]);
        file_path.MakeRelativeTo(iDirectory.GetPath());
        FilePath = file_path.GetFullPath(wxPATH_WIN);
        FileTableEntry.FilePathLength = FilePath.Len();
        mosTable.Write(&(FileTableEntry.FilePathLength), 4);
        FileTableEntry.FilePath = (wxByte*)memcpy(new wxByte[2*FileTableEntry.FilePathLength], FilePath.mb_str(enc), 2*FileTableEntry.FilePathLength);
        mosTable.Write(FileTableEntry.FilePath, 2*FileTableEntry.FilePathLength);
        FileTableEntry.Unknown_001 = 2;
        mosTable.Write(&(FileTableEntry.Unknown_001), 1);
        FileTableEntry.IsCompressed = true;
        mosTable.Write(&(FileTableEntry.IsCompressed), 1);
        FileTableEntry.IsEncrypted = true;
        mosTable.Write(&(FileTableEntry.IsEncrypted), 1);
        FileTableEntry.Unknown_002 = 0;
        mosTable.Write(&(FileTableEntry.Unknown_002), 1);

        wxFFileInputStream fis(files[i]);
        #ifdef __LINUX__
            wxMemoryOutputStream tmp;
        #else
            wxFFileOutputStream tmp(TempFile);
        #endif

        if(file_path.GetExt() == wxT("xml") || file_path.GetExt() == wxT("x16"))
        {
            // encode bxml
            BXML::Convert(&fis, BXML::DetectType(&fis), &tmp, BXML_BINARY);
        }
        else if(file_path.GetExt() == wxT("bin"))
        {
            // encode bdat
            BDAT::Convert(&fis, BDAT::DetectType(&fis), &tmp, BDAT_BINARY);
        }
        else
        {
            // compress raw
            //fis.Read(tmp);
            tmp.Write(fis);
        }

        FileTableEntry.FileDataOffset = mosFiles.TellO();
        FileTableEntry.FileDataSizeUnpacked = tmp.GetSize();
        mosTable.Write(&(FileTableEntry.FileDataSizeUnpacked), 4);

        buffer_unpacked = new wxByte[FileTableEntry.FileDataSizeUnpacked];
        #ifdef __LINUX__
            tmp.CopyTo(buffer_unpacked, FileTableEntry.FileDataSizeUnpacked);
        #else
            wxFFileInputStream(TempFile).Read(buffer_unpacked, FileTableEntry.FileDataSizeUnpacked);
        #endif
        buffer_packed = Pack(buffer_unpacked, FileTableEntry.FileDataSizeUnpacked, &(FileTableEntry.FileDataSizeSheared), &(FileTableEntry.FileDataSizeStored), FileTableEntry.IsEncrypted, FileTableEntry.IsCompressed, compression);
        mosFiles.Write(buffer_packed, FileTableEntry.FileDataSizeStored);

        mosTable.Write(&(FileTableEntry.FileDataSizeSheared), 4);
        mosTable.Write(&(FileTableEntry.FileDataSizeStored), 4);
        mosTable.Write(&(FileTableEntry.FileDataOffset), 4);
        FileTableEntry.Padding = new wxByte[60];
        memset(FileTableEntry.Padding, 0, 60);
        mosTable.Write(FileTableEntry.Padding, 60);

        wxDELETEA(buffer_unpacked);
        wxDELETEA(buffer_packed);
    }

    if(redirect)
    {
        *redirect = wxT("Writing File Entries...\n");
    }
    else
    {
        wxPrintf(wxT("\nWriting File Entries...\n"));
    }

    wxByte* Signature = new wxByte[8]{'U', 'O', 'S', 'E', 'D', 'A', 'L', 'B'};
    oStream->Write(Signature, 8);
    wxUint32 Version = 2;
    oStream->Write(&Version, 4);
    wxByte* Unknown_001 = new wxByte[5]{0, 0, 0, 0, 0};
    oStream->Write(Unknown_001, 5);
    wxUint32 FileDataSizePacked = mosFiles.GetLength();
    oStream->Write(&FileDataSizePacked, 4);
    oStream->Write(&FileCount, 4);
    bool IsCompressed = true;
    oStream->Write(&IsCompressed, 1);
    bool IsEncrypted = true;
    oStream->Write(&IsEncrypted, 1);
    wxByte* Unknown_002 = new wxByte[62];
    memset(Unknown_002, 0, 62);
    oStream->Write(Unknown_002, 62);

    wxUint32 FileTableSizeUnpacked = mosTable.GetLength();
    wxUint32 FileTableSizeSheared = FileTableSizeUnpacked;
    wxUint32 FileTableSizePacked = FileTableSizeUnpacked;
    buffer_unpacked = new wxByte[FileTableSizeUnpacked];
    mosTable.CopyTo(buffer_unpacked, FileTableSizeUnpacked);
    buffer_packed = Pack(buffer_unpacked, FileTableSizeUnpacked, &FileTableSizeSheared, &FileTableSizePacked, IsEncrypted, IsCompressed, compression);
    wxDELETEA(buffer_unpacked);

    oStream->Write(&FileTableSizePacked, 4);
    oStream->Write(&FileTableSizeUnpacked, 4);
    oStream->Write(buffer_packed, FileTableSizePacked);
    wxDELETEA(buffer_packed);
    wxUint32 OffsetGlobal = oStream->TellO()+4;
    oStream->Write(&OffsetGlobal, 4);

    buffer_packed = new wxByte[FileDataSizePacked];
    mosFiles.CopyTo(buffer_packed, FileDataSizePacked);
    oStream->Write(buffer_packed, FileDataSizePacked);
    wxDELETEA(buffer_packed);

    wxDELETEA(Unknown_002);
    wxDELETEA(Unknown_001);
    wxDELETEA(Signature);

    #ifndef __LINUX__
        wxRemoveFile(TempFile);
    #endif
}

wxByte* BPKG::Unpack(wxByte* buffer, wxUint32 sizeStored, wxUint32 sizeSheared, wxUint32 sizeUnpacked, bool isEncrypted, bool isCompressed)
{
    wxByte* out = buffer;

    if(isEncrypted)
    {
        wxUint32 padsize;
        wxByte* tmp = BCRYPT::Decrypt(out, sizeStored, &padsize);
        if(out != buffer)
        {
            wxDELETEA(out);
        }
        out = tmp;
    }

    if(isCompressed)
    {
        wxByte* tmp = BCRYPT::Deflate(out, sizeSheared, sizeUnpacked);
        if(out != buffer)
        {
            wxDELETEA(out);
        }
        out = tmp;
    }

    // neither encrypted, nor compressed -> raw copy
    if(out == buffer)
    {
        out = new wxByte[sizeUnpacked];
        if(sizeSheared < sizeUnpacked)
        {
            memcpy(out, buffer, sizeSheared);
        }
        else
        {
            memcpy(out, buffer, sizeUnpacked);
        }
    }

    return out;
}

wxByte* BPKG::Pack(wxByte* buffer, wxUint32 sizeUnpacked, wxUint32* sizeSheared, wxUint32* sizeStored, bool encrypt, bool compress, wxUint32 compressionLevel)
{
    wxByte* out = buffer;
    *sizeSheared = sizeUnpacked;
    *sizeStored = *sizeSheared;

    if(compress)
    {
        wxByte* tmp = BCRYPT::Inflate(out, sizeUnpacked, sizeSheared, compressionLevel);
        if(out != buffer)
        {
            wxDELETEA(out);
        }
        *sizeStored = *sizeSheared;
        out = tmp;
    }

    if(encrypt)
    {
        wxByte* tmp = BCRYPT::Encrypt(out, *sizeSheared, sizeStored);
        if(out != buffer)
        {
            wxDELETEA(out);
        }
        out = tmp;
    }

    // neither encrypt, nor compress -> raw copy
    if(out == buffer)
    {
        if(*sizeSheared < *sizeStored)
        {
            out = new wxByte[*sizeStored];
            memcpy(out, buffer, *sizeSheared);
        }
        else
        {
            memcpy(out, buffer, *sizeStored);
        }
    }

    return out;
}
