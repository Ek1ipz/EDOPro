#include "core_utils.h"
#include "bufferio.h"
#include "dllinterface.h"
#include <common.h>

namespace CoreUtils {

#define PARSE_SINGLE(query,value) case query: {\
value = BufferIO::Read<decltype(value)>(current);\
break;\
}

void Query::Parse(char*& current) {
	//char* current = buff;
	flag = 0;
	for(;;) {
		uint16 size = BufferIO::Read<uint16>(current);
		if(size == 0) {
			onfield_skipped = true;
			return;
		}
		uint32_t _flag = BufferIO::Read<uint32_t>(current);
		flag |= _flag;
		switch(_flag) {
			PARSE_SINGLE(QUERY_CODE,code)
			PARSE_SINGLE(QUERY_POSITION, position)
			PARSE_SINGLE(QUERY_ALIAS, alias)
			PARSE_SINGLE(QUERY_TYPE, type)
			PARSE_SINGLE(QUERY_LEVEL, level)
			PARSE_SINGLE(QUERY_RANK, rank)
			PARSE_SINGLE(QUERY_ATTRIBUTE, attribute)
			PARSE_SINGLE(QUERY_RACE, race)
			PARSE_SINGLE(QUERY_ATTACK, attack)
			PARSE_SINGLE(QUERY_DEFENSE, defense)
			PARSE_SINGLE(QUERY_BASE_ATTACK, base_attack)
			PARSE_SINGLE(QUERY_BASE_DEFENSE, base_defense)
			PARSE_SINGLE(QUERY_REASON, reason)
			PARSE_SINGLE(QUERY_OWNER, owner)
			PARSE_SINGLE(QUERY_STATUS, status)
			PARSE_SINGLE(QUERY_IS_PUBLIC, is_public)
			PARSE_SINGLE(QUERY_LSCALE, lscale)
			PARSE_SINGLE(QUERY_RSCALE, rscale)
			case QUERY_REASON_CARD: {
				reason_card = ReadLocInfo(current);
				break;
			}
			case QUERY_EQUIP_CARD: {
				equip_card = ReadLocInfo(current);
				break;
			}
			case QUERY_TARGET_CARD: {
				uint32_t count = BufferIO::Read<uint32_t>(current);
				for(uint32_t i = 0; i < count; i++)
					target_cards.push_back(ReadLocInfo(current));
				break;
			}
			case QUERY_OVERLAY_CARD: {
				uint32_t count = BufferIO::Read<uint32_t>(current);
				for(uint32_t i = 0; i < count; i++)
					overlay_cards.push_back(BufferIO::Read<uint32_t>(current));
				break;
			}
			case QUERY_COUNTERS: {
				uint32_t count = BufferIO::Read<uint32_t>(current);
				for(uint32_t i = 0; i < count; i++)
					counters.push_back(BufferIO::Read<uint32_t>(current));
				break;
			}
			case QUERY_LINK: {
				link = BufferIO::Read<uint32_t>(current);
				link_marker = BufferIO::Read<uint32_t>(current);
				break;
			}
			case QUERY_END: {
				return;
			}
		}
	}
}

template<typename T>
void insert_value(std::vector<uint8_t>& vec, T val) {
	const auto vec_size = vec.size();
	const auto val_size = sizeof(T);
	vec.resize(vec_size + val_size);
	std::memcpy(&vec[vec_size], &val, val_size);
}

