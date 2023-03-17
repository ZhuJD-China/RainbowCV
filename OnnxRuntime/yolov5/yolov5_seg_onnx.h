#include <iostream>
#include<memory>
#include <opencv2/opencv.hpp>
#include "../yolo_utils.h"
#include<onnxruntime_cxx_api.h>


class YoloSegOnnx
{
public:
	YoloSegOnnx() : _OrtMemoryInfo(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPUOutput))
	{
	}

	~YoloSegOnnx()
	{
	} // delete _OrtMemoryInfo;


public:
	/** \brief Read onnx-model
	* \param[in] modelPath:onnx-model path
	* \param[in] isCuda:if true,use Ort-GPU,else run it on cpu.
	* \param[in] cudaID:if isCuda==true,run Ort-GPU on cudaID.
	* \param[in] warmUp:if isCuda==true,warm up GPU-model.
	*/
	bool ReadModel(const std::string& modelPath, bool isCuda = false, int cudaID = 0, bool warmUp = true);

	/** \brief  detect.
	* \param[in] srcImg:a 3-channels image.
	* \param[out] output:detection results of input image.
	*/
	bool OnnxDetect(cv::Mat& srcImg, std::vector<OutputSeg>& output);


	/** \brief  detect,batch size= _batchSize
	* \param[in] srcImg:A batch of images.
	* \param[out] output:detection results of input images.
	*/
	bool OnnxBatchDetect(std::vector<cv::Mat>& srcImg, std::vector<std::vector<OutputSeg>>& output);

private:
	template <typename T>
	T VectorProduct(const std::vector<T>& v)
	{
		return std::accumulate(v.begin(), v.end(), 1, std::multiplies<T>());
	};
	int Preprocessing(const std::vector<cv::Mat>& SrcImgs, std::vector<cv::Mat>& OutSrcImgs,
	                  std::vector<cv::Vec4d>& params);
#if(defined YOLO_P6 && YOLO_P6==true)
	//const float _netAnchors[4][6] = { { 19,27, 44,40, 38,94 },{ 96,68, 86,152, 180,137 },{ 140,301, 303,264, 238,542 },{ 436,615, 739,380, 925,792 } };
	const int _netWidth = 1280;  //ONNX
	const int _netHeight = 1280; //ONNX
	const int _segWidth = 320;  //_segWidth=_netWidth/mask_ratio
	const int _segHeight = 320;
	const int _segChannels = 32;
	const int _strideSize = 4;  //stride size
#else
	//const float _netAnchors[3][6] = { { 10,13, 16,30, 33,23 },{ 30,61, 62,45, 59,119 },{ 116,90, 156,198, 373,326 } };
	const int _netWidth = 640; //ONNX-net-input-width
	const int _netHeight = 640; //ONNX-net-input-height
	const int _segWidth = 160; //_segWidth=_netWidth/mask_ratio
	const int _segHeight = 160;
	const int _segChannels = 32;
	const int _strideSize = 3; //stride size
#endif // YOLO_P6

	int _batchSize = 1; //if multi-batch,set this
	bool _isDynamicShape = false; //onnx support dynamic shape

	const int _netStride[4] = {8, 16, 32, 64};
	float _boxThreshold = 0.25;
	float _classThreshold = 0.5;
	float _nmsThreshold = 0.45;
	float _maskThreshold = 0.5;
	float _nmsScoreThreshold = _boxThreshold * _classThreshold;

	//ONNXRUNTIME	
	Ort::Env _OrtEnv = Ort::Env(ORT_LOGGING_LEVEL_ERROR, "Yolov5-Seg");
	Ort::SessionOptions _OrtSessionOptions = Ort::SessionOptions();
	Ort::Session* _OrtSession = nullptr;
	Ort::MemoryInfo _OrtMemoryInfo;

	std::shared_ptr<char> _inputName, _output_name0, _output_name1;
	std::vector<char*> _inputNodeNames;  // Enter node name
	std::vector<char*> _outputNodeNames; // Output node name

	size_t _inputNodesNum = 0; // Number of input nodes
	size_t _outputNodesNum = 0;  // Number of output nodes

	ONNXTensorElementDataType _inputNodeDataType;  // data type
	ONNXTensorElementDataType _outputNodeDataType;
	std::vector<int64_t> _inputTensorShape; // Input tensor shape

	std::vector<int64_t> _outputTensorShape;
	std::vector<int64_t> _outputMaskTensorShape;
};
