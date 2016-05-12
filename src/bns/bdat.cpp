#include "bdat.h"

//#define VERBOSE_OUT

wxArrayString LookupSplitToWords(wxByte* data, wxUint32 size)
{
    wxArrayString words;
    wxMBConvUTF16 enc;
    wxUint32 start = 0;
    wxUint32 end = 0;
    while(end < size)
    {
        if(data[end] == 0 && data[end+1] == 0)
        {
            wxByte* ptr = data+start;
            wxByte* tmp = (wxByte*)memcpy((void*)(new wxByte[(end+2)-start]), ptr, (end+2)-start);
            words.Add(wxString((char*)tmp, enc));
            ptr = NULL;
            wxDELETEA(tmp);
            start = end+2;
        }
        end += 2;
    }
    return words;
}



BDAT_LOOKUPTABLE::BDAT_LOOKUPTABLE()
{
    Data = NULL;
}

BDAT_LOOKUPTABLE::~BDAT_LOOKUPTABLE()
{
    wxDELETEA(Data);
}

void BDAT_LOOKUPTABLE::Read(wxInputStream* iStream, BDAT_TYPE iType)
{
    Data = new wxByte[Size];
    iStream->Read(Data, Size);
}

void BDAT_LOOKUPTABLE::Write(wxOutputStream* oStream, BDAT_TYPE oType)
{
    oStream->Write(Data, Size);
}



BDAT_FIELDTABLE::BDAT_FIELDTABLE()
{
    Data = NULL;
}

BDAT_FIELDTABLE::~BDAT_FIELDTABLE()
{
    wxDELETEA(Data);
}

void BDAT_FIELDTABLE::Read(wxInputStream* iStream, BDAT_TYPE iType)
{
    iStream->Read(&Unknown1, 2);
    iStream->Read(&Unknown2, 2);
    iStream->Read(&Size, 4);
    Data = new wxByte[Size-8];
    iStream->Read(Data, Size-8);
}

void BDAT_FIELDTABLE::Write(wxOutputStream* oStream, BDAT_TYPE oType)
{
    oStream->Write(&Unknown1, 2);
    oStream->Write(&Unknown2, 2);
    oStream->Write(&Size, 4);
    oStream->Write(Data, Size-8);
}



BDAT_LOOSE::BDAT_LOOSE()
{
    Fields = NULL;
    Padding = NULL;
}

BDAT_LOOSE::~BDAT_LOOSE()
{
    wxDELETEA(Fields);
    wxDELETEA(Padding);
}

void BDAT_LOOSE::Read(wxInputStream* iStream, BDAT_TYPE iType)
{
    iStream->Read(&FieldCount, 4);
    FieldCountUnfixed = FieldCount;
    iStream->Read(&SizeFields, 4);
    iStream->Read(&SizeLookup, 4);
    iStream->Read(&Unknown, 1);

    wxUint32 OffsetCurrent;
    wxUint32 OffsetStart = iStream->TellI();
    wxUint32 OffsetExpected = OffsetStart + SizeFields;

    Fields = new BDAT_FIELDTABLE[FieldCount];
    for(wxUint32 i=0; i<FieldCount; i++)
    {
        OffsetCurrent = iStream->TellI();
        if(OffsetCurrent >= OffsetExpected)
        {
            #ifdef VERBOSE_OUT
            wxPrintf(wxT("  Fixed Fieldcount: %i->%i\n"), FieldCount, i);
            #endif
            FieldCount = i;
            iStream->SeekI(OffsetExpected-OffsetCurrent, wxFromCurrent);
            break;
        }
        Fields[i].Read(iStream, iType);
    }
    #ifdef VERBOSE_OUT
    wxPrintf(wxT("  Fieldcount: %i\n"), FieldCount);
    #endif
    OffsetCurrent = iStream->TellI();
    SizePadding = OffsetExpected - OffsetCurrent;
    if(SizePadding < 0)
    {
        wxPrintf(wxT("CRITICAL ERROR: Negative psize of padding bytes\n"));
        exit(255);
    }
    if(SizePadding > 0)
    {
        SizePadding = OffsetExpected - OffsetCurrent;
        #ifdef VERBOSE_OUT
        wxPrintf(wxT("  Fixed Padding: %i Bytes\n"), SizePadding);
        #endif
        Padding = new wxByte[SizePadding];
        iStream->Read(Padding, SizePadding);
    }

    Lookup.Size = SizeLookup;
    Lookup.Read(iStream, iType);
}

void BDAT_LOOSE::Write(wxOutputStream* oStream, BDAT_TYPE oType)
{
    oStream->Write(&FieldCountUnfixed, 4);
    wxUint32 OffsetSizes = oStream->TellO();
    oStream->Write(&SizeFields, 4);
    oStream->Write(&SizeLookup, 4);
    oStream->Write(&Unknown, 1);

    wxUint32 OffsetStart = oStream->TellO();
    for(wxUint32 i=0; i<FieldCount; i++)
    {
        Fields[i].Write(oStream, oType);
    }
    if(SizePadding < 0)
    {
        wxPrintf(wxT("CRITICAL ERROR: Negative psize of padding bytes\n"));
        exit(255);
    }
    if(SizePadding > 0) {
        oStream->Write(Padding, SizePadding);
    }
    SizeFields = oStream->TellO() - OffsetStart;
    Lookup.Size = SizeLookup;
    Lookup.Write(oStream, oType);
    SizeLookup = oStream->TellO() - OffsetStart - SizeFields;

    oStream->SeekO(OffsetSizes, wxFromStart);
    oStream->Write(&SizeFields, 4);
    oStream->Write(&SizeLookup, 4);
    oStream->SeekO(1 + SizeFields + SizeLookup, wxFromCurrent);
}



