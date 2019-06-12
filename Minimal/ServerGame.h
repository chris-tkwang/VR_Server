#pragma once
#include "ServerNetwork.h"
#include "NetworkData.h"

class ServerGame
{

public:

    ServerGame(void);
    ~ServerGame(void);

    void update();

	void receiveFromClients();

	void sendActionPackets();

	std::pair<int, int> my_attack = std::make_pair(-1,-1);
	std::pair<int, int> other_attack = std::make_pair(-1, -1);
	std::pair<int, int> my_damage = std::make_pair(-1, -1);
	std::pair<int, int> other_damage = std::make_pair(-1, -1);
	glm::mat4 my_headPose = glm::mat4(1.0f);
	glm::mat4 other_headPose = glm::mat4(1.0f);

	bool game_mode = true;
	bool my_done = false;
	bool other_done = false;

private:

   // IDs for the clients connecting for table in ServerNetwork 
    static unsigned int client_id;

   // The ServerNetwork object 
    ServerNetwork* network;

	// data buffer
   char network_data[MAX_PACKET_SIZE];
};