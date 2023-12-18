/**
 * Header file for the MNNLoader
 */

#ifndef INCLUDE_HTTPLOADER_HPP_
#define INCLUDE_HTTPLOADER_HPP_

#include <memory>
#include <string>
#include "FileLoader.hpp"
//#include "MNNCommon.hpp"
#include "Singleton.hpp"

namespace sgns
{

    /**
     * This class is for parsing the information in an MNN model file.
     * If you want to use this class, we can inheritance from this class
     * and implement logic based on model info
     */
    class HTTPLoader : public FileLoader
    {
        SINGLETON_PTR(HTTPLoader);
    public:

        /**ok
         * Load Data on the MNN file
         * @param filename - MNN file part
         * @return Interpreter of MNN file
         *
         */
        std::shared_ptr<void> LoadFile(std::string filename) override;
        std::shared_ptr<void> LoadASync(std::string filename, bool parse, std::shared_ptr<boost::asio::io_context> ioc) override;
    protected:

    };

} // End namespace sgns

#endif /* INCLUDE_HTTPLOADER_HPP_ */