BDAT_SUBARCHIVE::BDAT_SUBARCHIVE()
{
    Unknown = NULL;
    Fields = NULL;
    Lookups = NULL;
}

BDAT_SUBARCHIVE::~BDAT_SUBARCHIVE()
{
    wxDELETEA(Unknown);
    wxDELETEA(Fields);
    wxDELETEA(Lookups);
}

void BDAT_SUBARCHIVE::Read(wxInputStream* iStream, BDAT_TYPE iType)
{
    Unknown = new wxByte[16];
    iStream->Read(Unknown, 16);
    iStream->Read(&SizeCompressed, 2);
    wxByte* DataCompressed = new wxByte[SizeCompressed];
    iStream->Read(DataCompressed, SizeCompressed);
    iStream->Read(&SizeDecompressed, 2);
    wxByte* DataDecompressed = BCRYPT::Deflate(DataCompressed, SizeCompressed, SizeDecompressed);
    iStream->Read(&FieldLookupCount, 4);

    Fields = new BDAT_FIELDTABLE[FieldLookupCount];
    Lookups = new BDAT_LOOKUPTABLE[FieldLookupCount];
    wxMemoryInputStream mis(DataDecompressed, SizeDecompressed);

    wxUint16 DataOffset;
    iStream->Read(&DataOffset, 2);
    for(wxUint32 i=1; i<=FieldLookupCount; i++)
    {
        mis.SeekI(DataOffset, wxFromStart);
        Fields[i-1].Read(&mis, iType);

        if(i < FieldLookupCount)
        {
            iStream->Read(&DataOffset, 2);
        }
        else
        {
            DataOffset = SizeDecompressed;
        }

        Lookups[i-1].Size = DataOffset - mis.TellI();
        Lookups[i-1].Read(&mis, iType);
    }

    wxDELETEA(DataCompressed);
    wxDELETEA(DataDecompressed);
}

void BDAT_SUBARCHIVE::Write(wxOutputStream* oStream, BDAT_TYPE oType)
{
    wxMemoryOutputStream mos;

    wxUint16* DataOffsets = new wxUint16[FieldLookupCount];
    DataOffsets[0] = 0;
    for(wxUint32 i=1; i<=FieldLookupCount; i++)
    {
        Fields[i-1].Write(&mos, oType);
        Lookups[i-1].Write(&mos, oType);
        if(i < FieldLookupCount)
        {
            DataOffsets[i] = mos.TellO();
        }
    }
    SizeDecompressed = mos.GetSize();
    if(mos.GetSize() > (1<<16)-1) {
        wxPrintf(wxT("CRITICAL ERROR: Subarchive decompressed size overflow\n"));
        exit(255);
    }
    wxByte* DataDecompressed = new wxByte[SizeDecompressed];
    mos.CopyTo(DataDecompressed, SizeDecompressed);
    wxUint32 SizeCompressedNew;
    wxByte* DataCompressed = BCRYPT::Inflate(DataDecompressed, SizeDecompressed, &SizeCompressedNew, 6);
    SizeCompressed = (wxUint16)SizeCompressedNew;

    oStream->Write(Unknown, 16);
    oStream->Write(&SizeCompressed, 2);
    oStream->Write(DataCompressed, SizeCompressed);
    oStream->Write(&SizeDecompressed, 2);
    oStream->Write(&FieldLookupCount, 4);
    for(wxUint32 i=0; i<FieldLookupCount; i++)
    {
        oStream->Write(&(DataOffsets[i]), 2);
    }

    wxDELETEA(DataDecompressed);
    wxDELETEA(DataCompressed);
    wxDELETEA(DataOffsets);
}



BDAT_ARCHIVE::BDAT_ARCHIVE()
{
    SubArchives = NULL;
}

BDAT_ARCHIVE::~BDAT_ARCHIVE()
{
    wxDELETEA(SubArchives);
}

void BDAT_ARCHIVE::Read(wxInputStream* iStream, BDAT_TYPE iType)
{
    iStream->Read(&SubArchiveCount, 4);
    iStream->Read(&Unknown, 2);

    SubArchives = new BDAT_SUBARCHIVE[SubArchiveCount];
    for(wxUint32 i=0; i<SubArchiveCount; i++)
    {
        SubArchives[i].Read(iStream, iType);
    }
}

void BDAT_ARCHIVE::Write(wxOutputStream* oStream, BDAT_TYPE oType)
{
    oStream->Write(&SubArchiveCount, 4);
    oStream->Write(&Unknown, 2);

    for(wxUint32 i=0; i<SubArchiveCount; i++)
    {
        SubArchives[i].Write(oStream, oType);
    }
}



BDAT_COLLECTION::BDAT_COLLECTION()
{
    Archive = NULL;
    Loose = NULL;
}

BDAT_COLLECTION::~BDAT_COLLECTION()
{
    wxDELETE(Archive);
    wxDELETE(Loose);
}

