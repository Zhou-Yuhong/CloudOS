/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <queue>
#include "rdt_struct.h"
#include "rdt_receiver.h"

int expectedseqnum;
packet sndpkt;
std::vector<packet> buffer; //buffer to store packets that can form a message
/* receiver initialization, called once at the very beginning */
bool notCorrupt(packet pac){
    //check if the pac reciver is complete based on checksum
    unsigned char checkResult = 0;
    unsigned char *pos = (unsigned char *)pac.data;
    for(int i = 0; i < 128; i++){
        checkResult += pos[i];
    }
    if(!checkResult){
        return true;
    }
    return false;
}

void decoding(packet pac, int &seqnum, bool &isEnd){
    char *p = pac.data;
    memcpy(&seqnum, p, sizeof(seqnum));
    p += sizeof(seqnum);
    memcpy(&isEnd, p, sizeof(isEnd));
}
int getDataLength(packet pac){
    char *p = pac.data;
    int result;
    p += 5;
    memcpy(&result, p, sizeof(int));
    return result;
}
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
    expectedseqnum = 1;
    make_pkt(sndpkt, 0);

}
message formMsg(){
    //form a message from the buffer
    message msg;
    //first calculate the totle size of message
    int size = 0;
    std::queue<int> lenGroup;
    for(auto it:buffer){
        int tmp = getDataLength(it);
        lenGroup.push(tmp);
        size += tmp;
    }
    //new space for data
    msg.size = size;
    msg.data = new char[size];
    char *p = msg.data;
    for(auto it:buffer){
        int tmp = lenGroup.front();
        lenGroup.pop();
        char *pos = it.data;
        pos += 9;
        memcpy(p, pos, tmp);
        p += tmp;
    } 
    //clear buffer
    buffer.clear();
    return msg;
}
/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    //make a copy of this pkt
    packet pac;
    char* dst = pac.data;
    char* src = pkt->data;
    memcpy(dst, src, 128);
    if(notCorrupt(pac)){
        int seqnum;
        bool isEnd;
        decoding(pac, seqnum, isEnd);
        if(seqnum == expectedseqnum){
            buffer.push_back(pac);
            if(isEnd){
               message msg = formMsg();
               Receiver_ToUpperLayer(&msg); 
               if(msg.data){
                   delete msg.data;
                   msg.data = nullptr;
               }
            }
            make_pkt(sndpkt, expectedseqnum);
            Receiver_ToLowerLayer(&sndpkt);
            expectedseqnum++;
        }else{
            Receiver_ToLowerLayer(&sndpkt);
        }
    }else{
        Receiver_ToLowerLayer(&sndpkt);
    }
    // /* 1-byte header indicating the size of the payload */
    // int header_size = 1;

    // /* construct a message and deliver to the upper layer */
    // struct message *msg = (struct message*) malloc(sizeof(struct message));
    // ASSERT(msg!=NULL);

    // msg->size = pkt->data[0];

    // /* sanity check in case the packet is corrupted */
    // if (msg->size<0) msg->size=0;
    // if (msg->size>RDT_PKTSIZE-header_size) msg->size=RDT_PKTSIZE-header_size;

    // msg->data = (char*) malloc(msg->size);
    // ASSERT(msg->data!=NULL);
    // memcpy(msg->data, pkt->data+header_size, msg->size);
    // Receiver_ToUpperLayer(msg);

    // /* don't forget to free the space */
    // if (msg->data!=NULL) free(msg->data);
    // if (msg!=NULL) free(msg);
}
void make_pkt(packet &pac,int expectedseqnum,bool ACK = true){
    // the struct of pac sent by receiver side is
    // int expectedseqnum | bool ACK|char checksum
    char* pos = pac.data;
    memccpy(pos, &expectedseqnum, sizeof(expectedseqnum));
    pos += sizeof(expectedseqnum);
    memcpy(pos, &ACK, sizeof(ACK));
    pos += sizeof(ACK);
    //form the checksum
    unsigned char* p = (unsigned char*)(pac.data);
    unsigned char checksum = 0;
    for(int i = 0; i < 5; i++){
        checksum += p[i];
    }
    checksum = ~checksum;
    checksum++;
    p += 5;
    memcpy(p, &checksum, sizeof(checksum));

}
