/*
    Copyright 2009, Felspar Co Ltd. http://fost.3.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/

#include "fost-inet-test.hpp"
#include <fost/detail/pop3.hpp>


using namespace fostlib::pop3;
using namespace fostlib;


namespace {

    const setting< string > c_pop3_test_account(
        "fost-inet/Cpp/fost-inet-test/pop3.cpp",
        "POP3 client test",
        "Username",
        "pop3test@felspar.net",
        true
    );

}


FSL_TEST_SUITE( pop3 );

FSL_TEST_FUNCTION( download_messages ) {
    host host(L"imap.felspar.net");

    iterate_mailbox(
        host,
        email_is_an_ndr,
        c_pop3_test_account.value(),
        setting<string>::value(
            L"POP3 client test",
            L"Password"
        )
    );

    /*
    // send an email to an invalid address for the next test run to remove
    using namespace appservices;
    appservices::service credentials(
        setting<fostlib::string>::value(
            L"AppserviceSDK tests",
            L"Api key"
        ),
        setting<fostlib::string>::value(
            L"AppserviceSDK tests",
            L"Api secret"
        )
    );

    appservices::send_email(
        email_address(
            coerce<ascii_string>(
                setting<fostlib::string>::value(
                    L"POP3 client test",
                    L"Valid Google mail address"
                )
            ),
            string("test")
        ),
        email_address(
            coerce<ascii_string>(
                string("cocoade@googlemail.com")
            ),
            string("test")
        ),
        "This message should not arrive here",

        "Please remove this inbox or change the test in "
        "fost-internet/Cpp/fost-inet-test/pop3.cpp",
        credentials
    );*/

    smtp_client server( host );


    text_body mail( L"This message shows that messages can be sent from appservices.felspar.com" );
    mail.headers().set(L"Subject", L"Test email -- send directly via SMTP");
    server.send(mail, "pop3test@felspar.com", "appservices@felspar.com");


    text_body should_be_bounced( L"This should be a bounced message. It shows that bounce messages are being received." );
    mail.headers().set(L"Subject", L"Test email -- sent to invalid address");
    server.send(should_be_bounced, "not-valid@felspar.com", "pop3test@felspar.com");
}