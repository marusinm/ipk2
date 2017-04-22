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

//#include <sys/types.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <sys/time.h>
//#include <netinet/in.h>
//#include <netinet/udp.h>
//#include <netinet/ip6.h>





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

    //TODO: zistit exit code pri zlom vstupe a doplnit
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

    Params params = {first_ttl, max_ttl, input_ip};
    return params;
}

int main(int argc, char **argv) {

    Params params = getParams(argc, argv);

    std::cout << "ip " <<  params.address << "\n";
    std::cout << "first ttl " << params.first_ttl << "\n";
    std::cout << "max ttl " << params.max_ttl << "\n";





    int sock, recv;
    struct addrinfo hints;
    struct addrinfo *result;

    //set structure for getaddrinfo
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family=AF_UNSPEC;
//    hints.ai_socktype=SOCK_RAW;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_protocol=0;
    hints.ai_flags=AI_ADDRCONFIG;
    std::string port = "33434"; //on forum TODO: vyskusat aj port 4

    if (int s = getaddrinfo(params.address.c_str(),port.c_str(),&hints, &result) != 0) {
        std::cerr << "getaddrinfo: %s" << gai_strerror(s) << "\n";
        return 6; //TODO: which result code out ?
    }

    // socket returns: (http://man7.org/linux/man-pages/man2/socket.2.html)
    // On success, a file descriptor for the new socket is returned.  On
    // error, -1 is returned, and errno is set appropriately.
    sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);


    int hop = 2;
    bool done = false;
    for (hop = 1; hop <= params.max_ttl && done == false; hop++) {
//    for (hop = 1; hop <= 4; hop++) {
        bool traceFinished = false;

//        std::cout << "±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±\n";
//        std::cout << "±±±±±±±±±±±±±±±±±±±"<<hop<<" "<<hop<<" "<<hop<<" "<<hop<<"±±±±±±±±±±±±±±±±±±±±±\n";
//        std::cout << "±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±\n";
        if (result->ai_family == AF_INET) {  //IPv4
    //        int hop = 0;
            setsockopt(sock, IPPROTO_IP, IP_TTL, &hop, sizeof(hop));
            int on = 1;
            //Mozno bude treba zmenit SOL_IP
            setsockopt(sock, SOL_IP, IP_RECVERR,(char*)&on, sizeof(on));    // osx to nepozna na docker to funguje
//            std::cout << "ipv 4\n";

        } else if (result->ai_family == AF_INET6) { //IPv6
    //        int hop = 0;
            setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hop, sizeof(hop));
            int on = 1;
            setsockopt(sock, SOL_IPV6, IPV6_RECVERR,(char*)&on, sizeof(on)); // osx to nepozna na docker to funguje
//            std::cout << "ipv 6\n";
        }

//        int r = connect(sock, result->ai_addr, result->ai_addrlen);
//        if(r == -1){
//            std::cerr << "connect() -1";
//        }

        //try to send message
        const char *message = "PING";
        //FIXME: sendto by nemal potrebovat connect ale mozno ho tam nakoniec bude treba dat
        //FIXME: v tutoriali je napisane nieco ine, funkcia s piatimi parametrami, ako flag som pridal 0
        int test = (int) sendto(sock, message, strlen(message), 0, result->ai_addr, result->ai_addrlen);
//        int test = (int) send(sock, message, strlen(message), 0);
//        std::cout << "send " << test << " chars\n";
        // get time

//        struct timeval timeout;
//        timeval time;
//        fd_set fds;
//        gettimeofday(&time, NULL);
//        double millis = (time.tv_sec * 1000.0) + (time.tv_usec / 1000.0);
//         set timeout and options for select
//        timeout.tv_sec = 3;
//        timeout.tv_usec = 0;
//        FD_ZERO(&fds);
//        FD_SET(sock, &fds);
//        char *recv_buffer;
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


//            printf("1 %s hop: %d \n", (char *)iov.iov_base, hop);

            if(traceFinished){
//                std::cout << "route finished\n";
                break;
            }

//            break;
            int res = (int) recvmsg(sock, &msg, MSG_ERRQUEUE | MSG_WAITALL);

            struct timeval timeout1;
            gettimeofday(&timeout1, NULL);

            struct timeval time_res;
            timersub(&timeout1, &timeout, &time_res);