void BDAT_COLLECTION::Read(wxInputStream* iStream, BDAT_TYPE iType)
{
    iStream->Read(&Compressed, 1);

    if(Compressed)
    {
        if(*((wxUint8*)(&Compressed)) > 1){iStream->SeekI(iStream->TellI()-1);} // NOTE: compressed byte is missing for compressed lists in russian pserver
        Archive = new BDAT_ARCHIVE();
        Archive->Read(iStream, iType);
        wxDELETEA(Loose);
        Loose = NULL;
        if(*((wxUint8*)(&Compressed)) > 1){iStream->Read(&Deprecated, 1);} // NOTE: read unknown byte after data for compressed lists in russian pserver
    }
    else
    {
        Loose = new BDAT_LOOSE();
        Loose->Read(iStream, iType);
        wxDELETEA(Archive);
        Archive = NULL;
    }
}

void BDAT_COLLECTION::Write(wxOutputStream* oStream, BDAT_TYPE oType)
{
    oStream->Write(&Compressed, 1);
    if(Compressed)
    {
        if(*((wxUint8*)(&Compressed)) > 1){oStream->SeekO(oStream->TellO()-1);} // NOTE: remove compressed byte for compressed lists in russian pserver
        Archive->Write(oStream, oType);
        if(*((wxUint8*)(&Compressed)) > 1){oStream->Write(&Deprecated, 1);} // NOTE: write unknown byte after data for compressed lists in russian pserver
    }
    else
    {
        Loose->Write(oStream, oType);
    }
}



BDAT_LIST::BDAT_LIST()
{
    //
}

BDAT_LIST::~BDAT_LIST()
{
    //
}

void BDAT_LIST::Read(wxInputStream* iStream, BDAT_TYPE iType)
{
    #ifdef VERBOSE_OUT
    wxUint32 pos = iStream->TellI();
    #endif
    iStream->Read(&Unknown1, 1);
    iStream->Read(&ID, 2);
    iStream->Read(&Unknown2, 2);
    iStream->Read(&Unknown3, 2);
    iStream->Read(&Size, 4);
    #ifdef VERBOSE_OUT
    wxPrintf(wxT("LIST(%i) @%i\n"), ID, pos);
    #endif
    wxUint32 OffsetStart = iStream->TellI();
    Collection.Read(iStream, iType);
    wxUint32 OffsetEnd = iStream->TellI();

    if(OffsetStart + Size != OffsetEnd)
    {
        #ifdef VERBOSE_OUT
        if(OffsetStart + Size < OffsetEnd)
        {
            wxPrintf(wxT("  ERROR: Read Overrun (List %i)\n"), ID);
        }
        else
        {
            wxPrintf(wxT("  ERROR: Read Underrun (List %i)\n"), ID);
        }
        #endif
        iStream->SeekI(OffsetStart + Size, wxFromStart);
    }
}

void BDAT_LIST::Write(wxOutputStream* oStream, BDAT_TYPE oType)
{
    oStream->Write(&Unknown1, 1);
    oStream->Write(&ID, 2);
    oStream->Write(&Unknown2, 2);
    oStream->Write(&Unknown3, 2);
    oStream->Write(&Size, 4);
    wxUint32 OffsetStart = oStream->TellO();
    Collection.Write(oStream, oType);
    wxUint32 OffsetEnd = oStream->TellO();
    oStream->SeekO(OffsetStart - 4, wxFromStart);
    Size = OffsetEnd - OffsetStart;
    oStream->Write(&Size, 4);
    oStream->SeekO(Size, wxFromCurrent);
}



BDAT_HEAD::BDAT_HEAD()
{
    Padding = NULL;
    Data = NULL;
}

BDAT_HEAD::~BDAT_HEAD()
{
    wxDELETEA(Padding);
    wxDELETEA(Data);
}

void BDAT_HEAD::Read(wxInputStream* iStream, BDAT_TYPE iType)
{
    iStream->Read(&Size_1, 4);
    iStream->Read(&Size_2, 4);
    iStream->Read(&Size_3, 4);
    Padding = new wxByte[62];
    iStream->Read(Padding, 62);
    Data = new wxByte[Size_1];
    if(!Complement) {
        iStream->Read(Data, Size_1);
    }
}

void BDAT_HEAD::Write(wxOutputStream* oStream, BDAT_TYPE oType)
{
    oStream->Write(&Size_1, 4);
    oStream->Write(&Size_2, 4);
    oStream->Write(&Size_3, 4);
    oStream->Write(Padding, 62);
    if(!Complement) {
        oStream->Write(Data, Size_1);
    }
}



BDAT_CONTENT::BDAT_CONTENT()
{
    Signature = NULL;
    Unknown = NULL;
    Lists = NULL;
}

BDAT_CONTENT::~BDAT_CONTENT()
{
    wxDELETEA(Signature);
    wxDELETEA(Unknown);
    wxDELETEA(Lists);
}

void BDAT_CONTENT::Read(wxInputStream* iStream, BDAT_TYPE iType)
{
    Signature = new wxByte[8];
    iStream->Read(Signature, 8);
    iStream->Read(&Version, 4);
    Unknown = new wxByte[9];
    iStream->Read(Unknown, 9);
    iStream->Read(&ListCount, 4);
    HeadList.Complement = false;
    if(ListCount < 20) {
        HeadList.Complement = true;
    }
    HeadList.Read(iStream, iType);
    Lists = new BDAT_LIST[ListCount];

    for(wxUint32 l=0; l<ListCount; l++)
    {
        Lists[l].Read(iStream, iType);
    }
}

