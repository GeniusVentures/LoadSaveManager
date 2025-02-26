/**
 * Header file for the HTTPCommon
 */
#ifndef HTTPCOMMON_HPP
#define HTTPCOMMON_HPP
#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <streambuf>
#include <string>
#include <memory>
#include "boost/asio/ssl.hpp"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "URLStringUtil.h"
#include "FILEError.hpp"
using Success = sgns::AsyncError::Success;
using CustomResult = sgns::AsyncError::CustomResult;


namespace sgns
{
	using namespace boost::asio;
	/**
	 * This class creates an HTTP Device and has a function to download
	 * from an HTTP server.
	 */
	class HTTPDevice : public std::enable_shared_from_this<HTTPDevice> {
	public:
		/**
		 * Completion callback template. We expect an io_context so the thread can be shut down if no outstanding async loads exist, and a buffer with the read information
		 * @param ioc - asio io context so we can stop this if no outstanding async tasks remain
		 * @param buffers - Contains path/data loaded
		 * @param parse - Whether to parse file upon completion (for MNN)
		 * @param save - Whether to save the file to local disk upon completion
		 */
		using CompletionCallback = std::function<void(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<std::pair<std::vector<std::string>, std::vector<std::vector<char>>>> buffers, bool parse, bool save)>;
		
		/**
		 * Status callback returns an error code as an async load proceeds
		 * @param int - Status code
		 */
		using StatusCallback = std::function<void(const CustomResult&)>;

		/**
		 * Create an HTTP Device to load a file from HTTP.
		 * @param http_host - address of HTTP Server
		 * @param http_path - File on HTTP server to get
		 * @param http_port - Port for HTTPS Server
		 * @param parse - Whether to parse file upon completion (for MNN currently)
		 * @param save - Whether to save the file to local disk upon completion
		 */
		HTTPDevice(
			std::string http_host,
			std::string http_path,
			std::string http_port,
			bool parse, bool save);
		~HTTPDevice() {
			// Cleanup
		}
		/**
		 * Start downloading file on an HTTPDevice
		 * @param ioc - ASIO context for async loading
		 * @param handle_read - Filemanager callback on completion
		 * @param status - Status function that will be updated with status codes as operation progresses
		 */
		void StartHTTPDownload(std::shared_ptr<boost::asio::io_context> ioc, CompletionCallback handle_read, StatusCallback status);
	private:
		/**
		 * Post HTTP Get to download file
		 * @param ioc - ASIO context for async loading
		 * @param socket - SSL socket to read on
		 * @param handle_read - Filemanager callback on completion
		 * @param status - Status function that will be updated with status codes as operation progresses
		 */
		void StartHTTPGet(std::shared_ptr<boost::asio::io_context> ioc,
			std::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> socket,
			CompletionCallback handle_read,
			StatusCallback status);

		//Common vars used for getting file from HTTP
		std::string http_host_;
		std::string http_path_;
		std::string http_port_;
		bool parse_;
		bool save_;
		bool downloading_ = false;
	};
}

#endif