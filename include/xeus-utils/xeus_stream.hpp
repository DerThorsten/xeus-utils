#ifndef XUTILS_XEUS_STREAM_HPP
#define XUTILS_XEUS_STREAM_HPP

#include <string>
#include <sstream>


namespace xeus_utils
{
    struct XEUS_UTILS_API xinterpreter_cout
    {

    };
    

    // todo use enable if

    template<class T>
    const xinterpreter_cout & operator <<(const xinterpreter_cout & out, T && value)
    {
        std::stringstream ss;
        ss<<value;
        xeus::get_interpreter().publish_stream("stdout", ss.str());
        return out;
    }
    template<class T>
    xinterpreter_cout & operator <<(xinterpreter_cout & out, T && value)
    {
        std::stringstream ss;
        ss<<value;
        xeus::get_interpreter().publish_stream("stdout", ss.str());
        return out;
    }
    template<class T>
    xinterpreter_cout & operator <<(xinterpreter_cout && out, T && value)
    {
        std::stringstream ss;
        ss<<value;
        xeus::get_interpreter().publish_stream("stdout", ss.str());
        return out;
    }


}


#endif