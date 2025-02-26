// MNNParser.cpp

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include "FileManager.hpp"
#include "MNNParser.hpp"
#include <MNN/Interpreter.hpp>
namespace sgns
{
    //SINGLETON_PTR_INIT(MNNParser);
    MNNParser* MNNParser::_instance = nullptr;
    void MNNParser::InitializeSingleton() {
        if (_instance == nullptr) {
            _instance = new MNNParser();
        }
    }
    MNNParser::MNNParser()
    {
        FileManager::GetInstance().RegisterParser("mnn", this);
    }
    std::shared_ptr<void> MNNParser::ParseData(std::shared_ptr<void> data)
    {
        if(data == nullptr)
        {
            throw std::range_error("Can not parsing null data");
        }
        std::shared_ptr<string> fileContent =
                std::static_pointer_cast<string>(data);
        std::shared_ptr<MNN::Interpreter> mnn_interpreter(
                MNN::Interpreter::createFromBuffer(fileContent.get()->c_str(),
                        fileContent.get()->size()));
        if (mnn_interpreter == nullptr)
        {
            throw std::range_error("Can not parsing data from input");
        }
        MNN::Session *mnn_session = nullptr; /* The holder of inference data */
        MNN::Tensor *input_tensor = nullptr;
        MNN::ScheduleConfig schedule_config;
        int input_batch;
        int input_channel;
        int input_height;
        int input_width;
        MNN::Tensor::DimensionType m_dimension_type;
        int m_num_output;
        // Paring data of MNN file
        schedule_config.numThread = 1;
        MNN::BackendConfig backend_config;
        backend_config.precision = MNN::BackendConfig::Precision_High;
        schedule_config.backendConfig = &backend_config;
        // Create session for reading model
        mnn_session = mnn_interpreter->createSession(schedule_config);
        // Tensor input and input dims
        input_tensor = mnn_interpreter->getSessionInput(mnn_session, nullptr);
        input_batch = input_tensor->batch();
        input_channel = input_tensor->channel();
        input_height = input_tensor->height();
        input_width = input_tensor->width();
        m_dimension_type = input_tensor->getDimensionType();
        // Adapt interpreter base on dims
        switch (m_dimension_type)
        {
            // caffe net type
            case MNN::Tensor::CAFFE:
                mnn_interpreter->resizeTensor(input_tensor, { input_channel,
                        input_height, input_width });
                mnn_interpreter->resizeSession(mnn_session);
                break;
                // Tensorflow net type
            case MNN::Tensor::TENSORFLOW:
                mnn_interpreter->resizeTensor(input_tensor, { input_batch,
                        input_height, input_width, input_channel });
                mnn_interpreter->releaseSession(mnn_session);
                break;
                //C4HW4 as data format
            case MNN::Tensor::CAFFE_C4:
                break;
                // Any new type MNN support, we can adapt it later
            default:
                break;
        }
        m_num_output =
                mnn_interpreter->getSessionOutputAll(mnn_session).size();
        // Parsing data to string
        std::ostringstream output;
        if (mnn_interpreter)
        {
            output << "==================INPUT-DIMS================"
                    << std::endl;
            if (input_tensor)
            {
                input_tensor->printShape();
            }
            switch (m_dimension_type)
            {
                case MNN::Tensor::CAFFE:
                    output
                            << "Dimension Type: (CAFE/PyTorch/ONNX) uses NCHW as data format"
                            << std::endl;
                    break;
                case MNN::Tensor::TENSORFLOW:
                    output
                            << "Dimension Type: (TENSORFLOW) uses NHWC as data format"
                            << std::endl;
                    break;
                case MNN::Tensor::CAFFE_C4:
                    output
                            << "Dimension Type: (CAFE_C4) uses NC4HW4 as data format"
                            << std::endl;
                    break;
                default:
                    output << "Dimension Type: UNKNOWN" << std::endl;
                    break;
            }
            output << "==================OUTPUT-DIMS================"
                    << std::endl;
            auto output_map = mnn_interpreter->getSessionOutputAll(
                    mnn_session);
            for (auto it = output_map.cbegin(); it != output_map.cend(); it++)
            {
                output << "Output : " << it->first << std::endl;
                it->second->printShape();
            }
            output << "============================================="
                    << std::endl;
        }
        std::cout << output.str() << std::endl;
        return fileContent;
    }
    std::shared_ptr<void> MNNParser::ParseASync(std::shared_ptr<std::vector<char>> data)
    {
        const char* bufferData = data->data();
        size_t bufferSize = data->size();
        // Create MNN interpreter from the buffer
        //MNN::CV::ImageProcess* process = MNN::CV::ImageProcess::create();
        MNN::Interpreter* mnn_interpreter = MNN::Interpreter::createFromBuffer(bufferData, bufferSize);
        if (mnn_interpreter == nullptr)
        {
            throw std::range_error("Can not parsing data from input");
        }
        MNN::Session* mnn_session = nullptr; /* The holder of inference data */
        MNN::Tensor* input_tensor = nullptr;
        MNN::ScheduleConfig schedule_config;
        int input_batch;
        int input_channel;
        int input_height;
        int input_width;
        MNN::Tensor::DimensionType m_dimension_type;
        int m_num_output;
        // Paring data of MNN file
        schedule_config.numThread = 1;
        MNN::BackendConfig backend_config;
        backend_config.precision = MNN::BackendConfig::Precision_High;
        schedule_config.backendConfig = &backend_config;
        // Create session for reading model
        mnn_session = mnn_interpreter->createSession(schedule_config);
        // Tensor input and input dims
        input_tensor = mnn_interpreter->getSessionInput(mnn_session, nullptr);
        input_batch = input_tensor->batch();
        input_channel = input_tensor->channel();
        input_height = input_tensor->height();
        input_width = input_tensor->width();
        m_dimension_type = input_tensor->getDimensionType();
        // Adapt interpreter base on dims
        switch (m_dimension_type)
        {
            // caffe net type
        case MNN::Tensor::CAFFE:
            mnn_interpreter->resizeTensor(input_tensor, { input_channel,
                    input_height, input_width });
            mnn_interpreter->resizeSession(mnn_session);
            break;
            // Tensorflow net type
        case MNN::Tensor::TENSORFLOW:
            mnn_interpreter->resizeTensor(input_tensor, { input_batch,
                    input_height, input_width, input_channel });
            mnn_interpreter->releaseSession(mnn_session);
            break;
            //C4HW4 as data format
        case MNN::Tensor::CAFFE_C4:
            break;
            // Any new type MNN support, we can adapt it later
        default:
            break;
        }
        m_num_output =
            mnn_interpreter->getSessionOutputAll(mnn_session).size();
        // Parsing data to string
        std::ostringstream output;
        if (mnn_interpreter)
        {
            output << "==================INPUT-DIMS================"
                << std::endl;
            if (input_tensor)
            {
                input_tensor->printShape();
            }
            switch (m_dimension_type)
            {
            case MNN::Tensor::CAFFE:
                output
                    << "Dimension Type: (CAFE/PyTorch/ONNX) uses NCHW as data format"
                    << std::endl;
                break;
            case MNN::Tensor::TENSORFLOW:
                output
                    << "Dimension Type: (TENSORFLOW) uses NHWC as data format"
                    << std::endl;
                break;
            case MNN::Tensor::CAFFE_C4:
                output
                    << "Dimension Type: (CAFE_C4) uses NC4HW4 as data format"
                    << std::endl;
                break;
            default:
                output << "Dimension Type: UNKNOWN" << std::endl;
                break;
            }
            output << "==================OUTPUT-DIMS================"
                << std::endl;
            auto output_map = mnn_interpreter->getSessionOutputAll(
                mnn_session);
            for (auto it = output_map.cbegin(); it != output_map.cend(); it++)
            {
                output << "Output : " << it->first << std::endl;
                it->second->printShape();
            }
            output << "============================================="
                << std::endl;
        }
        std::cout << output.str() << std::endl;
        //return fileContent;
        std::shared_ptr<string> result = std::make_shared < string>("test");
        return result;
    }
} // End namespace sgns

