#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <netinet/ether.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
/*
 * Napište program pro příjem všech linkových rámců vypisující následující informace: 
 * délka rámce
 * linkový protokol
 * adresy odesílatele
 * adresy příjemce (v dvojtečkovém tvaru)
 * protokol síťové vrstvy 
 */

#define BUFFLEN 1500

int finish = 0;

void sig_handler(int sig) {
    if (sig == SIGINT)
        finish = 1;
}

int main(int argc, char *argv[]) {
    int count = 0, ethidx = -1;
    char* s, *chyba;
    if (argc == 2) {
        s = argv[1];
        count = (int) strtol(s, &chyba, 10);
        ethidx = -1;
    }
    if (argc == 3) {
        s = argv[1];
        count = (int) strtol(s, &chyba, 10);
        s = argv[2];
        ethidx = (int) strtol(s, &chyba, 10);
    }

    //    cout << "argv0: " << argv[0] << " argv1: " << argv[1] << " argv2: " << argv[2] << endl;
    //    cout << "count: " << count << " ethidx: " << ethidx << endl;

    struct sigaction act;

    int sock, i;
    ssize_t length;
    char buffer[BUFFLEN];
    struct sockaddr_ll address;
    socklen_t address_size;

    memset(&act, 0, sizeof (act));
    act.sa_handler = sig_handler;
    sigaction(SIGINT, &act, NULL);

    sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (sock == -1) {
        perror("socket error");
        return -1;
    }

    address_size = sizeof (address);

    for (int i = 0; i < count; i++) {
        length = recvfrom(sock, buffer, BUFFLEN, 0, (struct sockaddr *) &address,
                &address_size);

        if (length == -1) {
            if (errno != EINTR) {
                perror("recvfrom error");
                close(sock);
                return -1;
            } else
                continue;
        }

        struct ether_header* header;
        header = (struct ether_header*) buffer;


        struct iphdr* data;
        data = (struct iphdr*) (buffer + sizeof (struct ether_header));

        struct in_addr address2;

        if (ethidx == -1 || ethidx == address.sll_ifindex) {
            cout << "Verze protokolu: " << (unsigned int) data->version << endl;
            cout << "Velikost hlavicky: " << (unsigned int) ntohs(data->ihl) << endl;
            cout << "Identifikátor packetu: " << data->id << endl;
            cout << "Celková délka packetu: " << data->tot_len << endl;
            cout << "Protokol (linkový): " << ntohs(address.sll_protocol) << endl;
            cout << "Index síťového rozhraní: " << address.sll_ifindex << endl;
            cout << "Velikost (linkové) adresy odesílatele: " << ntohs(address.sll_halen) << endl;
            cout << "adresa prijemce : " << ether_ntoa((ether_addr*) header->ether_dhost) << endl;
            address2.s_addr = data->saddr;
            cout << "IP adresa prijemce : " << inet_ntoa(address2) << endl;
            address2.s_addr = data->daddr;
            cout << "IP adresa odesilatele : " << inet_ntoa(address2) << endl;
            cout << "doba zivotnosti paketu (TTL) " << ntohs(data->ttl) << endl;



            switch (data->protocol) {
                case IPPROTO_TCP:
                    cout << "protokol sitove vrstvy : TCP" << endl << endl;
                    break;
                case IPPROTO_UDP:
                    cout << "protokol sitove vrstvy : UDP" << endl << endl;
                    break;
                default:
                    cout << "protokol sitove vrstvy : " << ntohs(data->protocol) << endl << endl;
                    break;
            }
        } else {
            cout << "Packet je z jineho rozhrani nez je pozadovano. ignoruji ho" << endl << endl;
        }



        /*
            if(data->protocol == IPPROTO_TCP) { 
                cout << "protokol sitove vrstvy : TCP" << endl;
            } else {
                if(data->protocol == IPPROTO_UDP) {
                    cout << "protokol sitove vrstvy : UDP" << endl;
                } else {
                    cout << "protokol sitove vrstvy : " << ntohs(data->protocol) << endl << endl;
                }
            }
         */
    }

    /* zrusime socket */
    close(sock);

    return 0;
}