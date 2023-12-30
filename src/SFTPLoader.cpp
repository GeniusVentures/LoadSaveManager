#include <sstream>
#include <filesystem>
#include <fstream>
#include <streambuf>
#include <string>
#include <memory>
#include <iostream>
#include "FileManager.hpp"
#include "SFTPLoader.hpp"
#include "boost/asio.hpp"
#include "boost/asio/ssl.hpp"
#include "URLStringUtil.h"
#include "libssh2.h"
#include "libssh2_sftp.h"
#include <thread>

#ifdef _WIN32
    // Windows platform
    using StreamDescriptor = boost::asio::windows::stream_handle;
#else
    // Assume POSIX-like platform
    using StreamDescriptor = boost::asio::posix::stream_descriptor;
#endif

namespace sgns
{
    SINGLETON_PTR_INIT(SFTPLoader);
    SFTPLoader::SFTPLoader()
    {
        FileManager::GetInstance().RegisterLoader("sftp", this);
    }

    std::shared_ptr<void> SFTPLoader::LoadFile(std::string filename)
    {
        const char* dummyValue = "Inside the SFTPTLoader::LoadFile Function";
        // for this test, we don't need to delete the shared_ptr as the data is static, so pass null lambda delete function
        return { (void*)dummyValue, [](void*) {} };
        /* TODO: scorpioluck20 - Need to implement this. How we load file base on format file?*/
    }

    void asyncCleanup(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, LIBSSH2_SFTP_HANDLE* sftpHandle)
    {
        libssh2_sftp_close_handle(sftpHandle);
        libssh2_sftp_shutdown(sftp);
        libssh2_session_disconnect(session, "Normal Shutdown");
        libssh2_session_free(session);
        libssh2_exit();
    }


    void asyncBlockRead(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<std::vector<char>> buffer,
        std::shared_ptr<boost::asio::ip::tcp::socket> tcpSocket,
        std::function<void(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<std::vector<char>> buffer, bool parse, bool save)> handle_read,
        size_t totalBytesRead, LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, LIBSSH2_SFTP_HANDLE* sftpHandle, bool parse, bool save)
    {
        libssh2_session_set_blocking(session, 0);
        int rc;
        rc = libssh2_sftp_read(sftpHandle, buffer->data() + totalBytesRead, buffer->size() - totalBytesRead);
        //std::cout << "Reading: " << rc << std::endl;
        
        if (rc > 0) {
            totalBytesRead += rc;
            // Process data if available
            if (totalBytesRead >= buffer->size())
            {
                //We've read all the data, send to parse/save
                std::cout << "SFTP Finish" << std::endl;
                handle_read(ioc, buffer, parse, save);
                asyncCleanup(session, sftp, sftpHandle);
            }
            else {
                //Async wait for the next part
                tcpSocket->async_wait(boost::asio::socket_base::wait_read, [ioc, buffer, tcpSocket, handle_read, totalBytesRead, session, sftp, sftpHandle, parse, save](const boost::system::error_code& ec) {
                    if (!ec) {
                        // Socket is readable, continue the SFTP read operation
                        asyncBlockRead(ioc, buffer, tcpSocket, handle_read, totalBytesRead, session, sftp, sftpHandle, parse, save);
                    }
                    else {
                        // Handle error
                    }
                    });
            }

        }
        else if (rc == LIBSSH2_ERROR_EAGAIN) {
            // Operation would block, set up asynchronous wait
            tcpSocket->async_wait(boost::asio::socket_base::wait_read, [ioc, buffer, tcpSocket, handle_read, totalBytesRead, session, sftp, sftpHandle, parse, save](const boost::system::error_code& ec) {
                if (!ec) {
                    // Socket is readable, continue the SFTP read operation
                    asyncBlockRead(ioc, buffer, tcpSocket, handle_read, totalBytesRead, session, sftp, sftpHandle, parse, save);
                }
                else {
                    // Handle error
                }
                });
        }
        else {
            // Handle other errors
        }
    }

