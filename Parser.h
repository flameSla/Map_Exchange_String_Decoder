#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <iostream>
#include <iomanip>
#include "nlohmann/json.hpp"

using namespace std;
using json = nlohmann::json;

///////////////////////////////////////////////
class Position
{
    public:
        int32_t x;
        int32_t y;
};
///////////////////////////////////////////////
class Parser
{
    public:
        Parser( string &MapString );
        virtual ~Parser();
        json map_gen_settings;
        json map_settings;

        friend ostream& operator<<(ostream& os, const Parser &p);

    private:
        json root;
        Position last_position;

        string MapString;
        size_t pos;

        void read_map_gen_settings();
        void read_map_settings();

        template <class T> T read();
        template <class T> T read_optional();
        string read_string();
        uint32_t read_uint32so();
        bool read_bool();
        json read_map_position();
};
#endif // PARSER_H
