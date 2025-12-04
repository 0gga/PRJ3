#pragma once
#include <memory>
#include <string>

struct FilePayload {
	std::string type;
	std::string filePath;
};

template<typename T>
struct typeName {
	static constexpr std::string_view type = "type:unknown";
};

template<>
struct typeName<std::string> {
	static constexpr std::string_view type = "type:string";
};

template<>
struct typeName<FilePayload> {
	static constexpr std::string_view type = "type:file";
};

template<typename T>
std::shared_ptr<std::string> encodePayload(const T& data) {
	auto buffer = std::make_shared<std::string>();
	buffer->reserve(64 + sizeof(T));

	buffer->append(typeName<T>::type);
	buffer->append("%%%");
	buffer->append(data);
	buffer->push_back('\n');

	return buffer;
}