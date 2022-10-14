#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>   //蓝牙的3个头文件.
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <errno.h>
#include "cpost.h"
#include <pthread.h>

static void ms_le_set_adv_param(int hdev, uint8_t adv_type, uint8_t *adv_data, uint8_t adv_len)
{
	struct hci_request rq;
	le_set_advertise_enable_cp advertise_cp;
	le_set_advertising_parameters_cp adv_params_cp;
    le_set_advertising_data_cp adv_data_cp;
	uint8_t status;
	int dd, ret;

	if (hdev < 0)
		hdev = hci_get_route(NULL);

	dd = hci_open_dev(hdev);
	if (dd < 0) {
		perror("Could not open device");
		exit(1);
	}

    // set adv disable
	memset(&advertise_cp, 0, sizeof(advertise_cp));
	advertise_cp.enable = 0x00;

	memset(&rq, 0, sizeof(rq));
	rq.ogf = OGF_LE_CTL;
	rq.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
	rq.cparam = &advertise_cp;
	rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
	rq.rparam = &status;
	rq.rlen = 1;

	ret = hci_send_req(dd, &rq, 1000);
    if (ret < 0)
		goto done;

    // set adv param
	memset(&adv_params_cp, 0, sizeof(adv_params_cp));
	adv_params_cp.min_interval = htobs(0x00C0);
	adv_params_cp.max_interval = htobs(0x00C0);
    adv_params_cp.advtype = adv_type;
	adv_params_cp.chan_map = 7;

    printf("min_interval %d max_interval %d advtype %d \r\n", 
            adv_params_cp.min_interval,
            adv_params_cp.max_interval,
            adv_params_cp.advtype);

	memset(&rq, 0, sizeof(rq));
	rq.ogf = OGF_LE_CTL;
	rq.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
	rq.cparam = &adv_params_cp;
	rq.clen = LE_SET_ADVERTISING_PARAMETERS_CP_SIZE;
	rq.rparam = &status;
	rq.rlen = 1;

	ret = hci_send_req(dd, &rq, 1000);
	if (ret < 0)
		goto done;

    // set adv data
    memset(&adv_data_cp, 0, sizeof(adv_data_cp));
    memcpy(adv_data_cp.data,adv_data,adv_len);
    adv_data_cp.length = adv_len;

    memset(&rq, 0, sizeof(rq));
	rq.ogf = OGF_LE_CTL;
	rq.ocf = OCF_LE_SET_ADVERTISING_DATA;
	rq.cparam = &adv_data_cp;
	rq.clen = LE_SET_ADVERTISING_DATA_CP_SIZE;
	rq.rparam = &status;
	rq.rlen = 1;
	ret = hci_send_req(dd, &rq, 1000);
    if (ret < 0)
		goto done;

    // set adv enable
	memset(&advertise_cp, 0, sizeof(advertise_cp));
	advertise_cp.enable = 0x01;

	memset(&rq, 0, sizeof(rq));
	rq.ogf = OGF_LE_CTL;
	rq.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
	rq.cparam = &advertise_cp;
	rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
	rq.rparam = &status;
	rq.rlen = 1;

	ret = hci_send_req(dd, &rq, 1000);

done:
	hci_close_dev(dd);
	if (ret < 0) {
		fprintf(stderr, "Can't set advertise mode on hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
		exit(1);
	}

	if (status) {
		fprintf(stderr,
			"LE set advertise enable on hci%d returned status %d\n",
								hdev, status);
		exit(1);
	}
}

static pthread_t pthread_gatt_server;

extern int ms_gatt_server_enable();
extern int ms_gatt_server_creat(uint8_t *p_addr, uint8_t p_addr_type);
void ms_bluez_gatt_cpost_handler();

static void *gatt_server_process(void *arg)
{

    int ret = ms_gatt_server_enable();
	cpost(ms_bluez_gatt_cpost_handler);
    return 0;
}

void ms_bluez_gatt_server_init()
{
    printf("ms_bluez_gatt_init ok\r\n");
    pthread_create(&pthread_gatt_server, NULL, gatt_server_process, NULL);
}

void ms_slave_test(int dev_id)
{
    uint8_t adv_data[] = {
        0x02,0x01,0x06,
        0x06,0x09,0x31,0x33,0x35,0x35,0x39
    };
    ms_le_set_adv_param(dev_id, 0x00, adv_data, 10);

    ms_bluez_gatt_server_init();
}

void ms_bluez_gatt_cpost_handler()
{
	printf("ms_bluez_gatt_cpost_handler\r\n");
	ms_slave_test(-1);
}

static pthread_t pthread_hci_recv;

static void *hci_recv_process(void *arg)
{
	int hdev = hci_get_route(NULL);
	int dd = hci_open_dev(hdev);
	if (dd < 0) {
		perror("Could not open device");
		exit(1);
	}

    struct hci_filter flt;
    /* Setup filter */
    hci_filter_clear(&flt);
    hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
    hci_filter_all_events(&flt);
    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
        printf("HCI filter setup failed");
        exit(EXIT_FAILURE);
    }
    
    unsigned char m_buf[HCI_MAX_EVENT_SIZE] = {0}; 
    int m_len = 0;
    unsigned char *ptr = m_buf;
    hci_event_hdr *hdr =NULL;

    while(1)
    {
        m_len = read(dd, m_buf, sizeof(m_buf));
        if (m_len < 0) {
            printf("Read failed");
            exit(EXIT_FAILURE);
        }
        hdr = (void *)(m_buf + 1);
        ptr = m_buf + (1 + HCI_EVENT_HDR_SIZE);
        m_len -= (1 + HCI_EVENT_HDR_SIZE);

		printf("> HCI Event: 0x%02x plen %d\n", hdr->evt, hdr->plen);
        // if(hdr->evt == 0x0E)
        // {
        //  printf("> HCI Event: 0x%02x plen %d\n", hdr->evt, hdr->plen);
			for(int i = 0; i < hdr->plen; i++)
			{
				printf("%02x ", *ptr++);
			}
    		printf("\r\n");
        // }
		// else{
		// 	printf("not 0x3e\r\n");
		// }
    }
	// cpost(ms_bluez_gatt_cpost_handler);
    return 0;
}

void ms_bluez_hci_recv_thread_start()
{
    printf("ms_bluez_hci_recv_thread_start ok\r\n");
    pthread_create(&pthread_gatt_server, NULL, hci_recv_process, NULL);
}
static pthread_t pthread_master_process;
static void *master_process(void *arg)
{
	// uint8_t addr[] = {0x04, 0xD6, 0xF4, 0x03, 0xAB, 0x87};
	uint8_t addr[] = {0x87,0xAB, 0x03, 0xF4, 0xD6, 0x04};
	ms_gatt_server_creat(addr, 0x01);
}
void master_test()
{
	printf("master_test\r\n");
	pthread_create(&pthread_master_process, NULL, master_process, NULL);
}
int main()
{
    printf("yuml bluez\r\n");
	ms_bluez_hci_recv_thread_start();
    ms_slave_test(-1);
	// master_test();
    while (1)
    {
		cpostProcess();
    }
    
    return 0;
}

