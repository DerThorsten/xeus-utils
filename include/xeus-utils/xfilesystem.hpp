#include <string>
#include <iostream>
#include <filesystem>
#include "xeus-utils/xeus_stream.hpp"

namespace fs = std::filesystem;

namespace xeus_utils
{

    void ls(const std::string & path)
    {
        static const xinterpreter_cout xcout;

        for (const auto & entry : fs::directory_iterator(path))
        {
            xcout << entry.path() <<"\n";
        }
    }

}