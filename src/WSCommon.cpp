/**
 * Source file for the WSCommon
 */
#include "WSCommon.hpp"

namespace sgns
{
    using namespace boost::asio;
    WSDevice::WSDevice(
        std::string ws_host,
        std::string ws_path,
        std::string ws_port,
        bool parse, bool save) 
    {
        ws_host_ = ws_host;
        ws_path_ = ws_path;
        ws_port_ = ws_port;
        parse_ = parse;
        save_ = save;
    }

    void WSDevice::StartWSDownload(std::shared_ptr<boost::asio::io_context> ioc, CompletionCallback handle_read, StatusCallback status)
    {
        //Resolve Address
        boost::asio::ip::tcp::resolver resolver(*ioc);
        boost::asio::ip::tcp::resolver::results_type results;
        try {
            results = resolver.resolve(ws_host_, ws_port_);
        }
        catch (const boost::system::system_error& e) {
            std::cerr << "Error resolving address: " << e.what() << std::endl;
            status(CustomResult(sgns::AsyncError::outcome::failure("WSS Could not resolve address")));
        }
        catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            status(CustomResult(sgns::AsyncError::outcome::failure("WSS Could not resolve address")));
        }
        catch (...) {
            std::cerr << "Unknown error occurred during address resolution." << std::endl;
            status(CustomResult(sgns::AsyncError::outcome::failure("WSS Could not resolve address")));
        }

        //Create SSL Context, using context::tls to accept the highest version client/server can deal with
        auto ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tls);

        //Disclude certain older insecure options
        ctx->set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::no_sslv3);

        //Default trusted authority definitions
        ctx->set_default_verify_paths();

        //Consider setting verify callback to check whether domain name matches cert
        // ctx->set_verify_callback(...);

        //Create Socket
        auto ws = std::make_shared<boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>(*ioc, *ctx);
        //Connect to server
        status(CustomResult(sgns::AsyncError::outcome::success(Success{ "Starting WS Connection" })));
        boost::asio::async_connect(ws->next_layer().next_layer(), results.begin(), results.end(), [self = shared_from_this(), ioc, ws, handle_read, status](const boost::system::error_code& error, const auto&) {
            if (!error) {
                // Perform the SSL asynchronous handshake
                status(CustomResult(sgns::AsyncError::outcome::success(Success{ "WS SSL Handshake Started" })));
                ws->next_layer().async_handshake(boost::asio::ssl::stream_base::client, [self, ioc, ws, handle_read, status](const boost::system::error_code& handshakeError) {
                    if (!handshakeError) {
                        // Perform the WebSocket asynchronous handshake
                        self->StartWSGet(ioc, ws, handle_read, status);
                    }
                    else {
                        std::cerr << "SSL handshake error: " << handshakeError.message() << std::endl;
                        status(CustomResult(sgns::AsyncError::outcome::failure("WS Handshake Error")));
                        handle_read(ioc, std::shared_ptr<std::pair<std::vector<std::string>, std::vector<std::vector<char>>>>(), false, false);
                    }
                    });
            }
            else {
                std::cerr << "Connect error: " << error.message() << std::endl;
                status(CustomResult(sgns::AsyncError::outcome::failure("WS Connection Error")));
                handle_read(ioc, std::shared_ptr<std::pair<std::vector<std::string>, std::vector<std::vector<char>>>>(), false, false);
            }
            });
    }

    void WSDevice::StartWSGet(std::shared_ptr<boost::asio::io_context> ioc,
        std::shared_ptr<boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>> ws,
        CompletionCallback handle_read,
        StatusCallback status)
    {
        // Perform the WebSocket asynchronous handshake
        status(CustomResult(sgns::AsyncError::outcome::success(Success{ "Starting WS Handshake" })));
        ws->async_handshake(ws_host_, ws_path_, [self = shared_from_this(), ioc, ws, handle_read, status](const boost::system::error_code& handshakeError) {
            if (!handshakeError) {
                //Request the file
                status(CustomResult(sgns::AsyncError::outcome::success(Success{ "Getting WS File" })));
                std::string request = "GET_FILE";
                ws->async_write(boost::asio::buffer(request), [self, ioc, ws, handle_read, status](const boost::system::error_code& write_error, std::size_t bytes_transferred) {
                    if (!write_error) {
                        //Read until WSEOF
                        status(CustomResult(sgns::AsyncError::outcome::success(Success{ "Reading WS File" })));
                        auto buffer = std::make_shared<boost::asio::streambuf>();
                        boost::asio::async_read_until(*ws, *buffer, "WSEOF", [self, ioc, ws, handle_read, status, buffer](const boost::system::error_code& read_error, std::size_t bytes_transferred) {
                            if (!read_error)
                            {
                                //auto outbuf = std::make_shared<std::vector<char>>(boost::asio::buffers_begin(buffer->data()), boost::asio::buffers_end(buffer->data()) - 5);
                                //std::cout << "WSS Finish" << std::endl;
                                status(CustomResult(sgns::AsyncError::outcome::success(Success{ "Finished Reading WS File" })));
                                auto finaldata = std::make_shared<std::pair<std::vector<std::string>, std::vector<std::vector<char>>>>();
                                std::filesystem::path p(self->ws_path_);
                                finaldata->first.push_back(p.filename().string());
                                //finaldata->second.push_back(*outbuf);
                                size_t dataSize = buffer->size()-5;
                                finaldata->second.emplace_back(
                                    boost::asio::buffers_begin(buffer->data()),
                                    boost::asio::buffers_begin(buffer->data()) + dataSize
                                );
                                handle_read(ioc, finaldata, self->parse_, self->save_);
                            }
                            else {
                                std::cerr << "File request read error: " << read_error.message() << std::endl;
                                status(CustomResult(sgns::AsyncError::outcome::failure("WS Read Failed. No EOF")));
                                handle_read(ioc, std::shared_ptr<std::pair<std::vector<std::string>, std::vector<std::vector<char>>>>(), false, false);
                            }
                            });
                    }
                    else {
                        std::cerr << "File request write error: " << write_error.message() << std::endl;
                        status(CustomResult(sgns::AsyncError::outcome::failure("WS Read Failed. Request file failed.")));
                    }
                    });
            }
            else {
                std::cerr << "WebSocket handshake error: " << handshakeError.message() << std::endl;
                status(CustomResult(sgns::AsyncError::outcome::failure("WS Handshake Error")));
                handle_read(ioc, std::shared_ptr<std::pair<std::vector<std::string>, std::vector<std::vector<char>>>>(), false, false);
            }
            });
    }
}