    void asyncSFTPSize(std::shared_ptr<boost::asio::io_context> ioc,
        LIBSSH2_SESSION* session,
        std::string sftp_path,
        std::shared_ptr<boost::asio::ip::tcp::socket> tcpSocket,
        LIBSSH2_SFTP* sftp,
        std::function<void(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<std::vector<char>> buffer, bool parse, bool save)> handle_read,
        LIBSSH2_SFTP_HANDLE* sftpHandle, bool parse, bool save)
    {
        std::string fullPath = "." + sftp_path;
        // Get the size of the file
        LIBSSH2_SFTP_ATTRIBUTES sftpAttrs;
        int rc = libssh2_sftp_stat(sftp, fullPath.c_str(), &sftpAttrs);
        if (rc == 0)
        {
            //Got size, start reading to buffer
            std::size_t file_size = sftpAttrs.filesize;
            auto buffer = std::make_shared<std::vector<char>>(file_size);
            asyncBlockRead(ioc, buffer, tcpSocket, handle_read, 0, session, sftp, sftpHandle, parse, save);
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN)
        {
            //Continue getting size
            tcpSocket->async_wait(boost::asio::socket_base::wait_read, [ioc, session, sftp_path, tcpSocket, sftp, handle_read, sftpHandle, parse, save](const boost::system::error_code& ec) {
                if (!ec) {
                    asyncSFTPSize(ioc, session, sftp_path, tcpSocket, sftp, handle_read, sftpHandle, parse, save);
                }
                else {
                }
                });
        }
        else {

        }
    }
    void asyncSFTPOpen(std::shared_ptr<boost::asio::io_context> ioc,
        LIBSSH2_SESSION* session,
        std::string sftp_path,
        std::shared_ptr<boost::asio::ip::tcp::socket> tcpSocket,
        LIBSSH2_SFTP* sftp,
        std::function<void(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<std::vector<char>> buffer, bool parse, bool save)> handle_read,
        bool parse, bool save)
    {
        std::string fullPath = "." + sftp_path;
        LIBSSH2_SFTP_HANDLE* sftpHandle = libssh2_sftp_open(sftp, fullPath.c_str(), LIBSSH2_FXF_READ, 0);
        if (sftpHandle == nullptr)
        {
            // Handle the error, if it is EAGAIN we need to continue
            int sftp_error_code = libssh2_session_last_errno(session);
            if (sftp_error_code == LIBSSH2_ERROR_EAGAIN)
            {
                tcpSocket->async_wait(boost::asio::socket_base::wait_read, [ioc, session, sftp_path, tcpSocket, sftp, handle_read, parse, save](const boost::system::error_code& ec) {
                    if (!ec) {
                        asyncSFTPOpen(ioc, session, sftp_path, tcpSocket, sftp, handle_read, parse, save);
                    }
                    else {
                    }
                    });
            }
            else {

            }
        }
        else {
            //SFTP Opened, get file size
            asyncSFTPSize(ioc, session, sftp_path, tcpSocket, sftp, handle_read, sftpHandle, parse, save);
        }

    }

    void asyncSFTPCreateSFTP(std::shared_ptr<boost::asio::io_context> ioc,
        LIBSSH2_SESSION* session,
        std::string sftp_path,
        std::shared_ptr<boost::asio::ip::tcp::socket> tcpSocket,
        std::function<void(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<std::vector<char>> buffer, bool parse, bool save)> handle_read,
        bool parse, bool save)
    {
        LIBSSH2_SFTP* sftp = libssh2_sftp_init(session);
        if (sftp == nullptr) {
            // Handle the error, if it is EAGAIN we need to continue
            int sftp_error_code = libssh2_session_last_errno(session);
            if (sftp_error_code == LIBSSH2_ERROR_EAGAIN)
            {
                tcpSocket->async_wait(boost::asio::socket_base::wait_read, [ioc, session, sftp_path, tcpSocket, handle_read, parse, save](const boost::system::error_code& ec) {
                    if (!ec) {
                        asyncSFTPCreateSFTP(ioc, session, sftp_path, tcpSocket, handle_read, parse, save);
                    }
                    else {
                    }
                    });
            }
            else {

            }
        }
        else {
            // SFTP instance initialization succeeded
            asyncSFTPOpen(ioc, session, sftp_path, tcpSocket, sftp, handle_read, parse, save);
        }
    }

    void asyncSFTPAuth(std::shared_ptr<boost::asio::io_context> ioc,
        LIBSSH2_SESSION* session,
        std::string sftp_path,
        std::string sftp_user,
        std::string sftp_pass,
        std::string sftp_pubkeyfile,
        std::string sftp_privkeyfile,
        std::string sftp_privkeypass,
        std::shared_ptr<boost::asio::ip::tcp::socket> tcpSocket,
        std::function<void(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<std::vector<char>> buffer, bool parse, bool save)> handle_read,
        bool parse, bool save)
    {
        int auth_result;
        if (!sftp_privkeyfile.empty())
        {
            auth_result = libssh2_userauth_publickey_fromfile(session, sftp_user.c_str(), nullptr, sftp_privkeyfile.c_str(), nullptr);
        }
        else {
            if (!sftp_pubkeyfile.empty()) {
                auth_result =libssh2_userauth_publickey_fromfile(session, sftp_user.c_str(), nullptr, sftp_pubkeyfile.c_str(), sftp_privkeypass.c_str());
            }
            else {
                auth_result = libssh2_userauth_password(session, sftp_user.c_str(), sftp_pass.c_str());
            }
        }
        
        if (auth_result == 0) {
            // Auth successful, create sftp instance
            asyncSFTPCreateSFTP(ioc, session, sftp_path, tcpSocket, handle_read, parse, save);
            
        }
        else if (auth_result == LIBSSH2_ERROR_EAGAIN) {
            tcpSocket->async_wait(boost::asio::socket_base::wait_read, [ioc, session, sftp_path, sftp_user, sftp_pass, sftp_pubkeyfile, sftp_privkeyfile, sftp_privkeypass, tcpSocket, handle_read, parse, save](const boost::system::error_code& ec) {
                if (!ec) {
                    // Continue with auth
                    asyncSFTPAuth(ioc, session, sftp_path, sftp_user, sftp_pass, sftp_pubkeyfile, sftp_privkeyfile, sftp_privkeypass, tcpSocket, handle_read, parse, save);
                }
                else {
                    // Handle error
                }
                });
        }
        else
        {

        }
    }