//            std::cout<< "timeres: "<<time_res.tv_sec<<"\n";
            if(time_res.tv_sec >= 2){

                std::cout<<hop<<"  * *\n";
                break;
            }

            //TRY via slelect a recvfrom
//            int r = connect(sock, result->ai_addr, result->ai_addrlen);
//            if(r == 0){
//                std::cout << "connect ok\n";
//            }
//            int res = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
//            int res = select(sock+1, &fds, NULL, NULL, &timeout);
//            int res = select(sizeof(fds), &fds, NULL, NULL, &timeout);
//            if (res == -1){
//                cout<<"select error error\n";
//                break;
//            }else if(res == 0){
//                cout<<"time expired\n";
//            }else{
//                res = (int) recvfrom (sock, recv_buffer, IP_MAXPACKET, 0, NULL, NULL);
//                if(res >= 0){
//                    std::cout << "recvfrom ok\n";
//                }
//            }

//            res = (int)recvfrom(sock, &msg, sizeof(msghdr), 0,  result->ai_addr, &result->ai_addrlen);
//            if(res >= 0){
//                std::cout << "recvfrom ok\n";
//            }


            if (res < 0) continue; //in case when message

//            printf("1 %s hop: %d \n", (char *)iov.iov_base, hop);

            //TODO:
            //TODO:
            //TODO:TOto mam skopcene z webu, pozri este to treba porovnat s tym dokumentom co posielali na forum
            //TODO:
            //lineary linked list
            for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {


                //TODO: prepisat koment
                /* skontrolujeme si pôvod správy - niečo podobné nám bude treba aj pre IPv6 */
                if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVERR) {
                    //TODO: prepisat koment
                    //získame dáta z hlavičky
                    struct sock_extended_err *e = (struct sock_extended_err *) CMSG_DATA(cmsg);

                    //TODO: prepisat koment
                    //bude treba niečo podobné aj pre IPv6 (hint: iný flag) //TODO: spravit pre IPV6
                    if (e && e->ee_origin == SO_EE_ORIGIN_ICMP) {

                        /* získame adresu - ak to robíte všeobecne tak sockaddr_storage */
//                        struct sockaddr_in *sin = (struct sockaddr_in *) (e + 1);
                    struct sockaddr_storage *sin_unspec = (struct sockaddr_storage *) (e + 1);

                        //TODO: prepisat koment
                        /*
                        * v sin máme zdrojovú adresu
                        * stačí ju už len vypísať viď: inet_ntop alebo getnameinfo
                        */

                        //TODO: toto je kod z poznamok, staihnuty z messangeru
                    if ( sin_unspec->ss_family == AF_INET) {
//                        if (sin->sin_family == AF_INET) {
                            struct sockaddr_in *sin = (struct sockaddr_in *) (e + 1);
                            traceFinished = true;
//                            std::cout << "finish detection\n";
                            if ((e->ee_type == ICMP_DEST_UNREACH)) {  // 3
                                if (e->ee_code == ICMP_PORT_UNREACH) { //3
//                                printf("HEJ, NOW DO SOMTING NEW %s \n", sin->sin_addr);
                                    done = true;
//                                    printf("ipaddress port_unrech %d.%d.%d.%d\n",
//                                           int(sin->sin_addr.s_addr & 0xFF),
//                                           int((sin->sin_addr.s_addr & 0xFF00) >> 8),
//                                           int((sin->sin_addr.s_addr & 0xFF0000) >> 16),
//                                           int((sin->sin_addr.s_addr & 0xFF000000) >> 24));
                                    std::cout << hop << " " << int(sin->sin_addr.s_addr & 0xFF) << "." <<
                                            int((sin->sin_addr.s_addr & 0xFF00) >> 8) << "." <<
                                            int((sin->sin_addr.s_addr & 0xFF0000) >> 16) << "." <<
                                            int((sin->sin_addr.s_addr & 0xFF000000) >> 24) << "\n";
//                                printf("HEJ, NOW DO SOMTING NEW %s \n", ms );
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
//                                printf(" ip address: %d.%d.%d.%d\n",
//                                       int(sin->sin_addr.s_addr & 0xFF),
//                                       int((sin->sin_addr.s_addr & 0xFF00) >> 8),
//                                       int((sin->sin_addr.s_addr & 0xFF0000) >> 16),
//                                       int((sin->sin_addr.s_addr & 0xFF000000) >> 24));
                                std::cout << hop << " " << int(sin->sin_addr.s_addr & 0xFF) << "." <<
                                        int((sin->sin_addr.s_addr & 0xFF00) >> 8) << "." <<
                                        int((sin->sin_addr.s_addr & 0xFF0000) >> 16) << "." <<
                                        int((sin->sin_addr.s_addr & 0xFF000000) >> 24) << "\n";
                            }
                        } else {

                            struct sockaddr_in6 *sin = (struct sockaddr_in6 *) (e + 1);
                            if ((e->ee_type == ICMP6_DST_UNREACH)) {  // 1
                                traceFinished = true;
                                if (e->ee_code == ICMP6_DST_UNREACH_NOPORT) { //4

                                    struct sockaddr_in6 sa;
                                    char str[INET6_ADDRSTRLEN];
                                    inet_pton(AF_INET6, "2001:db8:8714:3a90::12", &(sa.sin6_addr));
                                    inet_ntop(AF_INET6, &(sa.sin6_addr), str, INET6_ADDRSTRLEN);
                                    printf("%s\n", str); // prints "2001:db8:8714:3a90::12"

//                                    std::cout << hop << " " << int(sin->sin6_addr.s_addr & 0xFF) << "." <<
//                                    int((sin->sin6_addr.s_addr & 0xFF00) >> 8) << "." <<
//                                    int((sin->sin6_addr.s_addr & 0xFF0000) >> 16) << "." <<
//                                    int((sin->sin6_addr.s_addr & 0xFF000000) >> 24) << "\n";
//
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
                            }else{
                                struct sockaddr_in6 sa;
                                char str[INET6_ADDRSTRLEN];
                                inet_pton(AF_INET6, "2001:db8:8714:3a90::12", &(sa.sin6_addr));
                                inet_ntop(AF_INET6, &(sa.sin6_addr), str, INET6_ADDRSTRLEN);
                                printf("aa %s\n", str); // prints "2001:db8:8714:3a90::12"
                            }
                        }

                        //TODO: toto je kod zo stack overflov
                        /* We are intrested in ICMP errors */
//                    if (e->ee_origin == SO_EE_ORIGIN_ICMP)
//                    {
//                        /* Handle ICMP errors types */
//                        switch (e->ee_type)
//                        {
//                            case ICMP_NET_UNREACH:
//                                /* Hendle this error */
//                                printf( "Network Unreachable Error\n");
//                                break;
//                            case ICMP_HOST_UNREACH:
//                                /* Hendle this error */
//                                printf("Host Unreachable Error\n");
//                                break;
//                                /* Handle all other cases. Find more errors :
//                                 * http://lxr.linux.no/linux+v3.5/include/linux/icmp.h#L39
//                                 */
//                            default:
//                                printf("ahoj moj\n");
//                                break;
//
//                        }
//                    }

                        //TODO takto vyzera tato podmienka z toho dokumentu
//                    if ( (e->ee_type == ICMP_DEST_UNREACH) && (e->ee_code ==
//                                                               ICMP_PORT_UNREACH) )
//                        printf("Destination port unreachable\n");
                        //TODO: pozri aj stackoverflow ktory mas otvoreny toto sa tam riesi
//                    if (e->ee_type == ...)
//                    {
//                        /*
//                        * Overíme si všetky možné návratové správy
//                        * hlavne ICMP_TIME_EXCEEDED and ICMP_DEST_UNREACH
//                        * v prvom prípade inkrementujeme TTL a pokračujeme
//                        * v druhom prípade sme narazili na cieľ
//                        *
//                        * kódy pre IPv4 nájdete tu
//                        * http://man7.org/linux/man-pages/man7/icmp.7.html
//                        *
//                        * kódy pre IPv6 sú ODLIŠNÉ!:
//                        * nájdete ich napríklad tu https://tools.ietf.org/html/rfc4443
//                        * strana 4
//                        */
//                    }
                    }
                }
            }
        }
    }

//    if (result->ai_family == AF_INET) {  //IPv4
//        std::cout << "ipv 4\n";
////        traceroute4(params, result, sock, recv);
//    } else if (result->ai_family == AF_INET6) { //IPv6
//        std::cout << "ipv 6\n";
////        traceroute6(params, result, sock, recv);
//    }

    return 0;

}