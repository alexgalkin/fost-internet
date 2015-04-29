/*
    Copyright 2011-2015, Felspar Co Ltd. http://support.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include "fost-inet-test.hpp"
#include <fost/log>
#include <fost/server>

#include <condition_variable>
#include <mutex>


using namespace fostlib;


FSL_TEST_SUITE( data_transmission );


namespace {
    void embed_acks(network_connection cnx) {
        std::vector<unsigned char> data(0x8000);
        for ( std::size_t block(0); block < 8; ++block) {
            FSL_CHECK_NOTHROW(cnx >> data);
            FSL_CHECK_NOTHROW(cnx << "ack\r\n");
        }
    }
}

FSL_TEST_FUNCTION( large_send_embed_acks ) {
    network_connection::server server(host("0.0.0.0"), 6218, embed_acks);
    // Give enough time for thread to start
    boost::this_thread::sleep(boost::posix_time::milliseconds(250));

    network_connection cnx(host("localhost"), 6218);
    std::string data(0x8000, 'x');
    for ( std::size_t block(0); block < 8; ++block ) {
        fostlib::log::debug("Sending data block", block);
        FSL_CHECK_NOTHROW(cnx << data);
        std::string ack;
        FSL_CHECK_NOTHROW(cnx >> ack);
        FSL_CHECK_EQ(ack, "ack");
    }
}


namespace {
#ifdef FOST_OS_WINDOWS
    // Windows doesn't seem to have a big enough buffer for this to pass
    // unless the number of blocks is small. Effectively disable the test
    // under Windows
    const std::size_t c_blocks = 1;
#else
    const std::size_t c_blocks = 800;
#endif

    void ack_at_end(network_connection cnx) {
        std::vector<unsigned char> data(0x8000);
        for ( std::size_t block(0); block < c_blocks; ++block ) {
            try {
                FSL_CHECK_NOTHROW(cnx >> data);
                for ( std::size_t i(0); i != data.size(); ++i )
                    try {
                        FSL_CHECK_EQ(data[i], "0123456789"[block %10]);
                    } catch ( exceptions::exception &e ) {
                        insert(e.data(), "byte-number", i);
                        throw;
                    }
            } catch ( exceptions::exception &e ) {
                insert(e.data(), "block-number", block);
                throw;
            }
        }
        FSL_CHECK_NOTHROW(cnx << "ack\r\n");
    }
}

FSL_TEST_FUNCTION( large_send_ack_at_end ) {
    network_connection::server server(host("0.0.0.0"), 6217, ack_at_end);
    // Give enough time for thread to start
    boost::this_thread::sleep(boost::posix_time::milliseconds(250));

    network_connection cnx(host("localhost"), 6217);
    for ( std::size_t block(0); block < c_blocks; ++block ) {
        std::string data(0x8000, "0123456789"[block %10]);
        FSL_CHECK_NOTHROW(cnx << data);
    }
    std::string ack;
    FSL_CHECK_NOTHROW(cnx >> ack);
    FSL_CHECK_EQ(ack, "ack");
}
