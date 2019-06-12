#pragma once
#include <string.h>
#include <glm/glm.hpp>

#define MAX_PACKET_SIZE 1000000

enum PacketTypes {

    INIT_CONNECTION = 0,

    ACTION_EVENT = 1,

};

struct Packet {

    unsigned int packet_type;
	std::pair<int, int> attack;
	std::pair<int, int> damage;
	bool done;
	glm::mat4 headPose;

    void serialize(char * data) {
        memcpy(data, this, sizeof(Packet));
    }

    void deserialize(char * data) {
        memcpy(this, data, sizeof(Packet));
    }
};