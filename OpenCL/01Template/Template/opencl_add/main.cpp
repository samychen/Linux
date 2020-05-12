#include <iostream>
#include <fstream>
#include <sstream>
#include <CL/cl.h>

const int ARRAY_SIZE = 1000000;

#ifdef _WIN32
#include <winsock2.h>
#include <time.h>
#else
#include <sys/time.h>
#endif

uint64_t GetCurrentTimeMsec()
{
#ifdef _WIN32
	struct timeval tv;
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;

	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tv.tv_sec = clock;
	tv.tv_usec = wtm.wMilliseconds * 1000;
	return ((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000);
#endif
}


//һ�� ѡ��OpenCLƽ̨������һ��������
cl_context CreateContext1()
{
	cl_int errNum;
	cl_uint numPlatforms;
	cl_platform_id firstPlatformId;
	cl_context context = NULL;

	//ѡ����õ�ƽ̨�еĵ�һ��
	errNum = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
	if (errNum != CL_SUCCESS || numPlatforms <= 0)
	{
		std::cerr << "Failed to find any OpenCL platforms." << std::endl;
		return NULL;
	}

	//����һ��OpenCL�����Ļ���
	cl_context_properties contextProperties[] =
	{
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)firstPlatformId,
		0
	};
	context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,
		NULL, NULL, &errNum);

	return context;
}

//һ�� ѡ��OpenCLƽ̨������һ��������
cl_context CreateContext()
{
	cl_int errNum;
	cl_uint numPlatforms;
	cl_platform_id firstPlatformId;
	cl_context context = NULL;

	clGetPlatformIDs(0, NULL, &numPlatforms);
	cl_platform_id *platforms = new cl_platform_id[numPlatforms];
// 	clGetPlatformIDs(numPlatforms, platforms, NULL);


	//ѡ����õ�ƽ̨��
	errNum = clGetPlatformIDs(numPlatforms, platforms, NULL);;
	if (errNum != CL_SUCCESS || numPlatforms <= 0)
	{
		std::cerr << "Failed to find any OpenCL platforms." << std::endl;
		return NULL;
	}

	//����һ��OpenCL�����Ļ���
	cl_context_properties contextProperties[] =
	{
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)platforms[0],
		0
	};
	context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,
		NULL, NULL, &errNum);

	return context;
}


//���� �����豸�������������
cl_command_queue CreateCommandQueue(cl_context context, cl_device_id *device)
{
	cl_int errNum;
	cl_device_id *devices;
	cl_command_queue commandQueue = NULL;
	size_t deviceBufferSize = -1;

	// ��ȡ�豸��������С
	errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);
	std::cout << "clGetContextInfo deviceBufferSize = " << deviceBufferSize << "\n";
	if (deviceBufferSize <= 0)
	{
		std::cerr << "No devices available.";
		return NULL;
	}

	// Ϊ�豸���仺��ռ�
	devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];
	errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);
	std::cout << "clGetContextInfo devices = " << deviceBufferSize / sizeof(cl_device_id) << "\n";
	//ѡȡ�����豸�еĵ�һ��
	commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);

	*device = devices[0];
	delete[] devices;
	return commandQueue;
}


// ���������͹����������
cl_program CreateProgram(cl_context context, cl_device_id device, const char* fileName)
{
	cl_int errNum;
	cl_program program;

	std::ifstream kernelFile(fileName, std::ios::in);
	if (!kernelFile.is_open())
	{
		std::cerr << "Failed to open file for reading: " << fileName << std::endl;
		return NULL;
	}

	std::ostringstream oss;
	oss << kernelFile.rdbuf();

	std::string srcStdStr = oss.str();
	const char *srcStr = srcStdStr.c_str();
	//�����������
	program = clCreateProgramWithSource(context, 1,
		(const char**)&srcStr,
		NULL, NULL);
	//����������
	errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (errNum != 0)
	{
		std::cerr << "clBuildProgram failed: " << errNum << std::endl;
		return NULL;
	}

	return program;
}

//�����͹����������
bool CreateMemObjects(cl_context context, cl_mem memObjects[3],
	float *a, float *b)
{
	memObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(float) * ARRAY_SIZE, a, NULL);
	memObjects[1] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(float) * ARRAY_SIZE, b, NULL);
	memObjects[2] = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(float) * ARRAY_SIZE, NULL, NULL);
	return true;
}


