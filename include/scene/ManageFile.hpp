#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm> 
#include <map>
#include <array>
#include <sstream>

class ManageFile
{
    private:
        int argc;
        char **argv;

    bool        verifyElem( std::vector<std::string> value );
    bool        valueRbgIsRight(float r, float g, float b);
    bool        rgbAllValueIsRight(std::string key, std::vector<float> vec);
    bool        valueIsShininess(float v);
    std::string strip(const std::string& str);

    public:
        void checkErrorFile( void );

    ManageFile( int argc, char **argv );
    ManageFile (const ManageFile &cp);
    ManageFile & operator= (const ManageFile &aff);
    ~ManageFile( void );
    
};