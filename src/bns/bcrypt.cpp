#include "bcrypt.h"

wxUint64 BCRYPT::FNV_PRIME = 0x01000193; //   16777619
wxUint64 BCRYPT::FNV_SEED  = 0x811C9DC5; // 2166136261

int BCRYPT::XOR_KEY_LENGTH = 16;
unsigned char BCRYPT::XOR_KEY[16] = {164, 159, 216, 179, 246, 142, 57, 194, 45, 224, 97, 117, 92, 75, 26, 7}; // A4 9F D8 B3 F6 8E 39 C2 2D E0 61 75 5C 4B 1A 07

int BCRYPT::CRYPT_KEY_LENGTH = 128; // = 16 byte
unsigned char BCRYPT::CRYPT_KEY[16] = {'b', 'n', 's', '_', 'o', 'b', 't', '_', 'k', 'r', '_', '2', '0', '1', '4', '#'};
//unsigned char BCRYPT::CRYPT_KEY[16] = {'b', 'n', 's', '_', 'f', 'g', 't', '_', 'c', 'b', '_', '2', '0', '1', '0', '!'};
//unsigned char BCRYPT::CRYPT_KEY[16] = {'*', 'P', 'l', 'a', 'y', 'B', 'N', 'S', '(', 'c', ')', '2', '0', '1', '4', '*'};

void BCRYPT::Xor(wxByte* buffer, wxUint32 size)
{
    for(wxUint32 i=0; i<size; i++)
    {
        buffer[i] = buffer[i] xor XOR_KEY[i % XOR_KEY_LENGTH];
    }
}

wxUint64 BCRYPT::CheckSum(wxByte* buffer, wxUint32 size)
{
    // FNV-1a algorithm
    wxUint64 checksum = FNV_SEED;

    for(wxUint32 i=0; i<size; i++)
    {
        checksum = (checksum ^ ((wxUint64)buffer[i])) * FNV_PRIME;
    }

    return checksum;
}

wxByte* BCRYPT::Deflate(wxByte* buffer, wxUint32 sizeCompressed, wxUint32 sizeDecompressed)
{
    wxByte* out = new wxByte[sizeDecompressed];
    wxMemoryInputStream mis(buffer, sizeCompressed);
    wxZlibInputStream* zis = new wxZlibInputStream(mis);
    zis->Read(out, sizeDecompressed);
    wxDELETE(zis);
    return out;
}

wxByte* BCRYPT::Inflate(wxByte* buffer, wxUint32 sizeDecompressed, wxUint32* sizeCompressed, wxUint32 compressionLevel)
{
    wxMemoryOutputStream mos;
    wxZlibOutputStream* zos = new wxZlibOutputStream(mos, compressionLevel);
    zos->Write(buffer, sizeDecompressed);
    wxDELETE(zos);
    *sizeCompressed = mos.GetSize();
    wxByte* out = new wxByte[*sizeCompressed];
    mos.CopyTo(out, *sizeCompressed);
    return out;
}

wxByte* BCRYPT::Decrypt(wxByte* buffer, wxUint32 size, wxUint32* sizePadded)
{
    // AES requires buffer to consist of blocks with 16 bytes (each)
    // expand last block by padding zeros if required...
    // -> the encrypted data in BnS seems already to be aligned to blocks
    *sizePadded = size + (AES_BLOCK_SIZE - (size % AES_BLOCK_SIZE)) % AES_BLOCK_SIZE;
    wxByte* out = new wxByte[*sizePadded];
    wxByte* tmp = buffer;

    if(*sizePadded > size)
    {
        tmp = (wxByte*)memcpy(new wxByte[*sizePadded], buffer, size);
        memset(tmp+size, 0, *sizePadded - size);
    }

    AES_KEY decryptingContext;
    AES_set_decrypt_key(CRYPT_KEY, CRYPT_KEY_LENGTH, &decryptingContext);
    for(wxUint32 i=0; i<*sizePadded; i+=16)
    {
        AES_decrypt(buffer+i, out+i, &decryptingContext);
    }

    if(tmp != buffer)
    {
        wxDELETEA(tmp);
    }

    return out;
}

wxByte* BCRYPT::Encrypt(wxByte* buffer, wxUint32 size, wxUint32* sizePadded)
{
    // AES requires buffer to consist of blocks with 16 bytes (each)
    // expand last block by padding zeros if required...
    // -> the encrypted data in BnS seems already to be aligned to blocks
    *sizePadded = size + (AES_BLOCK_SIZE - (size % AES_BLOCK_SIZE)) % AES_BLOCK_SIZE;
    wxByte* out = new wxByte[*sizePadded];
    wxByte* tmp = buffer;

    if(*sizePadded > size)
    {
        tmp = (wxByte*)memcpy(new wxByte[*sizePadded], buffer, size);
        memset(tmp+size, 0, *sizePadded - size);
    }

    AES_KEY encryptingContext;
    AES_set_encrypt_key(CRYPT_KEY, CRYPT_KEY_LENGTH, &encryptingContext);
    for(wxUint32 i=0; i<*sizePadded; i+=16)
    {
        AES_encrypt(tmp+i, out+i, &encryptingContext);
    }

    if(tmp != buffer)
    {
        wxDELETEA(tmp);
    }

    return out;
}

wxString BCRYPT::BytesToHex(wxByte* buffer, wxUint32 size)
{
    wxString appendix = wxEmptyString;

    if(size < 1)
    {
        return wxEmptyString;
    }
/*
    if(size > 1024)
    {
        size = 1024;
        appendix = wxT("...");
    }
*/
    wxByte* hex = new wxByte[3*size];
    hex[3*size-1] = '\0';
    wxString tmp;

    for(wxUint32 i=0; i<size; i++)
    {
        tmp = wxString::Format(wxT("%.2x"), buffer[i]);
        hex[3*i] = tmp[0];
        hex[3*i+1] = tmp[1];
        if(i < size-1)
        {
            hex[3*i+2] = '-';
        }
    }

    wxString out = wxString::FromAscii((char*)hex) + appendix;
    wxDELETEA(hex);
    return out;
}
