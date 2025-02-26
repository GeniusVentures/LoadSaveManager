#include <sstream>
#include <filesystem>
#include <fstream>
#include <streambuf>
#include <string>
#include "FileManager.hpp"
#include "MNNLoader.hpp"
#include "FILECommon.hpp"
namespace sgns
{
    MNNLoader* MNNLoader::_instance = nullptr;
    void MNNLoader::InitializeSingleton() {
        if (_instance == nullptr) {
            _instance = new MNNLoader();
        }
    }
    MNNLoader::MNNLoader()
    {
        FileManager::GetInstance().RegisterLoader("file", this);
    }

    std::shared_ptr<void> MNNLoader::LoadFile(std::string filename)
    {
        if (!std::filesystem::exists(filename))
        {
            throw std::range_error("File was not exist in system");
        }
        std::ifstream inputFile(filename, std::ios_base::binary);
        if (!inputFile.is_open())
        {
            throw std::range_error("Can not open file");
        }
        // Read all file to string
        std::string dataContent((std::istreambuf_iterator<char>(inputFile)),
                std::istreambuf_iterator<char>());
        inputFile.close();
        std::shared_ptr<string> result = std::make_shared<string>(
                dataContent);
        return result;
    }

    std::shared_ptr<void> MNNLoader::LoadASync(std::string filename,bool parse,bool save,std::shared_ptr<boost::asio::io_context> ioc, CompletionCallback handle_read, StatusCallback status)
    {
        std::shared_ptr<string> result = std::make_shared < string>("init");
        // Create a file device which will have a stream_descriptor or stream_file based on whether we are on posix OS or not.
        auto fileDevice = std::make_shared<FILEDevice>(ioc, filename, 0);
        auto buffer = std::make_shared<boost::asio::streambuf>();
        status(CustomResult(sgns::AsyncError::outcome::success(Success{ "Starting local file read" })));
        ////Async Read.
       boost::asio::async_read(fileDevice->getFile(), *buffer,
            boost::asio::transfer_all(),
            [fileDevice, ioc, handle_read, status, parse, save, buffer, filename](const boost::system::error_code& error, std::size_t bytes_transferred) {
                if (error.value() == 2)
                {
                    std::cout << "LOCAL Finish" << std::endl;
                    //auto finalbuffer = std::make_shared<std::vector<char>>(boost::asio::buffers_begin(buffer->data()), boost::asio::buffers_end(buffer->data()));
                    status(CustomResult(sgns::AsyncError::outcome::success(Success{ "Local File Finished Reading" })));
                    auto finaldata = std::make_shared<std::pair<std::vector<std::string>, std::vector<std::vector<char>>>>();
                    std::filesystem::path p(filename);
                    finaldata->first.push_back(p.filename().string());
                    size_t dataSize = buffer->size();
                    finaldata->second.emplace_back(
                        boost::asio::buffers_begin(buffer->data()),
                        boost::asio::buffers_begin(buffer->data()) + dataSize
                    );
                    handle_read(ioc, finaldata, parse, save);
                }
                else {
                    std::cerr << "File read error: " << error.message() << std::endl;
                    status(CustomResult(sgns::AsyncError::outcome::failure("Local File Read Fail")));
                    handle_read(ioc, std::shared_ptr<std::pair<std::vector<std::string>, std::vector<std::vector<char>>>>(), false, false);
                }
            });
       
        return result;
    }

} // End namespace sgns
