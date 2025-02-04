
#include "ServerGame.h"

unsigned int ServerGame::client_id; 

ServerGame::ServerGame(void)
{
    // id's to assign clients for our table
    client_id = 0;

    // set up the server network to listen 
    network = new ServerNetwork(); 
}

ServerGame::~ServerGame(void)
{
}

void ServerGame::update()
{
    // get new clients
   if(network->acceptNewClient(client_id))
   {
        printf("client %d has been connected to the server\n",client_id);

        client_id++;
   }

   receiveFromClients();
}

void ServerGame::receiveFromClients()
{

    Packet packet;

    // go through all clients
    std::map<unsigned int, SOCKET>::iterator iter;

    for(iter = network->sessions.begin(); iter != network->sessions.end(); iter++)
    {
        int data_length = network->receiveData(iter->first, network_data);

        if (data_length <= 0) 
        {
            //no data recieved
            continue;
        }

        int i = 0;
        while (i < (unsigned int)data_length) 
        {
            packet.deserialize(&(network_data[i]));
            i += sizeof(Packet);

            switch (packet.packet_type) {

                case INIT_CONNECTION:

                    printf("server received init packet from client\n");

                    sendActionPackets();

                    break;

                case ACTION_EVENT:

                    //printf("server received action event packet from client\n");
					if (packet.attack.first != -1) {
						other_attack = packet.attack;
						game_mode = true;
					}

					if (packet.damage.first != -1) {
						other_damage = packet.damage;
					}

					other_done = packet.done;
					other_headPose = packet.headPose;
                    sendActionPackets();

                    break;

                default:

                    printf("error in packet types\n");

                    break;
            }
        }
    }
}


void ServerGame::sendActionPackets()
{
    // send action packet
    const unsigned int packet_size = sizeof(Packet);
    char packet_data[packet_size];

    Packet packet;
    packet.packet_type = ACTION_EVENT;
	packet.attack = my_attack;
	packet.damage = my_damage;
	packet.done = my_done;
	packet.headPose = my_headPose;

	my_attack.first = -1;
	my_attack.second = -1;
	my_damage.first = -1;
	my_damage.second = -1;

    packet.serialize(packet_data);

    network->sendToAll(packet_data,packet_size);
}