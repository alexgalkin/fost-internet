/*
    Copyright 2015, Felspar Co Ltd. http://support.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include "fost-inet.hpp"
#include <fost/server.hpp>

#include <condition_variable>
#include <mutex>
#include <thread>


using namespace fostlib;
namespace asio = boost::asio;


struct network_connection::server::state {
    boost::asio::io_service io_service;
    /// Thread to run all of the IO tasks in
    std::thread io_worker;
    /// Stop the thread from terminating (until we want it to)
    std::unique_ptr<asio::io_service::work> work;

    /// Thread for dispatching work
    std::thread dispatcher;

    boost::asio::ip::tcp::acceptor listener;

    /*
     * Socket handling is awkward. It's lifetime must at least match the accept handler
     * This code assumes there is only a single accept handler that is waiting at any time
     * and therefore the socket at this level is available as a sort of global.
     * With C++14 we'll be able to capture the socket using std::move in the closure, but
     * C++11 makes that awkward.
     */
    // TODO: Change to std::move captured in the closure in C++14
    std::unique_ptr<asio::ip::tcp::socket> socket;

    state(const host &h, uint16_t p, std::function<void(network_connection)> fn)
    : listener(io_service),
            socket(new asio::ip::tcp::socket(io_service)) {
        // Report aborts
        asio::ip::tcp::endpoint endpoint(h.address(), p);
        listener.open(endpoint.protocol());
        listener.set_option(asio::socket_base::enable_connection_aborted(true));
        listener.bind(endpoint);
        listener.listen();

        // Spin up the threads that are going to handle processing
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        std::condition_variable signal;
        io_worker = std::move(std::thread([this, &mutex, &signal]() {
            std::unique_lock<std::mutex> lock(mutex);
            work.reset(new asio::io_service::work(io_service));
            lock.unlock();
            signal.notify_one();
            std::cout << "Signalled io_service is running" << std::endl;
            bool again = false;
            do {
                again = false;
                try {
                    io_service.run();
                } catch ( std::exception &e ) {
                    again = true;
                    std::cout << "Caught " << e.what() << std::endl;
                } catch ( ... ) {
                    again = true;
                    std::cout << "Unknown exception caught" << std::endl;
                }
            } while ( again );
        }));
        signal.wait(lock);
        dispatcher = std::move(std::thread([this, fn, &mutex, &signal]() {
            std::unique_lock<std::mutex> lock(mutex);
            auto handler = [this, fn](const boost::system::error_code& error) {
                std::cout << "Got a connect " << error << std::endl;
                if ( !error ) {
                    fn(network_connection(io_service, std::move(socket)));
                }
                // Re-async_accept here
            };
            listener.async_accept(*socket, handler);
            lock.unlock();
            signal.notify_one();
            std::cout << "Signalled first async_accept handler registered" << std::endl;
        }));
        signal.wait(lock);
        std::cout << "Start up of server complete" << std::endl;
    }

    ~state() {
        std::cout << "Server tear down requested" << std::endl;
        work.reset();
        io_service.stop();
        io_worker.join();
        dispatcher.join();
    }
};


network_connection::server::server(const host &h, uint16_t p,
        std::function<void(network_connection)> fn)
: pimpl(new state(h, p, fn)) {
}


network_connection::server::~server() {
}