    void asyncSFTPHandshake(std::shared_ptr<boost::asio::io_context> ioc,
        LIBSSH2_SESSION* session,
        std::string sftp_path,
        std::string sftp_user,
        std::string sftp_pass,
        std::string sftp_pubkeyfile,
        std::string sftp_privkeyfile,
        std::string sftp_privkeypass,
        std::shared_ptr<boost::asio::ip::tcp::socket> tcpSocket,
        std::function<void(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<std::vector<char>> buffer, bool parse, bool save)> handle_read,
        bool parse, bool save)
    {
        libssh2_socket_t sock;
        sock = tcpSocket->native_handle();
        int rc = libssh2_session_handshake(session, sock);
       
        if (rc == 0) {
            // Handshake successful, proceed with authentication
            asyncSFTPAuth(ioc, session, sftp_path, sftp_user, sftp_pass, sftp_pubkeyfile, sftp_privkeyfile, sftp_privkeypass, tcpSocket, handle_read, parse, save);
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN) {
            tcpSocket->async_wait(boost::asio::socket_base::wait_read, [ioc, session, sftp_path, sftp_user, sftp_pass, sftp_pubkeyfile, sftp_privkeyfile, sftp_privkeypass, tcpSocket, handle_read, parse, save](const boost::system::error_code& ec) {
                if (!ec) {
                    // Continue with handshake
                    asyncSFTPHandshake(ioc, session, sftp_path, sftp_user, sftp_pass, sftp_pubkeyfile, sftp_privkeyfile, sftp_privkeypass, tcpSocket, handle_read, parse, save);
                }
                else {
                    // Handle error
                }
                });
        }
        else
        {

        }
    }
    void asyncSFTPRead(
        std::shared_ptr<boost::asio::io_context> ioc,
        LIBSSH2_SESSION* session,
        std::string sftp_path,
        std::string sftp_user,
        std::string sftp_pass,
        std::string sftp_pubkeyfile,
        std::string sftp_privkeyfile,
        std::string sftp_privkeypass,
        std::shared_ptr<boost::asio::ip::tcp::socket> tcpSocket,
        std::function<void(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<std::vector<char>> buffer, bool parse, bool save)> handle_read,
        bool parse, bool save) {

        // Connect to the SFTP server
        libssh2_socket_t sock;
        sock = tcpSocket->native_handle();
        //libssh2_session_handshake(session, sock);
        libssh2_session_set_blocking(session, 0);

        asyncSFTPHandshake(ioc, session, sftp_path, sftp_user, sftp_pass, sftp_pubkeyfile, sftp_privkeyfile, sftp_privkeypass, tcpSocket, handle_read, parse, save);

        // Authenticate, giving priority to private key, then public key, then user/pass
        //int auth_result;
        //if (!sftp_privkeyfile.empty())
        //{
        //    auth_result = libssh2_userauth_publickey_fromfile(session, sftp_user.c_str(), nullptr, sftp_privkeyfile.c_str(), nullptr);
        //}
        //else {
        //    if (!sftp_pubkeyfile.empty()) {
        //        auth_result =libssh2_userauth_publickey_fromfile(session, sftp_user.c_str(), nullptr, sftp_pubkeyfile.c_str(), sftp_privkeypass.c_str());
        //    }
        //    else {
        //        auth_result = libssh2_userauth_password(session, sftp_user.c_str(), sftp_pass.c_str());
        //    }
        //}


        //// Combine . and sftpPath to form the full path
        //std::string fullPath = "." + sftp_path;

        //// Open an SFTP channel
        //LIBSSH2_SFTP* sftp = libssh2_sftp_init(session);
        //LIBSSH2_SFTP_HANDLE* sftpHandle = libssh2_sftp_open(sftp, fullPath.c_str(), LIBSSH2_FXF_READ, 0);

        //if (!sftpHandle) {
        //    int sftp_error_code = libssh2_sftp_last_error(sftp);
        //    std::cerr << "Error opening SFTP file handle: " << sftp_error_code << std::endl;
        //}
        //// Get the size of the file
        //LIBSSH2_SFTP_ATTRIBUTES sftpAttrs;
        //libssh2_sftp_stat(sftp, fullPath.c_str(), &sftpAttrs);
        //std::size_t file_size = sftpAttrs.filesize;

        ////Create a buffer based on file size
        //auto buffer = std::make_shared<std::vector<char>>(file_size);

        //libssh2_session_set_blocking(session, 0);

            //Start reading
        //int rc;
        //size_t totalBytesRead = 0;
        //bool complete = false;
        //while (!complete) {
        //    rc = libssh2_sftp_read(sftpHandle, buffer->data() + totalBytesRead, buffer->size() - totalBytesRead);
        //    if (rc > 0)
        //    {
        //        // Increment the total bytes read
        //        totalBytesRead += rc;
        //        //std::cout << "reading" << std::endl;
        //        // Check if the buffer is full
        //        if (totalBytesRead >= buffer->size()) {
        //            // Process the buffer
        //            std::cout << "SFTP Finish" << std::endl;
        //            ioc->post([ioc, buffer, parse, save, handle_read]() {
        //                handle_read(ioc, buffer, parse, save);
        //                });
        //            complete = true;

        //            // Reset totalBytesRead for the next iteration
        //                //totalBytesRead = 0;
        //        }
        //    }
        //    else {
        //        std::cout << "Not complete but err" << std::endl;
        //    }
        //}
        //asyncBlockRead(ioc, buffer, tcpSocket, handle_read, totalBytesRead, session, sftp, sftpHandle, parse, save);
            //Cleanup
        //libssh2_sftp_close_handle(sftpHandle);
        //libssh2_sftp_shutdown(sftp);
        //libssh2_session_disconnect(session, "Normal Shutdown");
        //libssh2_session_free(session);
        //libssh2_exit();

    }