void Query::GenerateBuffer(std::vector<uint8_t>& buffer, bool is_public) {
	if(onfield_skipped) {
		insert_value<uint16_t>(buffer, 0);
		return;
	}
	for(uint64_t _flag = 1; _flag <= QUERY_END; _flag <<= 1) {
		if((this->flag & _flag) != _flag)
			continue;
		insert_value<uint16_t>(buffer, GetSize(_flag) + sizeof(uint32_t));
		insert_value<uint32>(buffer, _flag);
		if(is_public && !IsPublicQuery(_flag)) {
			const auto vec_size = buffer.size();
			const auto flag_size = GetSize(_flag);
			buffer.resize(vec_size + flag_size);
			std::memset(&buffer[vec_size], 0, flag_size);
			continue;
		}
		if(_flag == QUERY_CODE) insert_value<uint32>(buffer, code);
		if(_flag == QUERY_POSITION) insert_value<uint32>(buffer, position);
		if(_flag == QUERY_ALIAS) insert_value<uint32>(buffer, alias);
		if(_flag == QUERY_TYPE) insert_value<uint32>(buffer, type);
		if(_flag == QUERY_LEVEL) insert_value<uint32>(buffer, level);
		if(_flag == QUERY_RANK) insert_value<uint32>(buffer, rank);
		if(_flag == QUERY_ATTRIBUTE) insert_value<uint32>(buffer, attribute);
		if(_flag == QUERY_RACE) insert_value<uint32>(buffer, race);
		if(_flag == QUERY_ATTACK) insert_value<uint32>(buffer, attack);
		if(_flag == QUERY_DEFENSE) insert_value<uint32>(buffer, defense);
		if(_flag == QUERY_BASE_ATTACK) insert_value<uint32>(buffer, base_attack);
		if(_flag == QUERY_BASE_DEFENSE) insert_value<uint32>(buffer, base_defense);
		if(_flag == QUERY_REASON) insert_value<uint32>(buffer, reason);
		if(_flag == QUERY_OWNER) insert_value<uint32>(buffer, owner);
		if(_flag == QUERY_STATUS) insert_value<uint32>(buffer, status);
		if(_flag == QUERY_IS_PUBLIC) insert_value<uint32>(buffer, is_public);
		if(_flag == QUERY_LSCALE) insert_value<uint32>(buffer, lscale);
		if(_flag == QUERY_RSCALE) insert_value<uint32>(buffer, rscale);
		if(_flag == QUERY_REASON_CARD || _flag == QUERY_EQUIP_CARD) {
			auto& info = (_flag == QUERY_REASON_CARD) ? reason_card : equip_card;
			insert_value<uint8>(buffer, info.controler);
			insert_value<uint8>(buffer, info.location);
			insert_value<uint32>(buffer, info.sequence);
			insert_value<uint32>(buffer, info.position);
		}
		if(_flag == QUERY_TARGET_CARD) {
			insert_value<uint32>(buffer, target_cards.size());
			for(auto& info : target_cards) {
				insert_value<uint8>(buffer, info.controler);
				insert_value<uint8>(buffer, info.location);
				insert_value<uint32>(buffer, info.sequence);
				insert_value<uint32>(buffer, info.position);
			}
		}
		if(_flag == QUERY_OVERLAY_CARD || _flag == QUERY_COUNTERS) {
			auto& vec = (_flag == QUERY_OVERLAY_CARD) ? overlay_cards : counters;
			insert_value<uint32>(buffer, vec.size());
			for(auto& val : vec) {
				insert_value<uint32>(buffer, val);
			}
		}
		if(_flag == QUERY_LINK) {
			insert_value<uint32>(buffer, link);
			insert_value<uint32>(buffer, link_marker);
		}
	}
}

bool Query::IsPublicQuery(uint32_t flag) {
	if(is_public || position & POS_FACEUP)
		return true;
	switch(flag) {
	case QUERY_CODE:
	case QUERY_ALIAS:
	case QUERY_TYPE:
	case QUERY_LEVEL:
	case QUERY_RANK:
	case QUERY_ATTRIBUTE:
	case QUERY_RACE:
	case QUERY_ATTACK:
	case QUERY_DEFENSE:
	case QUERY_BASE_ATTACK:
	case QUERY_BASE_DEFENSE:
	case QUERY_STATUS:
	case QUERY_LSCALE:
	case QUERY_RSCALE:
	case QUERY_LINK:
		return false;
	}
	return true;
}

uint32_t Query::GetSize(uint32_t flag) {
	switch(flag) {
	case QUERY_CODE:
	case QUERY_POSITION:
	case QUERY_ALIAS:
	case QUERY_TYPE:
	case QUERY_LEVEL:
	case QUERY_RANK:
	case QUERY_ATTRIBUTE:
	case QUERY_RACE:
	case QUERY_ATTACK:
	case QUERY_DEFENSE:
	case QUERY_BASE_ATTACK:
	case QUERY_BASE_DEFENSE:
	case QUERY_REASON:
	case QUERY_OWNER:
	case QUERY_STATUS:
	case QUERY_IS_PUBLIC:
	case QUERY_LSCALE:
	case QUERY_RSCALE: {
		return sizeof(uint32_t);
		break;
	}
	case QUERY_REASON_CARD:
	case QUERY_EQUIP_CARD: {
		return sizeof(uint16) + sizeof(uint64);
		break;
	}
	case QUERY_TARGET_CARD: {
		return sizeof(uint32) + (target_cards.size() * (sizeof(uint16) + sizeof(uint64)));
		break;
	}
	case QUERY_OVERLAY_CARD: {
		return sizeof(uint32) + (overlay_cards.size() * sizeof(uint32));
		break;
	}
	case QUERY_COUNTERS: {
		return sizeof(uint32) + (counters.size() * sizeof(uint32));
		break;
	}
	case QUERY_LINK: {
		return sizeof(uint32) + sizeof(uint32);
		break;
	}
	case QUERY_END:
		return 0;
		break;
	}
	return 0;
}

