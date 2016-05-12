#include "main.h"

void print_help()
{
    printf("Usage: bnsdat [option] [compression]\n");
    printf("\n");
    printf("Option:\n");
    printf("\n");
    printf("  -e FILE         Extract xml.dat/config.dat FILE to raw binary files.\n");
    printf("                  Files will be extracted into a subdirectory\n");
    printf("                  of the same directory (xml.dat.files).\n");
    printf("  -x FILE         Extract xml.dat/config.dat FILE to human readable files.\n");
    printf("                  Files will be extracted into a subdirectory\n");
    printf("                  of the same directory (xml.dat.file).\n");
    printf("  -d FILE         Dump datafile.bin FILE to human readable files.\n");
    printf("                  Files will be extracted into a subdirectory\n");
    printf("                  of the same directory (datafile.bin.files).\n");
    printf("                  All xml files and some selected text files\n");
    printf("                  will be produced.\n");
    printf("  -dxml FILE      Dump datafile.bin FILE to human readable files.\n");
    printf("                  Files will be extracted into a subdirectory\n");
    printf("                  of the same directory (datafile.bin.files).\n");
    printf("                  All xml files will be produced.\n");
    printf("  -dtxt FILE      Dump datafile.bin FILE to human readable files.\n");
    printf("                  Files will be extracted into a subdirectory\n");
    printf("                  of the same directory (datafile.bin.files).\n");
    printf("                  Only some selected text files will be produced.\n");
    printf("  -s FILE         Switch (transform) a xml FILE between\n");
    printf("                  binary and plain format (bxml <-> xml).\n");
    printf("  -t DIRECTORY    Translate the content of DIRECTORY to english.\n");
    printf("                  A translation.xml dictionary file must be\n");
    printf("                  present in the application directory.\n");
    printf("                  Content:\n");
    printf("                  + datafile.bin\n");
    printf("                  + text/textdata.subtitle.x16\n");
    printf("                  + outsource/surveyquestions.x16\n");
    printf("  -c DIRECTORY    Compress content of DIRECTORY to xml.dat\n");
    printf("                  File will be created in the same directory\n");
    printf("\n");
    printf("Compression (Optional):\n");
    printf("\n");
    printf("  VALUE           Number VALUE indicating compression level\n");
    printf("                  Range [0..9] with 0: none, 9: maximum\n");
    printf("                  This is optional, default value: 6\n");
    printf("                  Only affects compression mode\n");
    printf("\n");
    printf("Further information can be found at:\n");
    printf("http://sourceforge.net/p/bns-tools\n");
}

void dump(wxString file, BDAT_TYPE format)
{
    wxPrintf(wxT("Operation Mode: Dump File\n\n") + file + wxT("\n\n"));

    wxFFileInputStream fis(file);

    if(file.EndsWith(wxT(".bin")))
    {
        BDAT_TYPE type = BDAT::DetectType(&fis);
        BDAT::Dump(&fis, type, wxFileName(file + wxT(".files") + wxFileName::GetPathSeparator()), format);
    }
    wxPrintf(wxT("\n"));
}

void transform(wxString file)
{
    wxPrintf(wxT("Operation Mode: Transform File\n\n") + file + wxT("\n\n"));

    wxString fileTemp = file + wxT(".tmp");

    if(file.EndsWith(wxT(".xml")) || file.EndsWith(wxT(".x16")))
    {
        wxFFileInputStream fis(file);
        BXML_TYPE type = BXML::DetectType(&fis);

        if(type == BXML_BINARY)
        {
            wxFFileOutputStream fos(fileTemp);
            BXML::Convert(&fis, BXML_BINARY, &fos, BXML_PLAIN);
        }
        if(type == BXML_PLAIN)
        {
            wxFFileOutputStream fos(fileTemp);
            BXML::Convert(&fis, BXML_PLAIN, &fos, BXML_BINARY);
        }
    }

    if(file.EndsWith(wxT(".bin")))
    {
        wxFFileInputStream fis(file);
        BDAT_TYPE type = BDAT::DetectType(&fis);

        if(type == BDAT_BINARY)
        {
            wxFFileOutputStream fos(fileTemp);
            BDAT::Convert(&fis, BDAT_BINARY, &fos, BDAT_PLAIN);
        }
        if(type == BDAT_PLAIN)
        {
            wxFFileOutputStream fos(fileTemp);
            BDAT::Convert(&fis, BDAT_PLAIN, &fos, BDAT_BINARY);
        }
    }

    wxRemoveFile(file);
    wxRenameFile(fileTemp, file);
}