    std::shared_ptr<void> SFTPLoader::LoadASync(std::string filename, bool parse, bool save, std::shared_ptr<boost::asio::io_context> ioc, CompletionCallback handle_read)
    {
            //Parse hostname and path
        std::string sftp_host;
        std::string sftp_path;
        std::string sftp_user;
        std::string sftp_pass;
        std::string sftp_pubkeyfile;
        std::string sftp_privkeyfile;
        std::string sftp_privekeypass;
        parseSFTPUrl(filename, sftp_host, sftp_path, sftp_user, sftp_pass, sftp_pubkeyfile, sftp_privkeyfile, sftp_privekeypass);
        std::cout << "host " << sftp_host << std::endl;
        std::cout << "path " << sftp_path << std::endl;
        std::cout << "user " << sftp_user << std::endl;
        std::cout << "pass " << sftp_pass << std::endl;
        std::cout << "pubkeyfile " << sftp_pubkeyfile << std::endl;
        std::cout << "privkeyfile " << sftp_privkeyfile << std::endl;
        std::cout << "privkeypass " << sftp_privekeypass << std::endl;

        // Initialize libssh2
        libssh2_init(0);
        
        // Create an SSH session
        LIBSSH2_SESSION* session = libssh2_session_init();
        //Debugging Items for inspecting communications
        //libssh2_trace(session, LIBSSH2_TRACE_AUTH | LIBSSH2_TRACE_SOCKET | LIBSSH2_TRACE_TRANS);
        //libssh2_trace(session, LIBSSH2_TRACE_AUTH | LIBSSH2_TRACE_SOCKET);
        boost::asio::ip::tcp::resolver resolver(*ioc);
        auto const results = resolver.resolve(sftp_host, "22");

        auto tcpSocket = std::make_shared<boost::asio::ip::tcp::socket>(*ioc);


        boost::asio::async_connect(*tcpSocket, results, [session, tcpSocket, sftp_path, sftp_user, sftp_pass, sftp_pubkeyfile, sftp_privkeyfile, sftp_privekeypass, ioc, handle_read, parse, save](const boost::system::error_code& connect_error, const auto& /*endpoint*/) {
            if (!connect_error)
            {
                //Create a new thread to process this synchronous sftp read.
                //std::thread([ioc, session, tcpSocket, sftp_user, sftp_pass, sftp_path, sftp_pubkeyfile, sftp_privkeyfile, sftp_privekeypass, handle_read, parse, save]() {
                    asyncSFTPRead(ioc, session, sftp_path, sftp_user, sftp_pass, sftp_pubkeyfile, sftp_privkeyfile, sftp_privekeypass, tcpSocket, handle_read, parse,save);
                    //}).detach();
            }
            else {
                std::cerr << "Error connecting to server: " << connect_error.message() << std::endl;
            }
            });
 

        std::shared_ptr<string> result = std::make_shared < string>("test");
        return result;
    }

} // End namespace sgns
