#include "rdma_common.h"
#include "rdma_client.h"

pthread_t client_init;
pthread_t worker[NUM_QUEUES];

struct sockaddr_in s_addr;
int rdma_status;
extern struct ctrl client_session;

static void *process_client_init(void *arg)
{
	rdma_status = RDMA_INIT;
//	TEST_NZ(start_rdma_client(&s_addr));
	start_rdma_client(&s_addr);
//	while (rdma_status == RDMA_CONNECT);
}

static void *simulator(void *arg)
{
	int cpu = *(int *)arg;
	struct queue *q = get_queue(cpu);

	printf("%s: start on %d\n", __func__, cpu);

//	for (int i = 0; i <= 100; i++) {
	int i = 0 ;
	while(true){
		printf("%s: req %d on %d\n", __func__, i, cpu);
		rdma_send_wr(q, IBV_WR_SEND, &q->ctrl->servermr, NULL);
		rdma_poll_cq(q->cq, 1);
		printf("%s: done %d on %d\n", __func__, i, cpu);
		i++;
		if(i ==100){
                        break;
                        printf("%s: i is 100 in while %d\n", __func__, cpu);
                }
	}
	printf("%s: end of while %d\n", __func__, cpu);

//	rdma_status = RDMA_DISCONNECT;
}

static inline int get_addr(char *sip)
{
	struct addrinfo *info;

	TEST_NZ(getaddrinfo(sip, NULL, NULL, &info));
	memcpy(&s_addr, info->ai_addr, sizeof(struct sockaddr_in));
	freeaddrinfo(info);
	return 0;
}

static void usage(void)
{
	printf("[Usage] : ");
	printf("./rdma_main [-i <server ip>] [-p <server port>]\n");
}

int main(int argc, char* argv[])
{
	int option;

	while ((option = getopt(argc, argv, "i:p:")) != -1) {
		switch (option) {
			case 'i':
				TEST_NZ(get_addr(optarg));
				break;
			case 'p':
				s_addr.sin_port = htons(strtol(optarg, NULL, 0));
				break;
			default:
				usage();
				break;
		}
	}

	if (!s_addr.sin_port || !s_addr.sin_addr.s_addr) {
		usage();
		return 0;
	}

	pthread_create(&client_init, NULL, process_client_init, NULL);
	while (rdma_status != RDMA_CONNECT);

//	TEST_NZ(start_rdma_client(&s_addr));

	// Client is connected with server throught RDMA from now.
	// From now on, You can do what you want to do with RDMA.
	printf("The client is connected successfully\n");

	int i = 0;
        while(true){
                if(i >= NUM_QUEUES)
                        break;
		pthread_create(&worker[i], NULL, simulator, &i);
		printf("%s: make simulator %d\n", __func__, i);
		sleep(1);
		i++;
	}
	while (rdma_status == RDMA_CONNECT);
	pthread_join(client_init, NULL);
//	while (rdma_status == RDMA_CONNECT);
	return 0;
}
