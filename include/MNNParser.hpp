#ifndef MNNPARSER_HPP
#define MNNPARSER_HPP

#include <memory>
#include "FileParser.hpp"
#include "MNNCommon.hpp"
namespace sgns
{
    class MNNParser: public FileParser
    {
            SINGLETON_PTR (MNNParser);
        public:
            static void InitializeSingleton();
            virtual shared_ptr<void> ParseData(shared_ptr<void> data)
                    override;
            virtual shared_ptr<void> ParseASync(std::shared_ptr<std::vector<char>> data)
                override;
    };
} // End namespace sgns

#endif  // MNNPARSER_HPP