// �ͷ�OpenCL��Դ
void Cleanup(cl_context context, cl_command_queue commandQueue,
	cl_program program, cl_kernel kernel, cl_mem memObjects[3])
{
	for (int i = 0; i < 3; i++)
	{
		if (memObjects[i] != 0)
			clReleaseMemObject(memObjects[i]);
	}
	if (commandQueue != 0)
		clReleaseCommandQueue(commandQueue);

	if (kernel != 0)
		clReleaseKernel(kernel);

	if (program != 0)
		clReleaseProgram(program);

	if (context != 0)
		clReleaseContext(context);
}
float do_for_test(int count)
{
	long long sum = 0;
	for (int i = 0; i < count; i++)
	{
		sum += i;
	}
	return (float)sum;
}
int main(int argc, char** argv)
{
	cl_context context = 0;
	cl_command_queue commandQueue = 0;
	cl_program program = 0;
	cl_device_id device = 0;
	cl_kernel kernel = 0;
	cl_mem memObjects[3] = { 0, 0, 0 };
	cl_int errNum;

	// һ��ѡ��OpenCLƽ̨������һ��������
	context = CreateContext();		// GPUƽ̨ context device queue buffer kernel 

	// ���� �����豸�������������
	commandQueue = CreateCommandQueue(context, &device);

	// �� �����͹����������
	program = CreateProgram(context, device, "HelloWorld.cl");

	// �ġ� ����OpenCL�ں˲������ڴ�ռ�
	kernel = clCreateKernel(program, "hello_kernel", NULL);

	//����Ҫ���������
	float *result = new float[ARRAY_SIZE];
	float *a = new float[ARRAY_SIZE];
	float *b = new float[ARRAY_SIZE];

	float *cpu_result = new float[ARRAY_SIZE];
	for (int i = 0; i < ARRAY_SIZE; i++)
	{
		cpu_result[i] = 0;
		result[i] = 0;
		a[i] = (float)i;
		b[i] = (float)(ARRAY_SIZE - i);
	}

	uint64_t start_time = GetCurrentTimeMsec();
	//�����ڴ����
	if (!CreateMemObjects(context, memObjects, a, b))
	{
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}

	// �塢 �����ں����ݲ�ִ���ں�
	errNum = clSetKernelArg(kernel, 0, sizeof(cl_mem), &memObjects[0]);
	errNum |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &memObjects[1]);
	errNum |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &memObjects[2]);

	size_t globalWorkSize[1] = { ARRAY_SIZE };	// �ܹ���
	size_t localWorkSize[1] = { 1 };
	// ִ���ں�
	errNum = clEnqueueNDRangeKernel(commandQueue,//һ����Ч��������У��ں˽�����Ӳ���command_queue���������豸��ִ��
		kernel,//һ����Ч���ں˶���kernel ��������OpenCL�����ı����command_queue�������ı���һ��
		1, //��ά��������ָ��ȫ�ֹ�����͹������еĹ����work_dim�������0����С�ڵ�3. 
		NULL,//�ڵ�ǰ������NULL,�����޶�OpenCLʱ���˲�����������ָ��һ��work_dim�޷���ֵ��Ϊƫ�������������㹤�����ȫ��ID,�����طǵ�֪����ƫ��λ��(0,0,��,0)��ʼ��ȫ��ID. 
		globalWorkSize,//��һ��work_dim�޷���ֵ������work_dimά��������ִ���ں˺�����ȫ�ֹ��������Ŀ��ȫ�ֹ��������Ŀ�������㣺global_work_size[0]*��*global_work_size[work_dim - 1]�� 
		localWorkSize,//�������һ��������(��ִ��kernel)�Ĺ��������Ŀ.�����鳤����������������������local_work_size[0]*��*local_work_size[work_dim - 1]
		0,
		NULL,
		NULL);
	if (errNum != 0) 
	{
		std::cerr << "clEnqueueNDRangeKernel failed: " << errNum << std::endl;
	}
	uint64_t end_time = GetCurrentTimeMsec(); // ֻ��GPUִ����
	std::cout << "GPU Executed program time1: " << end_time - start_time << std::endl;
	// ���� ��ȡִ�н�����ͷ�OpenCL��Դ
	errNum = clEnqueueReadBuffer(commandQueue, memObjects[2], CL_TRUE,
		0, ARRAY_SIZE * sizeof(float), result,
		0, NULL, NULL);
	end_time = GetCurrentTimeMsec();
	std::cout << "GPU Executed program time2: " << end_time - start_time << std::endl;
	std::cout << std::fixed;//���ÿ�ѧ������
	std::cout.precision(0); //����һλС��
	
	
	for (int i = 0; i < ARRAY_SIZE; i++)
	{
// 		std::cout << result[i] << " ";
	}

	start_time = GetCurrentTimeMsec();
	for (int i = 0; i < ARRAY_SIZE; i++)
	{
		cpu_result[i] = a[i] + b[i] ;
		cpu_result[i] += do_for_test(1000);

	}
	end_time = GetCurrentTimeMsec();
	std::cout << "CPU Executed program time: " << end_time - start_time << std::endl;
	std::cout << std::endl;
	std::cout << std::fixed;//���ÿ�ѧ������
	std::cout.precision(0); //����һλС��

	std::cout << "result[0]:		" << result[0] << std::endl;
	std::cout << "result[ARRAY_SIZE-1]: " << result[ARRAY_SIZE - 1] << std::endl;

	std::cout << "cpu_result[0]: " << cpu_result[0] << std::endl;
	std::cout << "cpu_result[1]: " << cpu_result[1] << std::endl;
	std::cout << "Executed program succesfully." << std::endl;
	
	getchar();
	Cleanup(context, commandQueue, program, kernel, memObjects);

	delete[] result;
	delete[] a;
	delete[] b;
	delete[] cpu_result;

	return 0;
}
