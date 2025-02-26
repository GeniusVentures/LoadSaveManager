/**
 * Header file for FILEError
 */
#ifndef FILERROR_HPP
#define FILERROR_HPP

#include <boost/outcome.hpp>

namespace sgns
{
    namespace AsyncError
    {
		namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;
        struct Success {
            std::string message;
        };
        using CustomResult = sgns::AsyncError::outcome::result<Success, std::string>;
    }
}

#endif