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

    state(const host &h, uint16_t p, std::function<void(network_connection)> fn)
    : listener(io_service, asio::ip::tcp::endpoint(h.address(), p)) {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        std::condition_variable signal;
        io_worker = std::move(std::thread([this, &mutex, &signal]() {
            std::unique_lock<std::mutex> lock(mutex);
            work.reset(new asio::io_service::work(io_service));
            lock.unlock();
            signal.notify_one();
            io_service.run();
        }));
        signal.wait(lock);
        dispatcher = std::move(std::thread([this, fn, &mutex, &signal]() {
            std::unique_lock<std::mutex> lock(mutex);
            std::unique_ptr<asio::ip::tcp::socket> socket(
                new asio::ip::tcp::socket(io_service));
            auto handler = [this, fn, &socket](const boost::system::error_code& error) {
                if ( !error ) {
                    fn(network_connection(io_service, std::move(socket)));
                }
                // Re-async_accept here
            };
            listener.async_accept(*socket, handler);
            lock.unlock();
            signal.notify_one();
        }));
        signal.wait(lock);
    }

    ~state() {
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

