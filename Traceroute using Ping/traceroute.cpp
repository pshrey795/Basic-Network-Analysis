//This file has been created by Shrey J. Patel 
//Traceroute implementation in C++ using sockets and ICMP packets

#include<bits/stdc++.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/ip_icmp.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<chrono>
#include<unistd.h>
using namespace std;
using namespace chrono;

#define PKT_SIZE 64 //Packet Size
#define PORT_NO 0   //Random port number, although inbuilt ping commands don't use any port for sending ICMP packets

//ICMP packet = 8 byte header + 56 byte message
struct ICMP_packet{
    struct icmphdr header;
    char message[PKT_SIZE-sizeof(struct icmphdr)];
};

//Custom data structure to store domain info: IP address, Domain Name, address type, etc.
struct domain_info{
    char* name;
    char* ip_addr;
    struct hostent* domain; 

    domain_info(char* name,char* ip_addr, struct hostent* domain) : name(name), ip_addr(ip_addr), domain(domain) {}
};

//Function to obtain entire host info from IP address using gethostbyname() using netdb
struct hostent *resolveDNS(char* domain_name){
    return gethostbyname(domain_name);
}

//Checksum calculation to avoid the contamination of the packet
//No significance as such in my code for tracing route, but used for debugging
unsigned short calculateCheckSum(void* pkt,int length){
    unsigned short *acc = (unsigned short*)pkt;
	unsigned int sum=0;

    //16-bit addition loop
	while(length>1){
        sum += *acc;
        *acc++;
        length-=2;
    }	
	if(length==1){
        sum += *(unsigned char*)acc;
    }

    //Addition of carry	
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);

    //Checksum = 1s complement of the final sum
    unsigned short result = ~sum;
	return result;
}

//Main ping function which takes in destination address, TTL value and the socket descriptor for the socket 
//through which the packet will be sent
bool ping(int sock_desc, struct sockaddr_in *send_addr, int ttl_val){

    //Socket settings for TTL value
    setsockopt(sock_desc, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val));

    //Initialising the send and receive packets
    struct ICMP_packet send_pkt;
    struct ICMP_packet rcv_pkt;

    //Filling the sending packet. Note that the contents of the message don't matter in this case
    
    //This function initialises all the bytes of the packet to zero
    bzero(&send_pkt,sizeof(send_pkt));

    //Filling contents of the header
    send_pkt.header.type = ICMP_ECHO;            //type 8 for ICMP echo requests in ping
    send_pkt.header.un.echo.id = getpid();       //ID of the current process to ensure synchronisation between send and receive
    send_pkt.header.un.echo.sequence = 1;           
    send_pkt.header.checksum = calculateCheckSum(&send_pkt,sizeof(send_pkt)); //Attaching checksum for fault detection

    struct sockaddr_in rcv_addr;                
    socklen_t rcv_addr_len = sizeof(rcv_addr);
    int send_success,rcv_success;

    //Actual packet transfer starts here
    auto start = high_resolution_clock::now();      //Start clock

    send_success = sendto(sock_desc,&send_pkt,sizeof(send_pkt),0,(const sockaddr *)send_addr,sizeof(*send_addr));
    rcv_success = recvfrom(sock_desc,&rcv_pkt,sizeof(rcv_pkt),0,(sockaddr *)&rcv_addr,&rcv_addr_len);

    auto stop = high_resolution_clock::now();       //End clock
    auto rtt = duration_cast<microseconds>(stop-start);     //Net round trip time

    //Printing the route
    cout<<ttl_val<<",";
    if(rcv_success<0){
        //If no packet was received before the timeout, then the round trip time is to taken zero and no IP address
        //is printed
        cout<<"*,0\n";                                
    }else{
        //If packet received successfully, then print the IP address of the router that sent the ICMP error and
        //also the net round trip time
        cout<<inet_ntoa(rcv_addr.sin_addr)<<","<<rtt.count()<<"\n";   
    }

    //Because the ping function is executed hop-by-hop with the length of hop increasing by 1, we will eventually 
    //arrive at the destination(unless TTL value reaches 256), in which case we are done tracing the route, and the 
    //ping loop should terminate. However it is not sufficient to just compare the IP addresses as the IP address
    //of an intermediate router/gateway can also be equal to the destination IP, so to actually determine the 
    //termination, we need to check if the TTL is just sufficient i.e. TTL not exceeded for the first time.

    //This can be checked from the type code of the reply stored in the header of received packet which is usually
    //stored in the 20th byte of the packet.
    //Header type code = 0  -> TTL not exceeded
    //Header type code = 11 -> TTL exceeded
    return *((char*)&rcv_pkt+20);
}

int main(int argc,char* argv[]){

    char* domain_name;

    if(argc<2){
        cout<<"Please provide domain name: ";cin>>domain_name;
    }else{
        domain_name = argv[1];
    }

    //Obtaining host info from IP
    struct hostent *domain_IP = resolveDNS(domain_name); 

    //Initialising a socket which sends/receives ICMP packets
    int sockfd = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);

    //Debugging
    if(sockfd<0){
        cout<<sockfd<<"\n";
    }

    //Extracting domain IP address as a string but stored as an in_addr in host entity 
    char* ip_addr = inet_ntoa(*(struct in_addr *)domain_IP->h_addr);
    //cout<<"Resolved IP address: "<<ip_addr<<"\n";

    //Headings for plotting
    cout<<"Hop,IP,RTT\n";

    //Deafult timeout value for ping
    double timeout = 3.0;

    //Extracting destination address as a sockaddr_in from the host_entity
    struct domain_info stats(domain_name,ip_addr,domain_IP);
    struct domain_info *domain = &stats;
    struct sockaddr_in *send_addr = (struct sockaddr_in*)malloc(sizeof(sockaddr_in));
    (*send_addr).sin_family = domain->domain->h_addrtype;
    (*send_addr).sin_port = htons (PORT_NO);
    (*send_addr).sin_addr.s_addr = *(long *)(domain->domain->h_addr);

    //Setting timeout values as a timeval struct to be used for socket settings
    struct timeval rt_out;
    rt_out.tv_sec = (int)timeout;
    rt_out.tv_usec = (int)(1000000.0*(timeout-rt_out.tv_sec));

    //Setting timeout value of the socket for receiving packets
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &rt_out, sizeof(rt_out));

    //Main traceroute loop. Note that the ping function itself returns whether the traceroute is over or not
    //Also ttl<256 and therefore termination is guaranteed
    int ttl=1;
    while(ping(sockfd,send_addr,ttl)){
        if(ttl==256){
            //Maximum value permissible for ttl is 255
            cout<<"Can't reach Destination\n";
            break;
        }else{
            ttl++;
        }
    }

    //Deallocating destination address
    delete send_addr;

    return 0;

}