uint32_t Query::GetSize() {
	uint32_t size = 0;
	if(onfield_skipped)
		return 0;
	for(uint64_t _flag = 1; _flag <= QUERY_END; _flag <<= 1) {
		if((this->flag & _flag) != _flag)
			continue;
		size += GetSize(_flag) + sizeof(uint32_t);
	}
	return size;
}

loc_info ReadLocInfo(char*& p, bool compat) {
	loc_info info;
	info.controler = BufferIO::Read<uint8_t>(p);
	info.location = BufferIO::Read<uint8_t>(p);
	if(compat) {
		info.sequence = BufferIO::Read<uint8_t>(p);
		info.position = BufferIO::Read<uint8_t>(p);
	} else {
		info.sequence = BufferIO::Read<int32_t>(p);
		info.position = BufferIO::Read<int32_t>(p);
	}
	return info;
}

CoreUtils::PacketStream ParseMessages(OCG_Duel duel) {
	uint32_t message_len;
	auto msg = OCG_DuelGetMessage(duel, &message_len);
	if(message_len)
		return PacketStream((char*)msg, message_len);
	else
		return PacketStream();
}

bool MessageBeRecorded(uint32_t message) {
	switch(message) {
	case MSG_HINT:
	case MSG_SELECT_BATTLECMD:
	case MSG_SELECT_IDLECMD:
	case MSG_SELECT_EFFECTYN:
	case MSG_SELECT_YESNO:
	case MSG_SELECT_OPTION:
	case MSG_SELECT_CARD:
	case MSG_SELECT_TRIBUTE:
	case MSG_SELECT_UNSELECT_CARD:
	case MSG_SELECT_CHAIN:
	case MSG_SELECT_PLACE:
	case MSG_SELECT_DISFIELD:
	case MSG_SELECT_POSITION:
	case MSG_SELECT_COUNTER:
	case MSG_SELECT_SUM:
	case MSG_SORT_CARD:
	case MSG_SORT_CHAIN:
	case MSG_ROCK_PAPER_SCISSORS:
	case MSG_ANNOUNCE_RACE:
	case MSG_ANNOUNCE_ATTRIB:
	case MSG_ANNOUNCE_CARD:
	case MSG_ANNOUNCE_NUMBER:
		return false;
	}
	return true;
}

void QueryStream::Parse(char*& buff) {
	uint32_t size = BufferIO::Read<uint32_t>(buff);
	char* current = buff;
	while((current - buff) < size) {
		queries.emplace_back(current);
	}
}

void QueryStream::GenerateBuffer(std::vector<uint8_t>& buffer) {
	std::vector<uint8_t> tmp_buffer;
	for(auto& query : queries) {
		query.GenerateBuffer(tmp_buffer, false);
	}
	insert_value<uint32_t>(buffer, tmp_buffer.size());
	buffer.insert(buffer.end(), tmp_buffer.begin(), tmp_buffer.end());
}

void QueryStream::GeneratePublicBuffer(std::vector<uint8_t>& buffer) {
	std::vector<uint8_t> tmp_buffer;
	for(auto& query : queries) {
		query.GenerateBuffer(tmp_buffer, true);
	}
	insert_value<uint32_t>(buffer, tmp_buffer.size());
	buffer.insert(buffer.end(), tmp_buffer.begin(), tmp_buffer.end());
}

PacketStream::PacketStream(char* buff, int len) {
	char* current = buff;
	while((current - buff) < len) {
		uint32_t size = BufferIO::Read<uint32_t>(current);
		packets.emplace_back(current, size - sizeof(uint8_t));
		current += size;
	}
}

}
