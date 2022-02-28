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
#include <iostream>
#include "rdt_struct.h"
#include "rdt_sender.h"

#define WINDOW_SIZE 10
#define TIME_OUT 1
int base,nextseqnum;
//use to store the message from upper(transform to packet,since a message may form more than one packet)
std::queue<packet> buffer; 
packet* window = nullptr;
// std::thread* background_send = nullptr;
/* sender initialization, called once at the very beginning */
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
    memcpy(p, &checksum, sizeof(checksum));
}


void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
    if(!window){
        window = new packet[WINDOW_SIZE];
    }
    base = 1;
    nextseqnum = 1;
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
    if(window){
        delete window;
        window = nullptr;
    }
    //std::cout << "base = "<<base<<"nextseqnum = "<<nextseqnum<<" buffer size = "<<buffer.size()<<std::endl;
}


/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    int maxpayload_size = 118;
    //fill the buffer
    //if(lockDebug) std::cout<<"get lock at "<<GetSimulationTime()<<std::endl;
    int num = msg->size / maxpayload_size;
    num = (msg->size % maxpayload_size) == 0 ? num : num + 1;
    char* pos = msg->data;
    int offset = 0;
    for(int i = 0; i < num; i++){
        if(i == num -1){
            //the last one
            packet pac = make_pkt1(pos, true, msg->size - offset);
            buffer.push(pac);
        }
        else{
            packet pac = make_pkt1(pos, false);
            offset += 118;
            pos += 118;
            buffer.push(pac);
        }
    }
    //if(lockDebug) std::cout<<"release lock at "<<GetSimulationTime()<<std::endl;
    //send message
    while(nextseqnum < base + WINDOW_SIZE && !buffer.empty()){
            //get the message
            packet pac = buffer.front();
            buffer.pop();
            int place = nextseqnum % WINDOW_SIZE;
            //fill the information of nextseqnum and checksum
            make_pkt2(nextseqnum,pac);
            window[place] = pac;
            //std::cout<<"send message at time "<<GetSimulationTime()<<std::endl;
            Sender_ToLowerLayer(window + place);
            if(base == nextseqnum) Sender_StartTimer(TIME_OUT);
            nextseqnum++;
    }
    
    
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
bool notCorrupt(packet pac){
    unsigned char checkResult = 0;
    unsigned char *pos = (unsigned char*)pac.data;
    for(int i = 0; i < 6; i++){
        checkResult += pos[i];
    }
    if(!checkResult){
        return true;
    }
    return false;
}
int getAckNum(packet pac){
    int ackNum;
    char* p = pac.data;
    memcpy(&ackNum, p, sizeof(int));
    return ackNum;
}
/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
    //first copy the packet
    //if(lockDebug) std::cout<<"get lock at "<<GetSimulationTime()<<std::endl;
    packet pac;
    char *dst = pac.data;
    char *src = pkt->data;
    memcpy(dst, src, 6);
    if(notCorrupt(pac)){
        int seqnum = getAckNum(pac);
        //std::cout<<"the recive ACK not corrupt, seqnum = "<<seqnum<<" base="<<base<<std::endl;
        if(seqnum < base - WINDOW_SIZE || seqnum > base + WINDOW_SIZE) return;
        int oldBase = base; 
        base = seqnum + 1;
        if(base == nextseqnum){
            Sender_StopTimer();
            while(nextseqnum < base + WINDOW_SIZE && !buffer.empty()){
                //get the message
                packet pac = buffer.front();
                buffer.pop();
                int place = nextseqnum % WINDOW_SIZE;
                //fill the information of nextseqnum and checksum
                make_pkt2(nextseqnum,pac);
                window[place] = pac;
                //std::cout<<"send message at time "<<GetSimulationTime()<<std::endl;
                Sender_ToLowerLayer(window + place);
                if(base == nextseqnum) Sender_StartTimer(TIME_OUT);
                nextseqnum++;
            }
        }else{
            if(oldBase != base){
                Sender_StartTimer(TIME_OUT);
            }
        }
    }else{
       //std::cout<<"the recive ACK corrupt"<<std::endl;
    }
    //if(lockDebug) std::cout<<"release lock at "<<GetSimulationTime()<<std::endl;
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
    //if(lockDebug) std::cout<<"get lock at "<<GetSimulationTime()<<std::endl;
    //std::cout<<"time out "<<"base = "<<base<<"nextseqnum = "<<nextseqnum<<std::endl;
    Sender_StartTimer(TIME_OUT);
    for(int i = base; i < nextseqnum; i++){
        int offset = i % WINDOW_SIZE;
        Sender_ToLowerLayer(window + offset);
    }
    //if(lockDebug) std::cout<<"release lock at "<<GetSimulationTime()<<std::endl;
}

