#include<iostream>
#include<fstream>
#include<filesystem>
namespace FileLoader {
inline std::vector<char>LoadShaderfile(const std::string& fn) {
		//ate: start reading at the end of the file
		//     the advantage of starting to read at the end of the file is that 
		//     we can use the read positoin to determine the size of the file and allocate a buffer
		//binary: read the file as binary file
		std::ifstream file(fn, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}
		std::filesystem::path p = fn;
		size_t fileSize = (size_t)file.tellg();
		std::cout << "File name: " << p.filename() << "\tFile Size: " << fileSize << std::endl;
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		return buffer;
	}
}