void BDAT_CONTENT::Write(wxOutputStream* oStream, BDAT_TYPE oType)
{
    oStream->Write(Signature, 8);
    oStream->Write(&Version, 4);
    oStream->Write(Unknown, 9);
    oStream->Write(&ListCount, 4);
/*
    HeadList.Complement = false;
    if(ListCount < 20) {
        HeadList.Complement = true;
    }
*/
    HeadList.Write(oStream, oType);

    for(wxUint32 l=0; l<ListCount; l++)
    {
        Lists[l].Write(oStream, oType);;
    }
}



BDAT::BDAT()
{
    _indexFaqs = -1;
    _indexCommons = -1;
    _indexCommands = -1;
}

BDAT::~BDAT()
{
    //
}

// TODO: backup solution using the client.exe file -> "text" list -> ID
void BDAT::DetectIndices()
{
    wxUint32 fieldsize;

    #ifdef VERBOSE_OUT
    wxPrintf(wxT("\n\nSIGNIFICANT LIST DETECTION\n\n"));
    #endif

    for(wxUint32 l=0; l<_content.ListCount; l++)
    {
        BDAT_LIST* blist = &(_content.Lists[l]);

        // check if this is FAQS list by verifying particular conditions
        if(blist->Unknown1 == 2 && blist->Unknown2 == 0/* && blist->Unknown3 == 4*/)
        {
            fieldsize = 0;
            if(blist->Collection.Archive && blist->Collection.Archive->SubArchiveCount > 0 && blist->Collection.Archive->SubArchives[0].FieldLookupCount > 0)
            {
                fieldsize = blist->Collection.Archive->SubArchives[0].Fields[0].Size;
            }
            else if(blist->Collection.Loose && blist->Collection.Loose->FieldCount > 0)
            {
                fieldsize = blist->Collection.Loose->Fields[0].Size;
            }
            if(fieldsize == 32)
            {
                _indexFaqs = l;
                #ifdef VERBOSE_OUT
                wxPrintf(wxT("+++ FAQ +++\nIndex: %i\nID:%i\nSIZE: %i\nUNK1: %i\nUNK2: %i\nUNK3: %i\nCompr: %i\nFieldSize: %i\n\n"), l, blist->ID, blist->Size, blist->Unknown1, blist->Unknown2, blist->Unknown3, blist->Collection.Compressed, fieldsize);
                #endif
            }
        }

        // check if this is GENERAL list by verifying particular conditions
        if(blist->Size > 5000000 && blist->Unknown1 == 1 && blist->Unknown2 == 0/* && blist->Unknown3 == 6*/)
        {
            fieldsize = 0;
            if(blist->Collection.Archive && blist->Collection.Archive->SubArchiveCount > 0 && blist->Collection.Archive->SubArchives[0].FieldLookupCount > 0)
            {
                fieldsize = blist->Collection.Archive->SubArchives[0].Fields[0].Size;
            }
            else if(blist->Collection.Loose && blist->Collection.Loose->FieldCount > 0)
            {
                fieldsize = blist->Collection.Loose->Fields[0].Size;
            }
            if(fieldsize == 28)
            {
                _indexCommons = l;
                #ifdef VERBOSE_OUT
                wxPrintf(wxT("+++ GENERAL +++\nIndex: %i\nID:%i\nSIZE: %i\nUNK1: %i\nUNK2: %i\nUNK3: %i\nCompr: %i\nFieldSize: %i\n\n"), l, blist->ID, blist->Size, blist->Unknown1, blist->Unknown2, blist->Unknown3, blist->Collection.Compressed, fieldsize);
                #endif
            }
        }

        // check if this is COMMANDS list by verifying particular conditions
        if(_indexCommons > -1 && (wxUint32)_indexCommons < l && blist->Unknown1 == 1 && blist->Unknown2 == 0/* && blist->Unknown3 == 31*/)
        {
            fieldsize = 0;
            if(blist->Collection.Archive && blist->Collection.Archive->SubArchiveCount > 0 && blist->Collection.Archive->SubArchives[0].FieldLookupCount > 0)
            {
                fieldsize = blist->Collection.Archive->SubArchives[0].Fields[0].Size;
            }
            else if(blist->Collection.Loose && blist->Collection.Loose->FieldCount > 0)
            {
                fieldsize = blist->Collection.Loose->Fields[0].Size;
            }
            if(fieldsize == 28)
            {
                _indexCommands = l;
                #ifdef VERBOSE_OUT
                wxPrintf(wxT("+++ COMMANDS +++\nIndex: %i\nID:%i\nSIZE: %i\nUNK1: %i\nUNK2: %i\nUNK3: %i\nCompr: %i\nFieldSize: %i\n\n"), l, blist->ID, blist->Size, blist->Unknown1, blist->Unknown2, blist->Unknown3, blist->Collection.Compressed, fieldsize);
                #endif
            }
        }
    }

    #ifdef VERBOSE_OUT
    wxPrintf(wxT("List Index Faqs (ID): %i (%i)\n"), _indexFaqs, _content.Lists[_indexFaqs].ID);
    wxPrintf(wxT("List Index Commons (ID): %i (%i)\n"), _indexCommons, _content.Lists[_indexCommons].ID);
    wxPrintf(wxT("List Index Commands (ID): %i (%i)\n"), _indexCommands, _content.Lists[_indexCommands].ID);
    #endif
}

