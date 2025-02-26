//
// Created by Super Genius on 6/12/23.
//

#include "URLStringUtil.h"
#include <stdexcept>

extern void getURLComponents(std::string url, std::string &prefix, std::string& base, std::string& extension)
{
    // Find the first occurrence of "://" in the URL.
    size_t index = url.find("://");
    // If "://" is not found, then the URL has no prefix.
    if (index == std::string::npos) {
        throw std::invalid_argument("url");
    }
    prefix = url.substr(0, index);
    base = url.substr(index+3, url.length());
    size_t start_index = base.rfind(".");
    if (start_index == std::string::npos) {
        extension = "";
    } else {
        extension = base.substr(start_index+1, base.length());
    }
}

extern void parseHTTPUrl(std::string url, std::string& host, std::string& path, std::string& port)
{
    // Find the first occurrence of "/" in the URL.
    size_t index = url.find("/");
    // If "/" is not found, then the URL has no prefix.
    if (index == std::string::npos) {
        throw std::invalid_argument("url");
    }
    host = url.substr(0, index);
    path = url.substr(index, url.length());
    //Look for : in the host to see if a port exists
    index = url.find(":");
    if (index == std::string::npos) {
        //Default to 443
        port = "443";
    }
    else {
        port = host.substr(index+1,host.length());
        host = host.substr(0, index);
    }
}

extern void parseSFTPUrl(std::string url, std::string& host, std::string& path, std::string& user, std::string& pass, std::string& publickey_file, std::string& privatekey_file, std::string& privatekey_pass)
{
    // Find the first occurrence of "@" in the URL.
    size_t index = url.find("@");
    // If "@" is not found, then we have no user/pass 
    if (index == std::string::npos) {
        user = "";
        pass = "";
    }
    else {
        user = url.substr(0, index);
        host = url.substr(index, url.length());
    }

    //Find Username and Pass
    index = user.find(":");
    // If ":" is not found, then we have no password minimally 
    if (index == std::string::npos) {
        pass = "";
    }
    else {
        pass = user.substr(index+1, user.length());
        user = user.substr(0, index);
    }

    //Find public key location
    index = pass.find("pubkey_identifier");
    // If "pubkey_identifier" is not found, then we have no key
    if (index != std::string::npos) {
        publickey_file = pass.substr(index + 17, pass.length());
        pass = "";
    }
    
    //Find Private key location
    index = pass.find("privkey_identifier");
    // If "privkey_identifier" is not found, then we have no private key
    if (index != std::string::npos) {
        privatekey_file = pass.substr(index + 18, pass.length());
        pass = "";
    }
    //Find Private key passphrase
    index = privatekey_file.find("key_passphrase");
    // If "key_passphrase" is not found, then we have no passphrase
    if (index != std::string::npos) {
        privatekey_pass = privatekey_file.substr(index + 14, privatekey_file.length());
        privatekey_file = privatekey_file.substr(0, index);
    }

    //Find host and path
    index = host.find("/");
    // If "/" is not found, then the URL is missing full path
    if (index == std::string::npos) {
        throw std::invalid_argument("url");
    }
    else {
        path = host.substr(index, host.length());
        host = host.substr(1, index-1);
    }
}

extern void parseIPFSUrl(std::string url, std::string& cid, std::string& file)
{
    // Find the first occurrence of "/" in the URL.
    size_t index = url.find("/");
    // If "/" is not found, then the URL has no prefix.
    if (index == std::string::npos) {
        throw std::invalid_argument("url");
    }
    cid = url.substr(0, index);
    file = url.substr(index+1, url.length());
}
