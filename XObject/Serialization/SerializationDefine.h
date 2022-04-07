#ifndef SerializationDefine_h__
#define SerializationDefine_h__

#include "bitsery/bitsery.h"
#include "bitsery/adapter/buffer.h"
#include "nlohmann/json.hpp"

using SerializeBuffer = std::vector<uint8_t>;
using OutputAdapter = bitsery::OutputBufferAdapter<SerializeBuffer>;
using InputAdapter = bitsery::InputBufferAdapter<SerializeBuffer>;
using Serializer = bitsery::Serializer<OutputAdapter>;
using Deserializer = bitsery::Deserializer<InputAdapter>;

#define X_JSON_TO(v1) nlohmann_json_j[#v1] = v1;

#define X_DEFINE_TYPE_INTRUSIVE(...)  \
    void __intrusive_to_json(nlohmann::json& nlohmann_json_j) { NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(X_JSON_TO, __VA_ARGS__)) }

#define X_SERIALIZE_BIND(...) \
	template<typename S> \
	void serialize(S& s) { s(__VA_ARGS__); } \
	X_DEFINE_TYPE_INTRUSIVE(__VA_ARGS__)

template <typename OStream>
OStream& operator<<(OStream& ostream, const SerializeBuffer& buffer) {
	ostream << buffer.size();
	ostream.write((const char*)buffer.data(), buffer.size());
	return ostream;
}

template <typename IStream>
IStream& operator>>(IStream& istream, SerializeBuffer& buffer) {
	size_t size;
	istream >> size;
	buffer.resize(size);
	istream.read((char*)buffer.data(), size);
	return istream;
}

#endif // SerializationDefine_h__