BDAT_TYPE BDAT::DetectType(wxInputStream* iStream)
{
    wxUint32 offset = iStream->TellI();
    iStream->SeekI(0);
    wxByte* Signature = new wxByte[8];
    iStream->Read(Signature, 8);
    iStream->SeekI(offset);

    BDAT_TYPE result = BDAT_UNKNOWN;

    //TODO: plain file not yet supported
    /*
    if(
        Signature[0] == '<' &&
        Signature[1] == '?' &&
        Signature[2] == 'x' &&
        Signature[3] == 'm' &&
        Signature[4] == 'l'
    )
    {
        result = BDAT_PLAIN;
    }
    */

    if(
        Signature[7] == 'B' &&
        Signature[6] == 'L' &&
        Signature[5] == 'S' &&
        Signature[4] == 'O' &&
        Signature[3] == 'B' &&
        Signature[2] == 'D' &&
        Signature[1] == 'A' &&
        Signature[0] == 'T'
    )
    {
        result = BDAT_BINARY;
    }

    wxDELETEA(Signature);

    return result;
}

void BDAT::Convert(wxInputStream* iStream, BDAT_TYPE iType, wxOutputStream* oStream, BDAT_TYPE oType)
{
    BDAT bns_dat;
    bns_dat.Load(iStream, iType);
    bns_dat.Save(oStream, oType);
}

void BDAT::Dump(wxInputStream* iStream, BDAT_TYPE iType, wxFileName oDirectory, BDAT_TYPE oType)
{
    BDAT bns_dat;
    bns_dat.Load(iStream, iType);
    oDirectory.Mkdir(0777, wxPATH_MKDIR_FULL);
    if(oType & BDAT_PLAIN)
    {
        bns_dat.DumpFaq(oDirectory.GetPath() + wxFileName::GetPathSeparator() + wxT("lookup_faq.txt"));
        bns_dat.DumpGeneral(oDirectory.GetPath() + wxFileName::GetPathSeparator() + wxT("lookup_general.txt"));
        bns_dat.DumpCommand(oDirectory.GetPath() + wxFileName::GetPathSeparator() + wxT("lookup_command.txt"));
    }
    if(oType & BDAT_XML)
    {
        bns_dat.DumpXML(oDirectory.GetPath());
    }
}

void BDAT::Load(wxInputStream* iStream, BDAT_TYPE iType)
{
    if(iType == BDAT_PLAIN)
    {
        //wxPrintf(wxT("BDAT_PLAIN import not yet supported!\n"));
        // FIXME: currently load as binary
        _content.Read(iStream, BDAT_BINARY);
        DetectIndices();
    }

    if(iType == BDAT_BINARY)
    {
        _content.Read(iStream, BDAT_BINARY);
        DetectIndices();
    }
}

void BDAT::Save(wxOutputStream* oStream, BDAT_TYPE oType)
{
    if(oType == BDAT_PLAIN)
    {
        //wxPrintf(wxT("BDAT_PLAIN export not yet supported!\n"));
        //FIXME: currently save as binary
        _content.Write(oStream, BDAT_BINARY);
    }

    if(oType == BDAT_BINARY)
    {
        _content.Write(oStream, BDAT_BINARY);
    }
}

