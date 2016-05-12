#ifndef BCRYPT_H
#define BCRYPT_H

#include <wx/mstream.h>
#include <wx/zstream.h>
#include <openssl/aes.h>

class BCRYPT
{
    public: static void Xor(wxByte* buffer, wxUint32 size);
    public: static wxUint64 CheckSum(wxByte* buffer, wxUint32 size);

    public: static wxByte* Deflate(wxByte* buffer, wxUint32 sizeCompressed, wxUint32 sizeDecompressed);
    public: static wxByte* Inflate(wxByte* buffer, wxUint32 sizeDecompressed, wxUint32* sizeCompressed, wxUint32 compressionLevel = 1);

    public: static wxByte* Decrypt(wxByte* buffer, wxUint32 size, wxUint32* sizePadded);
    public: static wxByte* Encrypt(wxByte* buffer, wxUint32 size, wxUint32* sizePadded);

    public: static wxString BytesToHex(wxByte* buffer, wxUint32 size);

    private: static wxUint64 FNV_PRIME;
    private: static wxUint64 FNV_SEED;

    private: static int XOR_KEY_LENGTH;
    private: static unsigned char XOR_KEY[16];

    private: static int CRYPT_KEY_LENGTH;
    private: static unsigned char CRYPT_KEY[16];
};

#endif // BCRYPT_H
