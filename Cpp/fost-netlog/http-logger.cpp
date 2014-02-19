/*
    Copyright 2010-2012, Felspar Co Ltd. http://support.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include "fost-netlog.hpp"
#include <fost/log>
#include <fost/http>


#ifdef FOST_OS_WINDOWS
__declspec(dllexport) int force_lib_file = 0;
#endif


using namespace fostlib;


class http_logger {
    http::user_agent ua;
    public:
        http_logger();
        http_logger(const json &);

        bool operator () (const fostlib::log::message &);
};


namespace {


    const fostlib::log::global_sink< http_logger > http_put("http-put");


}


http_logger::http_logger() {
}
http_logger::http_logger(const json &) {
}


bool http_logger::operator () (const fostlib::log::message &m) {
    boost::shared_ptr<mime> body(new text_body(
        json::unparse(coerce<json>(m), true)));
    http::user_agent::request req("PUT", url("http://localhost:23456/"), body);
    ua(req);
    return true;
}