void translate(wxString directory)
{
    wxPrintf(wxT("Operation Mode: Translate Files\n\n") + directory + wxT("\n\n"));
    wxString file;

    // load dictionary
    #ifdef __LINUX__
        file = wxStandardPaths::Get().GetExecutablePath().BeforeLast('/') + wxT("/translation.xml");
    #endif
    #ifdef __WINDOWS__
        file = wxStandardPaths::Get().GetExecutablePath().BeforeLast('\\') + wxT("\\translation.xml");
    #endif
    if(!wxFileExists(file))
    {
        wxPrintf(wxT("Missing Dictionary: \'translation.xml\'\n"));
        return;
    }
    BI18N translator;
    wxFFileInputStream tis(file);
    translator.Load(&tis, BI18N_XML);

    // translate surveys
    {
        file = directory + wxFileName::GetPathSeparator() + wxT("outsource") + wxFileName::GetPathSeparator()  + wxT("surveyquestions.x16");
        if(wxFileExists(file))
        {
            wxPrintf(wxT("Translating: Surveys...\n"));
            BXML xml;
            wxFFileInputStream fis(file);
            BXML_TYPE type = BXML::DetectType(&fis);
            xml.Load(&fis, type);
            xml.Translate(&translator);
            // FIXME: close input stream first...
            wxFFileOutputStream fos(file);
            xml.Save(&fos, type);
        }
    }

    // translate subtitles
    {
        file = directory + wxFileName::GetPathSeparator() + wxT("text") + wxFileName::GetPathSeparator()  + wxT("textdata.subtitle.x16");
        if(wxFileExists(file))
        {
            wxPrintf(wxT("Translating: Subtitles...\n"));
            BXML xml;
            wxFFileInputStream fis(file);
            BXML_TYPE type = BXML::DetectType(&fis);
            xml.Load(&fis, type);
            xml.Translate(&translator);
            // FIXME: close input stream first...
            wxFFileOutputStream fos(file);
            xml.Save(&fos, type);
        }
    }

    // translate datafile.bin
    {
        file = directory + wxFileName::GetPathSeparator() + wxT("datafile.bin");
        if(wxFileExists(file))
        {
            wxPrintf(wxT("Translating: Content...\n"));
            BDAT data;
            wxFFileInputStream fis(file);
            BDAT_TYPE type = BDAT::DetectType(&fis);
            data.Load(&fis, type);
            //data.TranslateFaq(&translator);
            data.TranslateGeneral(&translator);
            //data.TranslateCommand(&translator);
            // FIXME: close input stream first...
            wxFFileOutputStream fos(file);
            data.Save(&fos, type);
        }
    }
}

void extract(wxString file, bool convert)
{
    if(convert)
    {
        wxPrintf(wxT("Operation Mode: Extract Files (Plain)\n\n") + file + wxT("\n\n"));
    }
    else
    {
        wxPrintf(wxT("Operation Mode: Extract Files (RAW)\n\n") + file + wxT("\n\n"));
    }
    wxFFileInputStream fis(file);
    BPKG::Extract(&fis, wxFileName(file + wxT(".files") + wxFileName::GetPathSeparator()), convert);
    wxPrintf(wxT("\n"));
}