void BDAT::DumpXML(wxString directory)
{
    wxPrintf(wxT("WARNING: This will take a long time and consume lot of RAM (abort: STRG+C)\n"));
    for(wxUint32 l=0; l<_content.ListCount; l++)
    {
        wxPrintf(wxT("\rDumping XML(%i/%i)..."), l+1, _content.ListCount);
        BDAT_LIST* blist = &(_content.Lists[l]);
        //if(blist->ID == 120 || blist->ID == 183 || blist->ID == 187)
        {
            // TODO: use wxFFile instead of XML because it is faster to write large files that way
            wxMBConvUTF8 enc;
            wxFFile doc(directory + wxFileName::GetPathSeparator() + wxString::Format(wxT("datafile_%03i.xml"), blist->ID), wxT("w"));
            doc.Write(wxT("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"), enc);
            doc.Write(wxString::Format(wxT("<list id=\"%i\" size=\"%i\" unk1=\"%i\" unk2=\"%i\" unk3=\"%i\">\n"), blist->ID, blist->Size, blist->Unknown1, blist->Unknown2, blist->Unknown3), enc);

            BDAT_COLLECTION* bcollection = &(blist->Collection);
            doc.Write(wxString::Format(wxT("\t<collection compressed=\"%i\">\n"), bcollection->Compressed), enc);
            if(bcollection->Compressed)
            {
                BDAT_ARCHIVE* barchive = bcollection->Archive;
                doc.Write(wxString::Format(wxT("\t\t<archive count=\"%i\">\n"), barchive->SubArchiveCount), enc);
                // TODO: add fields and lookups from sub-archives
                for(wxUint32 a=0; a<barchive->SubArchiveCount; a++)
                {
                    BDAT_SUBARCHIVE* bsubarchive = &(barchive->SubArchives[a]);
                    doc.Write(wxString::Format(wxT("\t\t\t<subarchive count=\"%i\">\n"), bsubarchive->FieldLookupCount), enc);
                    for(wxUint32 f=0; f<bsubarchive->FieldLookupCount; f++)
                    {
                        BDAT_FIELDTABLE* bfield = &(bsubarchive->Fields[f]);
                        doc.Write(wxString::Format(wxT("\t\t\t\t<field size=\"%i\" unk1=\"%i\" unk2=\"%i\">"), bfield->Size, bfield->Unknown1, bfield->Unknown2), enc);
                        doc.Write(BCRYPT::BytesToHex(bfield->Data, bfield->Size-8), enc);
                        doc.Write(wxT("</field>\n"), enc);

                        BDAT_LOOKUPTABLE* blookup = &(bsubarchive->Lookups[f]);
                        wxArrayString words = LookupSplitToWords(blookup->Data, blookup->Size);
                        doc.Write(wxString::Format(wxT("\t\t\t\t<lookup count=\"%i\">\n"), words.GetCount()), enc);
                        wxUint32 empty = 0;
                        for(wxUint32 w=0; w<words.GetCount(); w++)
                        {
                            // only add non-empty words
                            if(!words[w].IsEmpty())
                            {
                                doc.Write(wxT("\t\t\t\t\t<word>"), enc);
                                doc.Write(words[w], enc);
                                doc.Write(wxT("</word>\n"), enc);
                            }
                            else
                            {
                                empty++;
                            }
                        }
                        doc.Write(wxString::Format(wxT("\t\t\t\t\t<empty count=\"%i\"/>\n"), empty), enc);
                        doc.Write(wxT("\t\t\t\t</lookup>\n"), enc);
                    }
                    doc.Write(wxT("\t\t\t</subarchive>\n"), enc);;
                }
                doc.Write(wxT("\t\t</archive>\n"), enc);
            }
            else
            {
                BDAT_LOOSE* bloose = bcollection->Loose;
                doc.Write(wxString::Format(wxT("\t\t<loose countFields=\"%i\" sizeFields=\"%i\" sizePadding=\"%i\" sizeLookup=\"%i\" unk=\"%i\">\n"), bloose->FieldCount, bloose->SizeFields, bloose->SizePadding, bloose->SizeLookup, bloose->Unknown), enc);

                for(wxUint32 f=0; f<bloose->FieldCount; f++)
                {
                    BDAT_FIELDTABLE* bfield = &(bloose->Fields[f]);
                    doc.Write(wxString::Format(wxT("\t\t\t<field size=\"%i\" unk1=\"%i\" unk2=\"%i\">"), bfield->Size, bfield->Unknown1, bfield->Unknown2), enc);
                    doc.Write(BCRYPT::BytesToHex(bfield->Data, bfield->Size-8), enc);
                    doc.Write(wxT("</field>\n"), enc);
                }

                doc.Write(wxT("\t\t\t<padding>"), enc);
                doc.Write(BCRYPT::BytesToHex(bloose->Padding, bloose->SizePadding), enc);
                doc.Write(wxT("</padding>\n"), enc);

                wxArrayString words = LookupSplitToWords(bloose->Lookup.Data, bloose->Lookup.Size);
                doc.Write(wxString::Format(wxT("\t\t\t<lookup count=\"%i\">\n"), words.GetCount()), enc);
                wxUint32 empty = 0;
                for(wxUint32 w=0; w<words.GetCount(); w++)
                {
                    // only add non-empty words
                    if(!words[w].IsEmpty())
                    {
                        doc.Write(wxT("\t\t\t\t<word>"), enc);
                        doc.Write(words[w], enc);
                        doc.Write(wxT("</word>\n"), enc);
                    }
                    else
                    {
                        empty++;
                    }
                }
                doc.Write(wxString::Format(wxT("\t\t\t\t<empty count=\"%i\"/>\n"), empty), enc);
                doc.Write(wxT("\t\t\t</lookup>\n"), enc);

                doc.Write(wxT("\t\t</loose>\n"), enc);
            }
            doc.Write(wxT("\t</collection>\n"), enc);
            doc.Write(wxT("</list>\n"), enc);
            doc.Close();
        }
    }
    wxPrintf(wxT("\n"));
}

void BDAT::DumpFaq(wxString file)
{
    if(_indexFaqs > -1)
    {
        wxPrintf(wxT("Dumping FAQ...\n"));

        BDAT_LIST* blist = &(_content.Lists[_indexFaqs]);

        // TODO: change to wxFFile because of performance

        wxMBConvUTF8 enc;
        wxFFile doc(file, wxT("w"));

// TODO: check if loose or archive...
        BDAT_LOOSE* bloose = blist->Collection.Loose;
        wxArrayString words = LookupSplitToWords(bloose->Lookup.Data, bloose->SizeLookup);
        for(wxUint32 w=0; w<words.GetCount(); w++)
        {
            // two words for each field (save both)
            doc.Write(wxT("<alias>\n"), enc);
            doc.Write(wxT("</alias>\n"), enc);
            doc.Write(wxT("<text>\n"), enc);
            doc.Write(words[w] + wxT("\n"), enc);
            doc.Write(wxT("</text>\n"), enc);
        }
        doc.Close();
    }
}

