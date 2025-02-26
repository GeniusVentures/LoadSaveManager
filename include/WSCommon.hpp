/**
 * Header file for the WSCommon
 */
#ifndef WSCOMMON_HPP
#define WSCOMMON_HPP
#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <streambuf>
#include <string>
#include <memory>
#include "boost/beast/websocket/ssl.hpp"
#include "boost/beast.hpp"
#include "boost/asio.hpp"
#include "URLStringUtil.h"
#include "FILEError.hpp"
using Success = sgns::AsyncError::Success;
using CustomResult = sgns::AsyncError::CustomResult;


namespace sgns
{
	using namespace boost::asio;
	/**
	* This class creates an WS Device and has a function to download
	* from an WS server.
	*/
	class WSDevice : public std::enable_shared_from_this<WSDevice> {
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
		 * Create an WS Device to load a file from WS.
		 * @param ws_host - address of WSS Server
		 * @param ws_path - File on WS server to get
		 * @param ws_port - Port for WS Server
		 * @param parse - Whether to parse file upon completion (for MNN currently)
		 * @param save - Whether to save the file to local disk upon completion
		 */
		WSDevice(
			std::string ws_host,
			std::string ws_path,
			std::string ws_port,
			bool parse, bool save);
		~WSDevice() {
			// Cleanup
		}
		/**
		 * Start downloading file on an HTTPDevice
		 * @param ioc - ASIO context for async loading
		 * @param handle_read - Filemanager callback on completion
		 * @param status - Status function that will be updated with status codes as operation progresses
		 */
		void StartWSDownload(std::shared_ptr<boost::asio::io_context> ioc, CompletionCallback handle_read, StatusCallback status);
	private:
		/**
		 * Post WS GET_FILE to download file
		 * @param ioc - ASIO context for async loading
		 * @param ws - Websock item to get from
		 * @param handle_read - Filemanager callback on completion
		 * @param status - Status function that will be updated with status codes as operation progresses
		 */
		void StartWSGet(std::shared_ptr<boost::asio::io_context> ioc,
			std::shared_ptr<boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>> ws,
			CompletionCallback handle_read,
			StatusCallback status);

		//Common vars used for getting file from SFTP
		std::string ws_host_;
		std::string ws_path_;
		std::string ws_port_;
		bool parse_;
		bool save_;
		bool downloading_ = false;
	};
}

#endif