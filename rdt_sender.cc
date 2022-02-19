/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <thread>
#include "rdt_struct.h"
#include "rdt_sender.h"

#define WINDOW_SIZE 16
#define TIME_OUT 1
int base,nextseqnum;
//use to store the message from upper(transform to packet,since a message may form more than one packet)
std::queue<packet> buffer; 
packet* window = nullptr;
std::mutex mtx;
std::thread* background_send = nullptr;
/* sender initialization, called once at the very beginning */

void sendMsg(){
    while(true){
        mtx.lock();
        while(nextseqnum < base + WINDOW_SIZE && !buffer.empty()){
            //get the message
            packet pac = buffer.front();
            buffer.pop();
            int place = nextseqnum % WINDOW_SIZE;
            window[place] = pac;
            Sender_ToLowerLayer(window + place);
            if(base == nextseqnum) Sender_StartTimer(TIME_OUT);
            nextseqnum++;
        }
        mtx.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }   
}

void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
    if(!window){
        window = new packet[WINDOW_SIZE];
    }
    base = 1;
    nextseqnum = 1;
    background_send = new std::thread(&sendMsg);

}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}


/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{

    
    int maxpayload_size = 118;
    //fill the buffer
    mtx.lock();
    int num = msg->size / maxpayload_size;
    num = (msg->size % maxpayload_size) == 0 ? num : num + 1;
    for(int i = 0; i < num; i++){

    }
    mtx.unlock();
    
    
    // /* split the message if it is too big */

    // /* reuse the same packet data structure */
    // packet pkt;

    // /* the cursor always points to the first unsent byte in the message */
    // int cursor = 0;

    // while (msg->size-cursor > maxpayload_size) {
	// /* fill in the packet */
	// pkt.data[0] = maxpayload_size;
	// memcpy(pkt.data+header_size, msg->data+cursor, maxpayload_size);

	// /* send it out through the lower layer */
	// Sender_ToLowerLayer(&pkt);

	// /* move the cursor */
	// cursor += maxpayload_size;
    // }

    // /* send out the last packet */
    // if (msg->size > cursor) {
	// /* fill in the packet */
	// pkt.data[0] = msg->size-cursor;
	// memcpy(pkt.data+header_size, msg->data+cursor, pkt.data[0]);

	// /* send it out through the lower layer */
	// Sender_ToLowerLayer(&pkt);
    // }
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
}

//the struct of packet is:
// nextseqnum|isEnd|length of data|data|checksum
// the form of pkt should divide into 2 steps, first is split msg into multi pkt, second is fill the nextseqnum, checksum
packet make_pkt1(char* data, bool isEnd = true,int length = 118){
    //first caculate the checksum
    // unsigned char checksum = 0;
    // unsigned char* pos = (unsigned char*)data;
    // for(int i=0;i<length;i++){
    //     checksum += pos[i];
    // }
    // checksum = ~checksum;
    // checksum++;

    //fill the packet
    packet result;
    char* p = (char*)(result.data);
    // memcpy(p, (char*)(&nextseqnum), sizeof(int));
    p += sizeof(int);
    memcpy(p, &isEnd, sizeof(bool));
    p += sizeof(bool);
    memcpy(p, &length, sizeof(length));
    p += sizeof(length);
    memcpy(p, data, length);
    return result;
}
void make_pkt2(int nextseqnum, packet& pac){
    //form the checksum
    char* p = (char*)(pac.data);
    memcpy(p, &nextseqnum, sizeof(int));
    //form the checksum
    unsigned char* pos = (unsigned char*)(pac.data);
    unsigned char checksum = 0;
    for(int i = 0; i < 127; i++){
        checksum += pos[i];
    }
    checksum = ~checksum;
    checksum++;
    p += 127;
    memcpy(p, checksum, sizeof(checksum));
}