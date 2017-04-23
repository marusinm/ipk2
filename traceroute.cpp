#include <getopt.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <vector>
#include <iomanip>
#include <linux/errqueue.h>
#include <netinet/icmp6.h>
#include <sys/time.h>
#include <arpa/inet.h>

using namespace std;

/**
 * Structure of arguments
**/
typedef struct params {
    int first_ttl, max_ttl;
    std::string address;
} Params;

/**
 * parse program arguments
 * @return structure of arguments
 */
Params getParams(int argc, char **argv) {

    int first_ttl = 1;  //default value
    int max_ttl = 30;   //default value
    std::string input_ip = "";

    if (argc > 1) {
        input_ip = argv[argc-1];
        int opt;
        while ((opt = getopt(argc, argv, "f:m:")) != EOF) {
            switch (opt) {
                case 'f':
                    first_ttl = std::stoi(optarg);
                    break;
                case 'm':
                    max_ttl = std::stoi(optarg);
                    break;
            }
        }
    }

    //TODO: handle options (if max is smaller than first, abs valeus, etc.)

    Params params = {first_ttl, max_ttl, input_ip};
    return params;
}

int main(int argc, char **argv) {

    //get params from cmd
    Params params = getParams(argc, argv);

    int sock;
    struct addrinfo hints;
    struct addrinfo *result;

    //set structure for getaddrinfo
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family=AF_UNSPEC; //flag means - we can use both ipv4 and ipv6
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_protocol=0;
    hints.ai_flags=AI_ADDRCONFIG;
    std::string port = "33434"; //on forum TODO: vyskusat aj port 4

    if (int s = getaddrinfo(params.address.c_str(),port.c_str(),&hints, &result) != 0) {
        std::cerr << "getaddrinfo: %s" << gai_strerror(s) << "\n";
        return 0;
    }

    sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    int hop;
    bool done = false;
    for (hop = params.first_ttl; hop <= params.max_ttl && done == false; hop++) {

        bool traceFinished = false;
        if (result->ai_family == AF_INET) {  //IPv4
            setsockopt(sock, IPPROTO_IP, IP_TTL, &hop, sizeof(hop));
            int on = 1;
            setsockopt(sock, SOL_IP, IP_RECVERR,(char*)&on, sizeof(on));    // osx to nepozna na docker to funguje

        } else if (result->ai_family == AF_INET6) { //IPv6
            setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hop, sizeof(hop));
            int on = 1;
            setsockopt(sock, SOL_IPV6, IPV6_RECVERR,(char*)&on, sizeof(on)); // osx to nepozna na docker to funguje
        }

        //solution with connect and send()
//        int r = connect(sock, result->ai_addr, result->ai_addrlen);
//        if(r == -1){
//            std::cerr << "connect() -1";
//        }

        //try to send message
        const char *message = "PING";
        int test = (int) sendto(sock, message, strlen(message), 0, result->ai_addr, result->ai_addrlen);
//        int test = (int) send(sock, message, strlen(message), 0);
        if (test < 0){
            std::cerr << "sned() -1" ;
        }

        struct timeval timeout;
        gettimeofday(&timeout, NULL);

        char cbuf[512]; //buffer for response
        struct iovec iov; //io structure
        struct msghdr msg; //receive message - could contain more headers
        struct cmsghdr *cmsg; //concrete header
        struct icmphdr icmph; //icmp header
        struct sockaddr_storage target; //structure for address compatible with ipv4/ipv6

        while(1) {

            iov.iov_base = &icmph; //we will receive icmp header
            iov.iov_len = sizeof(icmph); //size of icmp header
            msg.msg_name = (void *) &target; //save dest. addr
            msg.msg_namelen = sizeof(target); //size of dest. addr
            msg.msg_iov = &iov; //icmp header
            msg.msg_iovlen = 1; // number of headers
            msg.msg_flags = 0;  //no flags
            msg.msg_control = cbuf; //buffer for message control
            msg.msg_controllen = sizeof(cbuf);

            if(traceFinished){
                break;
            }

            int res = (int) recvmsg(sock, &msg, MSG_ERRQUEUE | MSG_WAITALL);

            struct timeval timeout1;
            gettimeofday(&timeout1, NULL);

            struct timeval time_res;
            timersub(&timeout1, &timeout, &time_res);

            if(time_res.tv_sec >= 2){ // 2 seconds
                std::cout<<hop<<"\t*\n";
                break;
            }

            double ms = (double) (timeout1.tv_sec - timeout.tv_sec) * 1000.0 +
                 (double) (timeout1.tv_usec - timeout.tv_usec) / 1000.0;

            if (res < 0) continue; //in case when message not arrived

            //iterate lineary linked list of headers
            for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {

                //check for headers
                if ((cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVERR) || (cmsg->cmsg_level == SOL_IPV6 && cmsg->cmsg_type == IPV6_RECVERR)) {

                    //get data from headers
                    struct sock_extended_err *e = (struct sock_extended_err *) CMSG_DATA(cmsg);

                    //must check this for ipv4 and ipv6
                    if ((e && e->ee_origin == SO_EE_ORIGIN_ICMP) || (e && e->ee_origin == SO_EE_ORIGIN_ICMP6)) {

                        //get struct for address for ipv4 and ipv6
                        struct sockaddr_storage *sin_unspec = (struct sockaddr_storage *) (e + 1);

                        //get hostname for extension
                        char host[1024];
                        getnameinfo((struct sockaddr*) sin_unspec, sizeof(struct sockaddr_storage), host, sizeof(host), NULL, 0, 0);


                        if ( sin_unspec->ss_family == AF_INET) { //flags for ipv4
                            struct sockaddr_in *sin = (struct sockaddr_in *) (e + 1);
                            traceFinished = true;
                            if ((e->ee_type == ICMP_DEST_UNREACH)) {  // 3
                                if (e->ee_code == ICMP_PORT_UNREACH) { //3
                                    done = true;

                                    //TODO: rewrite to inet_ntop
                                    std::cout << hop << "\t "<< host << " (" << int(sin->sin_addr.s_addr & 0xFF) << "." <<
                                            int((sin->sin_addr.s_addr & 0xFF00) >> 8) << "." <<
                                            int((sin->sin_addr.s_addr & 0xFF0000) >> 16) << "." <<
                                            int((sin->sin_addr.s_addr & 0xFF000000) >> 24)<<")";
                                            printf("\t%0.3lf %s\n", ms, " ms");
                                } else if (e->ee_code == ICMP_HOST_UNREACH) {  //1
                                    cout << "H!\n";
                                    break;
                                } else if (e->ee_code == ICMP_NET_UNREACH) {  // 0
                                    cout << "N!\n";
                                    break;
                                } else if (e->ee_code == ICMP_PROT_UNREACH) { // protocol // 2
                                    cout << "P!\n";
                                    break;
                                } else if (e->ee_code == ICMP_PKT_FILTERED) { // communication admini. prohi. // 13
                                    cout << "X!\n";
                                    break;
                                } else {
                                    cout << "Destination Unreachable\n";
                                }
                            } else {

                                //TODO: rewrite to inet_ntop
                                std::cout << hop << "\t " << host << " (" << int(sin->sin_addr.s_addr & 0xFF) << "." <<
                                        int((sin->sin_addr.s_addr & 0xFF00) >> 8) << "." <<
                                        int((sin->sin_addr.s_addr & 0xFF0000) >> 16) << "." <<
                                        int((sin->sin_addr.s_addr & 0xFF000000) >> 24)<<")";
                                printf("\t%.03lf %s\n", ms, " ms");
                            }
                        } else { //flags for ipv6
                            struct sockaddr_in6 *sin = (struct sockaddr_in6 *) (e + 1);
                            traceFinished = true;

                            if ((e->ee_type == ICMP6_DST_UNREACH)) {  // 1
                                if (e->ee_code == ICMP6_DST_UNREACH_NOPORT) { //4

                                    done = true;
                                    char str[INET6_ADDRSTRLEN];
                                    inet_ntop(AF_INET6, &sin->sin6_addr, str, INET6_ADDRSTRLEN);
                                    cout << hop << "\t " << host << " (" << str << ")";
                                  printf("\t%.03lf %s\n", ms, " ms");

                                } else if (e->ee_code == ICMP6_DST_UNREACH_ADDR) {  //3
                                    cout << "H!\n";
                                    break;
                                } else if (e->ee_code == ICMP6_DST_UNREACH_NOROUTE) {  // 0
                                    cout << "N!\n";
                                    break;
                                } else if (e->ee_code == 7) { // protocol // Error in Source Routing Header // 7
                                    cout << "P!\n";
                                    break;
                                } else if (e->ee_code == ICMP6_DST_UNREACH_ADMIN) { // communication admini. prohi. // 1
                                    cout << "X!\n";
                                    break;
                                } else {
                                    cout << "Destination Unreachable\n";
                                }
                            }else {
                                char str[INET6_ADDRSTRLEN];
                                inet_ntop(AF_INET6, &sin->sin6_addr, str, INET6_ADDRSTRLEN);
                                cout << hop << "\t " << host << " (" << str << ")";
                                printf("\t%.03lf %s\n", ms, " ms");
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}