void BDAT::DumpGeneral(wxString file)
{
    if(_indexCommons > -1)
    {
        wxPrintf(wxT("Dumping GENERAL...\n"));

        BDAT_LIST* blist = &(_content.Lists[_indexCommons]);

        wxMBConvUTF8 enc;
        wxFFile doc(file, wxT("w"));

        if(blist->Collection.Compressed)
        {
            BDAT_ARCHIVE* barchive = blist->Collection.Archive;
            for(wxUint32 s=0; s<barchive->SubArchiveCount; s++)
            {
                BDAT_SUBARCHIVE* bsubarchive = &(barchive->SubArchives[s]);
                for(wxUint32 f=0; f<bsubarchive->FieldLookupCount; f++)
                {
                    //BDAT_FIELDTABLE* bfield = &(bsubarchive->Fields[f]);
                    BDAT_LOOKUPTABLE* blookup = &(bsubarchive->Lookups[f]);
                    wxArrayString words = LookupSplitToWords(blookup->Data, blookup->Size);
                    for(wxUint32 w=0; w<words.GetCount(); w++)
                    {
                        // two words for each field
                        if(w%2 == 0)
                        {
                            doc.Write(wxT("<alias>\n"), enc);
                            doc.Write(words[w] + wxT("\n"), enc);
                            doc.Write(wxT("</alias>\n"), enc);
                        }
                        else
                        {
                            doc.Write(wxT("<text>\n"), enc);
                            doc.Write(words[w] + wxT("\n"), enc);
                            doc.Write(wxT("</text>\n"), enc);
                        }
                    }
                }
            }
        }
        else
        {
            BDAT_LOOSE* bloose = blist->Collection.Loose;
            wxArrayString words = LookupSplitToWords(bloose->Lookup.Data, bloose->SizeLookup);
            for(wxUint32 w=0; w<words.GetCount(); w++)
            {
                // two words for each field
                if(w%2 == 0)
                {
                    doc.Write(wxT("<alias>\n"), enc);
                    doc.Write(words[w] + wxT("\n"), enc);
                    doc.Write(wxT("</alias>\n"), enc);
                }
                else
                {
                    doc.Write(wxT("<text>\n"), enc);
                    doc.Write(words[w] + wxT("\n"), enc);
                    doc.Write(wxT("</text>\n"), enc);
                }
            }
        }

        doc.Close();
    }
}

void BDAT::DumpCommand(wxString file)
{
    if(_indexCommands > -1)
    {
        wxPrintf(wxT("Dumping COMMAND...\n"));

        BDAT_LIST* blist = &(_content.Lists[_indexCommands]);

        wxMBConvUTF8 enc;
        wxFFile doc(file, wxT("w"));

// TODO: check if loose or archive...
        BDAT_LOOSE* bloose = blist->Collection.Loose;
        wxArrayString words = LookupSplitToWords(bloose->Lookup.Data, bloose->SizeLookup);
        for(wxUint32 w=0; w<words.GetCount(); w++)
        {
            // one word for each field
            doc.Write(wxT("<alias>\n"), enc);
            doc.Write(wxT("</alias>\n"), enc);
            doc.Write(wxT("<text>\n"), enc);
            doc.Write(words[w] + wxT("\n"), enc);
            doc.Write(wxT("</text>\n"), enc);
        }
        doc.Close();
    }
}

void BDAT::TranslateFaq(BI18N* translator)
{
        /*
        <loose countFields="62" sizeFields="1984" sizePadding="0" sizeLookup="16030" unk="1">
            <field size="32" unk1="1" unk2="65535">01-00-00-00-00-00-00-00-00-00-00-00-8e-00-00-00-1a-00-00-00-01-00-00-00</field>
            <field size="32" unk1="1" unk2="65535">02-00-00-00-00-00-00-00-a8-00-00-00-96-00-00-00-c6-00-00-00-01-00-00-00</field>
            <field size="32" unk1="1" unk2="65535">03-00-00-00-00-00-00-00-5c-01-00-00-24-00-00-00-7a-01-00-00-01-00-00-00</field>
        <lookup count="124">
            <word>【系统】 如何创建门派？</word>
            <word>
                达到20级之后，去找【门派管理员】，花费20银来创建。<br/><br/>
                但在此之前，需要先加入武林盟或浑天教，并且没有加入其他门派。
            </word>
            <word>【系统】 如何提升门派声望？</word>
            <word>
                门派声望是兑换【门派声望牌】获得。<br/><br/>
                门派声望牌是获得灵气是一同获得，将所获得的门派声望牌拿给【门派管理员】即可提升门派声望。
            </word>
            <word>【系统】 如何修改人脉昵称？</word>
            <word>
                暂不支持~<br/><br/>
            </word>
        */
}

