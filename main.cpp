#include <stdio.h>
#include <curl/curl.h>
#include <string>
#include <future>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <exception>
#include <iterator>
#include <boost\program_options.hpp>

namespace opt = boost::program_options;

static size_t WriteData(char *data, size_t size, size_t nmemb, std::ofstream* buffer) {
    size_t result = 0;
    if (buffer != NULL) {
        buffer->write(data, size*nmemb);
        result = size * nmemb;
    }
    return result;
}

int DownloadFile(const std::string& url, const std::string& filename) {
    CURL *curl;
    CURLcode res;
    std::ofstream output(filename, std::ios::binary);

    if (output) {
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
                curl_easy_cleanup(curl);
            }
        }
    }
    output.close();
    return 0;
}

std::vector<std::string> ParseCommand(int argc, char **argv) {
    opt::options_description desc("All options");

    desc.add_options()
        ("help,h", "produce help message")
        ("uri,i", opt::value<std::vector<std::string> >(), "list of files to download")
        ;

    opt::variables_map vm;
    opt::store(opt::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        exit(0);
    }

    opt::notify(vm);

    return vm["uri"].as<std::vector<std::string> >();
}

std::string ParseFilename(std::string& url) {
    std::string filename;
    for (std::string::reverse_iterator rit = url.rbegin(); rit != url.rend(); ++rit) {
        if (*rit == '/') {
            break;
        }
        filename += *rit;
    }

    reverse(filename.begin(), filename.end());

    return filename;
}

int main(int argc, char *argv[]) {

    try {
        std::vector<std::string> fileList = ParseCommand(argc, argv);
        for (std::vector<std::string>::iterator it = fileList.begin(); it < fileList.end(); ++it) {
            std::string url = *it;
            std::string filename = ParseFilename(url);

            std::future<int> fut = std::async(DownloadFile, url, filename);

            std::cout << "Getting " << filename << "...";

            int errorCode = fut.get();
            if (!errorCode) {
                std::cout << "OK" << std::endl;
            }
            else {
                std::cout << "Error " << errorCode << std::endl;
                exit(1);
            }
        }
    }
    catch (std::exception& ex) {
        std::cout << "Exception happend " << ex.what() << std::endl;
    }

    return 0;
}
