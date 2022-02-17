// based on https://github.com/mmore500/emscripten-targz/blob/master/main.cc
#pragma once
#include "inflate.h"
#include "untar.h"



void untar_file(const std::string & zip_filename, const std::string & dir_name)
{
    auto file = gzopen(zip_filename.c_str(), "rb");
    auto temp = std::tmpfile();

    // unzip into temporary file
    inflate(file, temp);

    gzclose(file);
    std::rewind(temp);

    std::filesystem::create_directory(dir_name);
    std::filesystem::current_path(dir_name); //setting path

    // untar into present working directory
    untar(temp, "temp");

    // deletes temporary file
    std::fclose(temp);

    // // remove the original .tar.gz archive... we don't need it anymore!
    // std::experimental::filesystem::remove(source_filename);

    // // print results
    // std::cout << "time to print results!" << std::endl;

    // for (const auto & filename : std::set{ "example/example_file.txt","example/example_directory/another_file.txt"}) {

    //     std::cout << "filename: " << filename << std::endl;

    //     std::cout << "  size: " << std::experimental::filesystem::file_size(filename) << std::endl;

    //     std::cout << "  content: " << std::endl;

    //     std::ifstream file{filename};

    //     std::string line;
    // while(getline(file, line)) std::cout << "    " << line << std::endl;

    // }

    // std::cout << "all done" << std::endl;

}