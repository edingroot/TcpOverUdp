#define SRV_IP "127.0.0.1"
#include <arpa/inet.h>
//#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/types.h>
//#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
     
#define BUFLEN 512
#define NPACK 50
#define PORT 9930
//#define BUF_SIZE 1000

#define SLOW_START 1
#define C_AVOID 2
#define F_RCVRY 3

 /* diep(), #includes and #defines like in the server */

struct sockaddr_in initial() {
	struct sockaddr_in si_other;

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	if (inet_aton(SRV_IP, &si_other.sin_addr) == 0) {
		fprintf(stderr, "inet_aton() failed\n");
		return si_other;
	}

	return si_other;
}

 
int connect_socket(void){
 	int s;
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		perror("socket");

	return s;
}

void close_socket(int s){
	close(s);
}

void print_cwnd(int cwnd){
	printf("CWND = %d\n", cwnd);
}

void print_duplicate(){
	printf("3 duplicate ack\n");
}

void print_timeout() {
	printf("Time out\n");
}

void print_state(int state) {
    switch (state) {
        case SLOW_START:
            printf("Current state: SLOW_START\n");
            break;

        case C_AVOID:
            printf("Current state: C_AVOID\n");
            break;

        case F_RCVRY:
            printf("Current state: F_RCVRY\n");
            break;
    }
}

void send_packets(int s, int cwnd, int send_buffer[], int *lastByteSent, struct sockaddr_in si_other) {
    int i;
    int slen = sizeof(si_other);
    char buf[BUFLEN];

    for (i = 0; i < cwnd && *lastByteSent < NPACK; i++) {
//        printf("sending packet %d\n", (*lastByteSent) + 1);
        memset(buf, 0, BUFLEN);
        sprintf(buf, "%d", send_buffer[++(*lastByteSent)]);
        sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t) slen);
    }
}

int main(void) {
    struct sockaddr_in si_other, recv_ip;
	int s, i, slen = sizeof(si_other), rlen = sizeof(recv_ip);
    char buf[BUFLEN];

	struct timeval tv;
  	fd_set readfds;

	si_other = initial();
	s = connect_socket();

	int send_buffer[NPACK + 1];
	int cwnd = 1;
	double cwnd_double = 1.0;
	int lastByteSent = 0;
	int lastByteAcked = 0;
	int ssthresh = 10;

	int pre_acked;
	int acked = 0;
	int duplicate_ack_count = 0;

	int state = SLOW_START;

	for (i = 0; i < NPACK + 1; i++) {
		send_buffer[i] = i;
	}

/* Insert your codes below */

    // Set socket timeout
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error");
    }

    print_state(state);
    print_cwnd(cwnd);

    // Send the first packet
    lastByteSent = -1;
    send_packets(s, cwnd, send_buffer, &lastByteSent, si_other);

    while (lastByteAcked < NPACK + 1) {
        // Wait for ACK packet
        if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr*) &recv_ip, (socklen_t*) &rlen) == -1) {
            // Timeout reached
            print_timeout();
            switch (state) {
                case SLOW_START:
                    ssthresh = cwnd / 2;
                    cwnd = 1;
                    duplicate_ack_count = 0;
                    break;

                case C_AVOID:
                case F_RCVRY:
                    ssthresh = cwnd / 2;
                    cwnd = 1;
                    duplicate_ack_count = 0;
                    print_cwnd(cwnd);
                    state = SLOW_START;
                    print_state(state);
                    break;
            }

            // Re-transmit missing segment
            lastByteSent = lastByteAcked - 1;
            send_packets(s, cwnd, send_buffer, &lastByteSent, si_other);
            continue;
        }

        // ACK packet received
        acked = atoi(buf);
//        printf("Received ACK packet from %s:%d Seq: %d\n\n",
//               inet_ntoa(si_other.sin_addr),
//               ntohs(si_other.sin_port),
//               acked
//        );
        if (acked == lastByteAcked) { // Duplicate ACK
            switch (state) {
                case SLOW_START:
                case C_AVOID:
                    duplicate_ack_count++;

                    if (duplicate_ack_count == 3) {
                        print_duplicate();
                        ssthresh = cwnd / 2;
                        cwnd = ssthresh + 3;
                        print_cwnd(cwnd);
                        state = F_RCVRY;
                        print_state(state);

                        // Re-transmit missing segment
                        lastByteSent = acked - 1;
                        send_packets(s, cwnd, send_buffer, &lastByteSent, si_other);

                    }
                    break;

                case F_RCVRY:
                    cwnd += 1;
                    // Transmit new segment(s), as allowed
                    send_packets(s, cwnd, send_buffer, &lastByteSent, si_other);

                    break;
            }


        } else { // New ACK
            lastByteAcked = acked;

            switch (state) {
                case SLOW_START:
                    cwnd += 1;
                    duplicate_ack_count = 0;
                    print_cwnd(cwnd);

                    // Transmit new segment(s), as allowed
                    send_packets(s, cwnd, send_buffer, &lastByteSent, si_other);

                    if (cwnd >= ssthresh) {
                        state = C_AVOID;
                        cwnd_double = cwnd;
                        print_state(state);
                    }
                    break;

                case C_AVOID:
                    cwnd_double += 1.0 / cwnd_double;
                    cwnd = (int) cwnd_double;
                    duplicate_ack_count = 0;
                    print_cwnd(cwnd);

                    // Transmit new segment(s), as allowed
                    send_packets(s, cwnd, send_buffer, &lastByteSent, si_other);

                    break;

                case F_RCVRY:
                    cwnd = ssthresh == 0 ? 1 : ssthresh;
                    duplicate_ack_count = 0;

                    state = C_AVOID;
                    cwnd_double = cwnd;
                    print_state(state);
                    break;
            }
        }

    }

/* Insert your codes above */

	close_socket(s);
	return 0;
}
