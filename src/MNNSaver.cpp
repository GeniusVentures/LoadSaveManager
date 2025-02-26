/*
 * MNNSaver.cpp
 */

#include <iostream>
#include <fstream>
#include <streambuf>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "FileManager.hpp"
#include "MNNSaver.hpp"
#include "FILECommon.hpp"

namespace sgns
{
    MNNSaver* MNNSaver::_instance = nullptr;
    void MNNSaver::InitializeSingleton() {
        if (_instance == nullptr) {
            _instance = new MNNSaver();
        }
    }
    MNNSaver::MNNSaver()
    {
        FileManager::GetInstance().RegisterSaver("file", this);
        FileManager::GetInstance().RegisterSaver("mnn", this);
    }

    void MNNSaver::SaveFile(std::string filename, std::shared_ptr<void> data)
    {
        if (data == nullptr)
        {
            throw range_error("Can not save with null data");
        }
        std::shared_ptr<string> fileContent =
                std::static_pointer_cast<string>(data);
        ofstream outputFile(filename, std::ios_base::binary);
        if (!outputFile.is_open())
        {
            throw range_error("Can not create file for save");
        }
        outputFile.write(fileContent.get()->c_str(),
                fileContent.get()->size());
        outputFile.close();
    }


    void MNNSaver::SaveASync(std::shared_ptr<boost::asio::io_context> ioc, 
        std::function<void(std::shared_ptr<boost::asio::io_context> ioc)> handle_write,
        std::string filename, std::shared_ptr<std::pair<std::vector<std::string>, std::vector<std::vector<char>>>> data, std::string suffix)
    {
        if (data->second.data() == nullptr)
        {
            throw range_error("Can not save with null data");
        }
        if (filename.empty()) {
            filename = boost::lexical_cast<string>((boost::uuids::random_generator())()) + "/";
        }

        

        //size_t remainingWrites = data.first.size();
        auto remainingWrites = std::make_shared<size_t>(data->first.size());
        for (size_t i = 0; i < data->first.size(); ++i) {
            //Create Directories for files
            const std::string& directoryWithFile = filename + data->first[i];
            std::cout << "dirwithfile: " << directoryWithFile << std::endl;
            std::filesystem::path filePath(directoryWithFile);
            std::filesystem::path directory = filePath.parent_path();
            std::filesystem::create_directories(directory);

            //Create Steam for async writes
            std::ofstream file(directoryWithFile, std::ios::binary);
            auto fileDevice = std::make_shared<FILEDevice>(ioc, directoryWithFile, 1);

            async_write(fileDevice->getFile(), boost::asio::buffer(data->second[i].data(), data->second[i].size()), boost::asio::transfer_exactly(data->second[i].size()), [fileDevice, ioc, handle_write, data, remainingWrites](const boost::system::error_code& error, std::size_t bytes_transferred)
                {
                    std::cout << "wrote" << std::endl;
                    (*remainingWrites)--;
                    if (*remainingWrites <= 0)
                    {
                        //Handle when written all
                        handle_write(ioc);
                    }
                });
        }
    }

} // End namespace sgns