void compress(wxString directory, wxUint32 compression)
{
    wxPrintf(wxT("Operation Mode: Compress Files\n\n") + directory + wxT("\n\n"));
    wxFFileOutputStream fos(directory.BeforeLast('.'));
    BPKG::Compress(wxFileName(directory, wxT("")), &fos, compression);
    wxPrintf(wxT("\n"));
}

IMPLEMENT_APP(bnsApp);

bool bnsApp::OnInit()
{
    // check if amount of command line arguments is correct
    // first parameter is the execuable file itself
    // second is the instruction
    // third is the file/directory
    // fourth is optional compression level

    if(argc < 3)
    {
        print_help();
        exit(1);
    }

    // Convert first parameter argv[1] from char* to wxString
    wxString instruction(argv[1], wxConvUTF8);
    // Convert second parameter argv[2] from char* to wxString
    wxString path(argv[2], wxConvUTF8);
    // Convert third parameter argv[3] from char* to int
    wxUint32 compression_level = 6;
    if(argc > 3)
    {
        compression_level = wxAtoi(argv[3]);
        if(compression_level < 0)
        {
            compression_level = 0;
        }
        if(compression_level > 9)
        {
            compression_level = 9;
        }
    }

    if(instruction != wxT("-e") &&
       instruction != wxT("-x") &&
       instruction != wxT("-d") &&
       instruction != wxT("-dxml") &&
       instruction != wxT("-dtxt") &&
       instruction != wxT("-s") &&
       instruction != wxT("-t") &&
       instruction != wxT("-c"))
    {
        wxPrintf(wxT("First argument is not a valid instruction\n"));
        exit(2);
    }

    if(instruction == wxT("-e"))
    {
        // Check if path is a valid file
        if(!wxFileExists(path))
        {
            wxPrintf(wxT("Second argument is not a valid file\n"));
            exit(3);
        }
        else
        {
            extract(path, false);
        }
    }

    if(instruction == wxT("-x"))
    {
        // Check if path is a valid file
        if(!wxFileExists(path))
        {
            wxPrintf(wxT("Second argument is not a valid file\n"));
            exit(3);
        }
        else
        {
            extract(path, true);
        }
    }

    if(instruction == wxT("-d"))
    {
        // Check if path is a valid file
        if(!wxFileExists(path))
        {
            wxPrintf(wxT("Second argument is not a valid file\n"));
            exit(3);
        }
        else
        {
            dump(path, (BDAT_TYPE)(BDAT_XML|BDAT_PLAIN));
        }
    }

    if(instruction == wxT("-dxml"))
    {
        // Check if path is a valid file
        if(!wxFileExists(path))
        {
            wxPrintf(wxT("Second argument is not a valid file\n"));
            exit(3);
        }
        else
        {
            dump(path, BDAT_XML);
        }
    }

    if(instruction == wxT("-dtxt"))
    {
        // Check if path is a valid file
        if(!wxFileExists(path))
        {
            wxPrintf(wxT("Second argument is not a valid file\n"));
            exit(3);
        }
        else
        {
            dump(path, BDAT_PLAIN);
        }
    }

    if(instruction == wxT("-s"))
    {
        // Check if path is a valid file
        if(!wxFileExists(path))
        {
            wxPrintf(wxT("Second argument is not a valid file\n"));
            exit(3);
        }
        else
        {
            transform(path);
        }
    }

    if(instruction == wxT("-t"))
    {
        // Check if path is a valid directory
        if(!wxDirExists(path))
        {
            wxPrintf(wxT("Second argument is not a valid directory\n"));
            exit(4);
        }
        else
        {
            translate(path);
        }
    }

    if(instruction == wxT("-c"))
    {
        // Check if path is a valid directory
        if(!wxDirExists(path) || !path.EndsWith(wxT(".dat.files")))
        {
            wxPrintf(wxT("Second argument is not a valid directory\n"));
            exit(4);
        }
        else
        {
            compress(path, compression_level);
        }
    }

    exit(0);
    return false;
}
