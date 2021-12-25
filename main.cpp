#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <zlib.h>
#include <cassert>
#include <stdio.h>
#include "base64.h"
#include "Parser.h"

#define assertm(exp, msg) assert(((void)msg, exp))

using namespace std;

////////////////////////////////////////////////////////////////////////
/// from http://windrealm.org/tutorials/decompress-gzip-stream.php
////////////////////////////////////////////////////////////////////////
bool gzipInflate(const std::string& compressedBytes, std::string& uncompressedBytes) {
	if (compressedBytes.size() == 0) {
		uncompressedBytes = compressedBytes;
		return true;
	}

	uncompressedBytes.clear();

	unsigned full_length = compressedBytes.size();
	unsigned half_length = compressedBytes.size() / 2;

	unsigned uncompLength = full_length;
	char* uncomp = (char*)calloc(sizeof(char), uncompLength);

	z_stream strm;
	strm.next_in = (Bytef *)compressedBytes.c_str();
	strm.avail_in = compressedBytes.size();
	strm.total_out = 0;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;

	bool done = false;

	if (inflateInit2(&strm, MAX_WBITS + 16) != Z_OK) {
		free(uncomp);
		return false;
	}

	while (!done) {
		// If our output buffer is too small
		if (strm.total_out >= uncompLength) {
			// Increase size of output buffer
			char* uncomp2 = (char*)calloc(sizeof(char), uncompLength + half_length);
			memcpy(uncomp2, uncomp, uncompLength);
			uncompLength += half_length;
			free(uncomp);
			uncomp = uncomp2;
		}

		strm.next_out = (Bytef *)(uncomp + strm.total_out);
		strm.avail_out = uncompLength - strm.total_out;

		// Inflate another chunk.
		int err = inflate(&strm, Z_SYNC_FLUSH);
		if (err == Z_STREAM_END) done = true;
		else if (err != Z_OK)  {
			break;
		}
	}

	if (inflateEnd(&strm) != Z_OK) {
		free(uncomp);
		return false;
	}

	for (size_t i = 0; i<strm.total_out; ++i) {
		uncompressedBytes += uncomp[i];
	}
	free(uncomp);
	return true;
}
////////////////////////////////////////////////////////////////////////
/// from https://panthema.net/2007/0328-ZLibString.html
////////////////////////////////////////////////////////////////////////
std::string decompress_string(const std::string& str)
{
	z_stream zs;                        // z_stream is zlib's control structure
	memset(&zs, 0, sizeof(zs));

	if (inflateInit(&zs) != Z_OK)
		throw(std::runtime_error("inflateInit failed while decompressing."));

	zs.next_in = (Bytef*)str.data();
	zs.avail_in = str.size();

	int ret;
	char outbuffer[32768];
	std::string outstring;

	// get the decompressed bytes blockwise using repeated calls to inflate
	do {
		zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
		zs.avail_out = sizeof(outbuffer);

		ret = inflate(&zs, 0);

		if (outstring.size() < zs.total_out) {
			outstring.append(outbuffer,
				zs.total_out - outstring.size());
		}

	} while (ret == Z_OK);

	inflateEnd(&zs);

	if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
		std::ostringstream oss;
		oss << "Exception during zlib decompression: (" << ret << ") "
			<< zs.msg;
		throw(std::runtime_error(oss.str()));
	}

	return outstring;
}
////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        fprintf(stderr,"\nFactorio Map Exchange String Decoder (c) flameSla\n");
        fprintf(stderr,"Special thx to: Hornwitser\n");
        fprintf(stderr,"\thttps://gist.github.com/Hornwitser/f291638024e7e3c0271b1f3a4723e05a#file-exchange_string_decoder-js\n");
        fprintf(stderr,"\nUsage: Map_Exchange_String_Decoder.exe input.txt MapGenSettings.json MapSettings.json\n");
        fprintf(stderr,"\nWhere:");
        fprintf(stderr,"\n  input.txt           - a file that contains \"Map Exchange String\"");
        fprintf(stderr,"\n  MapGenSettings.json - output file");
        fprintf(stderr,"\n  MapSettings.json    - output file");
        fprintf(stderr,"\n");
        return -1;
    }

    string input_filename = argv[1]; // Map Exchange String
    string output_filename_1 = argv[2]; // map-gen-settings
    string output_filename_2 = argv[3]; // map-settings

    remove( output_filename_1.c_str() );
    remove( output_filename_2.c_str() );

    streampos file_size;
    char *memblock;
    ifstream file ( input_filename, ios::in|ios::binary|ios::ate);
    if ( file.is_open() )
    {
        file_size = file.tellg();
        memblock = new char [file_size];
        file.seekg (0, ios::beg );
        file.read ( memblock, file_size );
        file.close();

        string base64String;
        for(size_t i=0, imax = file_size; i<imax; i++)
        {
            char c = memblock[i];
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || c == '+' || c == '/' || c == '=' || c == '>' || c == '<')
            {
                base64String += c;
            }
        }
        delete[] memblock;

        bool check = false;
        if( base64String[0]=='>' && base64String[1]=='>' && base64String[2]=='>' &&
            base64String[base64String.length()-3]=='<' && base64String[base64String.length()-2]=='<' && base64String[base64String.length()-1]=='<' )
            check = true;
        assertm( check == true, "Not a map exchange string" );
        base64String = base64String.substr (3, base64String.length()-6);

        string compressedString = base64_decode( base64String );
        string decompressed = decompress_string( compressedString );

        Parser p( decompressed );
        //cout << p << endl;

        ofstream out_file2(output_filename_1, ios::out|ios::binary|ios::trunc);
        if (out_file2.is_open())
        {
            out_file2 << setw(4) << p.map_gen_settings;
            out_file2.close();
        }
        else
        {
            cout << "Unable to open file - '" << output_filename_1 << "'";
            return 1;
        }

        ofstream out_file3(output_filename_2, ios::out|ios::binary|ios::trunc);
        if (out_file3.is_open())
        {
            if( !p.map_settings.is_null() )
                out_file3 << setw(4) << p.map_settings;
            out_file3.close();
        }
        else
        {
            cout << "Unable to open file - '" << output_filename_2 << "'";
            return 1;
        }
    }
    else
    {
        cout << "Unable to open file - '" << input_filename << "'";
        return 1;
    }

    return 0;
}