void BDAT::TranslateGeneral(BI18N* translator)
{
    // translate words in list general
    if(_indexCommons > -1)
    {
        BDAT_LIST* blist = &(_content.Lists[_indexCommons]);

        wxMBConvUTF16 enc;
        BDAT_FIELDTABLE* bfield;
        BDAT_LOOKUPTABLE* blookup;
        wxArrayString words;
        wxString translated;

        if(blist->Collection.Compressed)
        {
            BDAT_ARCHIVE* barchive = blist->Collection.Archive;
            BDAT_SUBARCHIVE* bsubarchive;
            wxUint32 alias_length;
            wxUint32 text_length;
            for(wxUint32 s=0; s<barchive->SubArchiveCount; s++)
            {
                bsubarchive = &(barchive->SubArchives[s]);
                for(wxUint32 f=0; f<bsubarchive->FieldLookupCount; f++)
                {
                    /*
                    <field size="28" unk1="1" unk2="65535">02-00-00-00-00-00-00-00-00-00-00-00-2c-00-00-00-4c-00-00-00</field>
                    <lookup count="2">
                        <word>Achieve.Name_1_Expand_Inventory_step1</word>
                        <word>Something doesn't fit</word>
                        <empty count="0"/>
                    </lookup>
                    */
                    bfield = &(bsubarchive->Fields[f]);
                    blookup = &(bsubarchive->Lookups[f]);
                    words = LookupSplitToWords(blookup->Data, blookup->Size);
                    wxDELETEA(blookup->Data);
                    blookup->Size = 0;
                    // alias
                    memcpy(bfield->Data + 8, &(blookup->Size), 4);
                    alias_length = 2*words[0].Len()+2;
                    blookup->Size += alias_length;
                    // text
                    memcpy(bfield->Data + 16, &(blookup->Size), 4);
                    translated = translator->Translate(words[1], words[0]);
                    if(!translated.IsEmpty())
                    {
                        words[1] = translated;
                    }
                    text_length = 2*words[1].Len()+2;
                    blookup->Size += text_length;
                    memcpy(bfield->Data + 12, &text_length, 4);
                    // write alias + text to lookup
                    blookup->Data = new wxByte[blookup->Size];
                    memset(blookup->Data, 0, blookup->Size);
                    memcpy(blookup->Data, words[0].mb_str(enc), alias_length-2);
                    memcpy(blookup->Data + alias_length, words[1].mb_str(enc), text_length-2);
                }
            }
            // TODO: update parent size infos...
        }
        else
        {
            BDAT_LOOSE* bloose = blist->Collection.Loose;
            blookup = &(bloose->Lookup);
            words = LookupSplitToWords(bloose->Lookup.Data, bloose->SizeLookup);
            wxArrayInt lengths;
            lengths.Alloc(words.GetCount());
            wxDELETEA(blookup->Data);
            bloose->SizeLookup = 0;
            for(wxUint32 w=0; w<words.GetCount(); w+=2)
            {
                bfield = &(bloose->Fields[w/2]);
                // alias
                memcpy(bfield->Data + 8, &(bloose->SizeLookup), 4);
                lengths[w] = 2*words[w].Len()+2;
                bloose->SizeLookup += lengths[w];
                // text
                memcpy(bfield->Data + 16, &(bloose->SizeLookup), 4);
                translated = translator->Translate(words[w+1], words[w]);
                if(!translated.IsEmpty())
                {
                    words[w+1] = translated;
                }
                lengths[w+1] = 2*words[w+1].Len()+2;
                bloose->SizeLookup += lengths[w+1];
            }
            blookup->Data = new wxByte[bloose->SizeLookup];
            memset(blookup->Data, 0, bloose->SizeLookup);
            wxUint32 offset = 0;
            for(wxUint32 w=0; w<words.GetCount(); w+=2)
            {
                // write alias + text to lookup
                memcpy(blookup->Data + offset, words[w].mb_str(enc), lengths[w]-2);
                offset += lengths[w];
                memcpy(blookup->Data + offset, words[w+1].mb_str(enc), lengths[w+1]-2);
                offset += lengths[w+1];
            }
            // TODO: update parent size infos...
        }
    }
}

void BDAT::TranslateCommand(BI18N* translator)
{
    // translate words in list command
    if(_indexCommands > -1)
    {
        BDAT_LIST* blist = &(_content.Lists[_indexCommands]);

        wxMBConvUTF16 enc;
        BDAT_FIELDTABLE* bfield;
        BDAT_LOOKUPTABLE* blookup;
        wxArrayString words;
        wxString translated;

        if(blist->Collection.Compressed)
        {
            //
        }
        else
        {
            BDAT_LOOSE* bloose = blist->Collection.Loose;
            blookup = &(bloose->Lookup);
            words = LookupSplitToWords(bloose->Lookup.Data, bloose->SizeLookup);
            wxArrayInt lengths;
            lengths.Alloc(words.GetCount());
            wxDELETEA(blookup->Data);
            bloose->SizeLookup = 0;
            for(wxUint32 w=0; w<words.GetCount(); w++)
            {
                /*
                <loose countFields="515" sizeFields="17204" sizePadding="0" sizeLookup="4516" unk="1">
                    <field size="28" unk1="1" unk2="0">01-00-00-00-00-00-00-00-0e-00-00-00-00-00-00-00-0d-00-00-00</field>
                    <field size="28" unk1="1" unk2="0">02-00-00-00-00-00-00-00-0a-00-00-00-0e-00-00-00-0d-00-00-00</field>
                    <field size="28" unk1="1" unk2="0">03-00-00-00-00-00-00-00-06-00-00-00-18-00-00-00-0d-00-00-00</field>
                    ...
                    // fields have different size !!!
                <padding></padding>
                <lookup count="515">
                    <word>INVITE</word>
                    <word>队伍邀请</word>
                    <word>邀请</word>
                */
                bfield = &(bloose->Fields[w]);
                // text
                memcpy(bfield->Data + 12, &(bloose->SizeLookup), 4);
                translated = translator->Translate(words[w], wxEmptyString);
                if(!translated.IsEmpty())
                {
                    words[w] = translated;
                }
                lengths[w] = 2*words[w].Len()+2;
                bloose->SizeLookup += lengths[w];
                memcpy(bfield->Data + 8, &(lengths[w]), 4);
            }
            blookup->Data = new wxByte[bloose->SizeLookup];
            memset(blookup->Data, 0, bloose->SizeLookup);
            wxUint32 offset = 0;
            for(wxUint32 w=0; w<words.GetCount(); w++)
            {
                // write text to lookup
                memcpy(blookup->Data + offset, words[w].mb_str(enc), lengths[w]-2);
                offset += lengths[w];
            }
            // TODO: update parent size infos...
        }
    }
}
