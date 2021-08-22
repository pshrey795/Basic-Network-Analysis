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

#define PKT_SIZE 64 //64 byte packet(56 byte message + 8 header)
#define PORT_NO 0

struct ICMP_packet{
    struct icmphdr header;
    char message[PKT_SIZE-sizeof(struct icmphdr)];
};

struct domain_info{
    char* name;
    char* ip_addr;
    struct hostent* domain; 

    domain_info(char* name,char* ip_addr, struct hostent* domain) : name(name), ip_addr(ip_addr), domain(domain) {}
};

struct hostent *resolveDNS(char* domain_name){
    return gethostbyname(domain_name);
}

unsigned short calculateCheckSum(void* pkt,int length){
    unsigned short *acc = (unsigned short*)pkt;
	unsigned int sum=0;

	while(length>1){
        sum += *acc;
        *acc++;
        length-=2;
    }	
	if(length==1){
        sum += *(unsigned char*)acc;
    }	
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
    unsigned short result = ~sum;
	return result;
}

bool ping(int sock_desc, struct domain_info *domain, int ttl_val, double timeout){

    struct ICMP_packet send_pkt;
    struct ICMP_packet rcv_pkt;

    //Setting timeout values
    struct timeval rt_out;
    rt_out.tv_sec = (int)timeout;
    rt_out.tv_usec = (int)(1000000.0*(timeout-rt_out.tv_sec));
    //cout<<rt_out.tv_sec<<" "<<rt_out.tv_usec<<"\n";

    //Socket settings
    setsockopt(sock_desc, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val));
    setsockopt(sock_desc, SOL_SOCKET, SO_RCVTIMEO, &rt_out, sizeof(rt_out));

    //Filling packet
    bzero(&send_pkt,sizeof(send_pkt));
    send_pkt.header.type = ICMP_ECHO;            //type 8 for echo requests in ping
    send_pkt.header.un.echo.id = getpid();       //ID of the current process
    send_pkt.header.un.echo.sequence = 1;
    send_pkt.header.checksum = calculateCheckSum(&send_pkt,sizeof(send_pkt));
    //cout<<calculateCheckSum(&send_pkt,sizeof(send_pkt))<<"\n";

    // for(auto i=0;i<sizeof(send_pkt.message)-1;i++){
    //     send_pkt.message[i] = i + '0';
    // }
    // send_pkt.message[sizeof(send_pkt.message)-1] = 0;

    //Resolve destination address
    struct sockaddr_in *send_addr = (struct sockaddr_in*)malloc(sizeof(sockaddr_in));
    (*send_addr).sin_family = domain->domain->h_addrtype;
    (*send_addr).sin_port = htons (PORT_NO);
    (*send_addr).sin_addr.s_addr = *(long *)(domain->domain->h_addr);

    struct sockaddr_in rcv_addr;
    socklen_t rcv_addr_len = sizeof(rcv_addr);
    int send_success,rcv_success;

    //Ping starts here
    auto start = high_resolution_clock::now();

    send_success = sendto(sock_desc,&send_pkt,sizeof(send_pkt),0,(const sockaddr *)send_addr,sizeof(*send_addr));
    rcv_success = recvfrom(sock_desc,&rcv_pkt,sizeof(rcv_pkt),0,(sockaddr *)&rcv_addr,&rcv_addr_len);

    auto stop = high_resolution_clock::now();
 
    if(rcv_success<0){
        cout<<"*\n";
    }else{
        cout<<inet_ntoa(rcv_addr.sin_addr)<<"\n";
    }

    auto rtt = duration_cast<microseconds>(stop-start);
    //cout<<rtt.count()<<"\n\n";

    delete send_addr;

    cout<<(int)rcv_pkt.header.type<<"\n";
    // cout<<calculateCheckSum(&rcv_pkt,sizeof(rcv_pkt))<<"\n";
    // cout<<"\n\n";

    return true;
}

int main(int argc,char* argv[]){

    char* domain_name;

    if(argc<2){
        cout<<"Please provide domain name: ";cin>>domain_name;
    }else{
        domain_name = argv[1];
    }

    struct hostent *domain_IP = resolveDNS(domain_name); 
    int sockfd = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);

    if(sockfd<0){
        cout<<sockfd<<"\n";
    }

    char* ip_addr = inet_ntoa(*(struct in_addr *)domain_IP->h_addr);
    cout<<"Resolved IP address: "<<ip_addr<<"\n";

    int m = 5;

    struct domain_info stats(domain_name,ip_addr,domain_IP);
    struct domain_info *ping_stats = &stats;

    cout<<ping_stats->ip_addr<<"\n";

    ping(sockfd,ping_stats,16,3.0);
    return 0;

}