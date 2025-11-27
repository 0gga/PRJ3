#pragma once

template<typename T>
struct TypeId;

struct FilePath {
	std::filesystem::path path;
};

template<>
struct TypeId<std::string> {
	static constexpr uint16_t value = 1;
};

template<>
struct TypeId<nlohmann::json> {
	static constexpr uint16_t value = 2;
};

template<>
struct TypeId<FilePath> {
	static constexpr uint16_t value = 3;
};


template<typename T>
struct Encoder;

template<>
struct Encoder<std::string> {
	static size_t payloadSize(const std::string& in) {
		return in.size();
	}

	static void encode(const std::string& in, char* out) {
		std::memcpy(out, in.data(), in.size());
	}
};

template<>
struct Encoder<nlohmann::json> {
	static size_t payloadSize(const nlohmann::json& in) {
		return in.dump().size();
	}

	static void encode(const nlohmann::json& in, char* out) {
		const std::string dump = in.dump();
		std::memcpy(out, dump.data(), dump.size());
	}
};

template<>
struct Encoder<FilePath> {
	static size_t payloadSize(const FilePath& in) {
		std::ifstream file(in.path, std::ios::binary);
		if (!file)
			return 0;

		file.seekg(0, std::ios::end);
		return file.tellg();
	}

	static void encode(const FilePath& in, char* out, size_t size) {
		std::ifstream file(in.path, std::ios::binary);
		file.read(out, static_cast<std::streamsize>(size));
	}
};

template<typename T>
std::shared_ptr<std::string> encodePayload(const T& data) {
	const size_t size = Encoder<T>::payloadSize(data);

	const uint32_t len    = htonl(static_cast<uint32_t>(size));
	const uint16_t typeId = htons(TypeId<T>::value);

	std::shared_ptr<std::string> bytes = std::make_shared<std::string>();
	bytes->resize(sizeof(typeId) + sizeof(len) + len);

	char* ptr = bytes->data();

	std::memcpy(ptr, &typeId, sizeof(typeId));
	ptr += sizeof(typeId);

	std::memcpy(ptr, &len, sizeof(len));
	ptr += sizeof(len);

	if constexpr(std::is_same_v<T, FilePath>)
		Encoder<FilePath>::encode(data, ptr, size);
	else
		Encoder<T>::encode(data, ptr);

	return bytes;
}
