#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include "ethercattype.h"
#include "nicdrv.h"
#include "ethercatbase.h"
#include "ethercatmain.h"
#include "ethercatdc.h"
#include "ethercatcoe.h"
#include "ethercatfoe.h"
#include "ethercatconfig.h"
#include "ethercatprint.h"

typedef struct pdo_mapping_t pdo_mapping_t;
typedef struct pdo_set_list_t pdo_set_list_t;
typedef struct slave_info_t slave_info_t;
typedef struct io_value_t io_value_t;

struct pdo_mapping_t
{
	int abs_offset;
	int abs_bit;
	int obj_idx;
	int obj_subidx;
	int bitlen;
	int bitmask;
};

struct pdo_set_list_t
{
	int list_num;
	pdo_mapping_t* pdo_mapping_list;
};

struct slave_info_t
{
	int input_list_num;
	int output_list_num;
	pdo_set_list_t* input_pdo_set_list;
	pdo_set_list_t* output_pdo_set_list;
};

struct io_value_t
{
	void* variable;
	int size;
	int slave;
	pdo_mapping_t* pdo_mapping;
	io_value_t* next;
};

static int si_map_sdo();
static int si_PDOassign(uint16 slave, uint16 PDOassign, int bitoffset, int isInput);
static pdo_mapping_t* find_pdo_mapping(int slave, int index, int subindex, int direction);

static char IOmap[4096];
static char* ifname = "eth0";

/* slave io mapping variables */
static slave_info_t* slave_list = NULL;
static int slave_num = 0;

static io_value_t* input_list = NULL;
static io_value_t* output_list = NULL;

static io_value_t* input_tail = NULL;
static io_value_t* output_tail = NULL;

static int is_connected = 0;

int io_init(void)
{
	int chk;
//	int expectedWKC;

	/* initialise SOEM, bind socket to ifname */
	if(ec_init(ifname))
	{
		/* ec_init on eth0 succeeded */
		/* find and auto-config slaves */
		if(ec_config_init(FALSE) > 0)
		{
			/* ec_slavecount slaves found and configured */
			slave_num = ec_slavecount;
			slave_list = (slave_info_t*)malloc(sizeof(slave_info_t) * (slave_num + 1));

			ec_config_map(&IOmap);
			ec_configdc();
			/* Slaves mapped, state to SAFE_OP */

			/* wait for all slaves to reach SAFE_OP state */
			ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);
			if(ec_slave[0].state != EC_STATE_SAFE_OP)
			{
				/* Not all slaves reached safe operational state */
				ec_readstate();
/*				for(i = 1; i <= ec_slavecount ; i++)
				{
					if(ec_slave[i].state != EC_STATE_SAFE_OP)
					{
						printf("Slave %d State=%2x StatusCode=%4x : %s\n",
							i, ec_slave[i].state, ec_slave[i].ALstatuscode,
							ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
					}
				}*/
				fprintf(stderr, "Not all slaves reached safe operational state\n");
				return -1;
			}

			/* read mapping info. */
			si_map_sdo();

/*			printf("segments : %d : %d %d %d %d\n", ec_group[0].nsegments,
				ec_group[0].IOsegment[0], ec_group[0].IOsegment[1],
				ec_group[0].IOsegment[2], ec_group[0].IOsegment[3]);*/

			/* Request operational state for all slaves */
//			expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
			/* Calculated workcounter : expectedWKC */
			ec_slave[0].state = EC_STATE_OPERATIONAL;
			/* send one valid process data to make outputs in slaves happy*/
			ec_send_processdata();
			ec_receive_processdata(EC_TIMEOUTRET);
			/* request OP state for all slaves */
			ec_writestate(0);
			chk = 40;
			/* wait for all slaves to reach OP state */
			do
			{
				ec_send_processdata();
				ec_receive_processdata(EC_TIMEOUTRET);
				ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
			}
			while(chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));
			if(ec_slave[0].state == EC_STATE_OPERATIONAL)
			{
				/* Operational state reached for all slaves */
				is_connected = 1;
				return 0;
			}
			else
			{
				/* Not all slaves reached operational state */
				ec_readstate();
/*				for(i = 1; i <= ec_slavecount; i++)
				{
					if(ec_slave[i].state != EC_STATE_OPERATIONAL)
					{
						printf("Slave %d State=0x%2.2x StatusCode=0x%4.4x : %s\n",
							i, ec_slave[i].state, ec_slave[i].ALstatuscode,
							ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
					}
				}*/
				fprintf(stderr, "Not all slaves reached operational state\n");
				return -1;
			}
		}
		else
		{
			/* No slaves found! */
			fprintf(stderr, "No slaves found!\n");
			return -1;
		}
	}
	else
	{
		/* No socket connection on eth0, Excecute as root */
		fprintf(stderr, "No socket connection on eth0, Excecute as root\n");
		return -1;
	}
}

int io_mapping(void* variable, int size, char* address, int direction)
{
	int slave, index, subindex;
	io_value_t* io_value;

	io_value = (io_value_t*)malloc(sizeof(io_value_t));
	io_value -> variable = variable;
	io_value -> size = size;
	io_value -> pdo_mapping = NULL; 
	io_value -> next = NULL;

	sscanf(address, "%d:0x%x:0x%x", &slave, &index, &subindex);
	io_value -> slave = slave;
	io_value -> pdo_mapping = find_pdo_mapping(slave, index, subindex, direction);
	if(io_value -> pdo_mapping == NULL)
	{
		free(io_value);
		fprintf(stderr, "I/O mapping failed : no found index : 0x%x, subindex : 0x%x object\n",
			index, subindex);
		return -1;
	}
	if(((io_value -> pdo_mapping -> bitlen) / 8) > size)
	{
		free(io_value);
		fprintf(stderr, "I/O mapping failed : not enough size of model variable\n");
		return -1;
	}

	if(direction)
	{
		if(input_list == NULL)
			input_list = io_value;
		else
			input_tail -> next = io_value;
		input_tail = io_value;
	}
	else
	{
		if(output_list == NULL)
			output_list = io_value;
		else
			output_tail -> next = io_value;
		output_tail = io_value;
	}

	return 0;
}

int io_exchange(void)
{
	int wkc;
	char* byte;
	io_value_t* list;

	list = output_list;
	while(list != NULL)
	{
		if(list -> pdo_mapping -> bitlen >= 8)
			memcpy(ec_slave[list -> slave].outputs + (list -> pdo_mapping -> abs_offset),
				list -> variable, (list -> pdo_mapping -> bitlen) / 8);
		else
		{
			byte = ec_slave[list -> slave].outputs + (list -> pdo_mapping -> abs_offset);
			*byte &= ~((list -> pdo_mapping -> bitmask) << (list -> pdo_mapping -> abs_bit));
			*byte |= ((*((char*)(list -> variable)) & (list -> pdo_mapping -> bitmask)) << (list -> pdo_mapping -> abs_bit));
		}

		list = list -> next;
	}

	ec_send_processdata();
	wkc = ec_receive_processdata(EC_TIMEOUTRET);

	list = input_list;
	while(list != NULL)
	{
		if(list -> pdo_mapping -> bitlen >= 8)
			memcpy(list -> variable,
				ec_slave[list -> slave].inputs + (list -> pdo_mapping -> abs_offset),
				(list -> pdo_mapping -> bitlen) / 8);
		else
		{
			memset(list -> variable, 0, list -> size);
			*((char*)(list -> variable)) =
				(*(ec_slave[list -> slave].inputs + (list -> pdo_mapping -> abs_offset)) >>
				(list -> pdo_mapping -> abs_bit)) & (list -> pdo_mapping -> bitmask);
		}

		list = list -> next;
	}

	return 0;
}

int io_cleanup(void)
{
	io_value_t* list;
	io_value_t* tmp;
	int slave, i;

	/* free pdo mapping lists */
	for(slave = 1; slave <= slave_num; slave++)
	{
		for(i = 1; i <= slave_list[slave].input_list_num; i++)
			free(slave_list[slave].input_pdo_set_list[i].pdo_mapping_list);
		for(i = 1; i <= slave_list[slave].output_list_num; i++)
			free(slave_list[slave].output_pdo_set_list[i].pdo_mapping_list);
		free(slave_list[slave].input_pdo_set_list);
		free(slave_list[slave].output_pdo_set_list);
	}
	free(slave_list);

	/* free exchange lists */
	list = output_list;
	while(list != NULL)
	{
		tmp = list -> next;
		free(list);
		list = tmp;
	}
	list = input_list;
	while(list != NULL)
	{
		tmp = list -> next;
		free(list);
		list = tmp;
	}
	
	/* close connection */
	if(is_connected)
	{
		ec_slave[0].state = EC_STATE_INIT;
		ec_writestate(0);
		ec_close();
	}
	return 0;
}

static int si_map_sdo()
{
	int slave;
	int wkc, rdl;
	int retVal = 0;
	uint8 nSM, iSM, tSM;
	int Tsize, outputs_bo, inputs_bo;
	uint8 SMt_bug_add;
	int mData = 0xFFFFFFFF;

	SMt_bug_add = 0;
	outputs_bo = 0;
	inputs_bo = 0;
	rdl = sizeof(nSM); nSM = 0;

	for(slave = 1; slave <= slave_num; slave++)
	{
		/* read SyncManager Communication Type object count */
		wkc = ec_SDOread(slave, ECT_SDO_SMCOMMTYPE, 0x00, FALSE, &rdl, &nSM, EC_TIMEOUTRXM);

		/* positive result from slave ? */
		if((wkc > 0) && (nSM > 2))
		{
			/* make nSM equal to number of defined SM */
			nSM--;

			/* limit to maximum number of SM defined, if true the slave can't be configured */
			if(nSM > EC_MAXSM)
				nSM = EC_MAXSM;

			/* iterate for every SM type defined */
			for(iSM = 2; iSM <= nSM; iSM++)
			{
				rdl = sizeof(tSM); tSM = 0;

				/* read SyncManager Communication Type */
				wkc = ec_SDOread(slave, ECT_SDO_SMCOMMTYPE, iSM + 1, FALSE, &rdl, &tSM, EC_TIMEOUTRXM);
				ec_SDOwrite(slave, 0x7010, 0x1, FALSE, 1, &mData, EC_TIMEOUTRXM);

				if(wkc > 0)
				{
					if((iSM == 2) && (tSM == 2)) // SM2 has type 2 == mailbox out, this is a bug in the slave!
					{
						SMt_bug_add = 1; // try to correct, this works if the types are 0 1 2 3 and should be 1 2 3 4
						/* Activated SM type workaround, possible incorrect mapping.*/
					}
					if(tSM)
						tSM += SMt_bug_add; // only add if SMt > 0

					if(tSM == 3) // outputs
					{
						/* read the assign RXPDO */
						Tsize = si_PDOassign(slave, ECT_SDO_PDOASSIGN + iSM, outputs_bo, 0);
						outputs_bo += Tsize;
					}   
					if(tSM == 4) // inputs
					{
						/* read the assign TXPDO */
						Tsize = si_PDOassign(slave, ECT_SDO_PDOASSIGN + iSM, inputs_bo, 1);
						inputs_bo += Tsize;
					}   
				}   
			}
		}
	}

	/* found some I/O bits ? */
	if((outputs_bo > 0) || (inputs_bo > 0))
		retVal = 1;
	return retVal;
}

/** Read PDO assign structure */
static int si_PDOassign(uint16 slave, uint16 PDOassign, int bitoffset, int isInput)
{
	uint16 idxloop, nidx, subidxloop, rdat, idx, subidx;
	uint8 subcnt;
	int wkc, bsize = 0, rdl;
	int32 rdat2;
	uint8 bitlen, obj_subidx;
	uint16 obj_idx;
	int abs_offset, abs_bit;

	pdo_set_list_t** pdo_set_list;
	int* list_num;

	if(isInput)
	{
		pdo_set_list = &(slave_list[slave].input_pdo_set_list);
		list_num = &(slave_list[slave].input_list_num);
	}
	else
	{
		pdo_set_list = &(slave_list[slave].output_pdo_set_list);
		list_num = &(slave_list[slave].output_list_num);
	}

	rdl = sizeof(rdat); rdat = 0;

	/* read PDO assign subindex 0 ( = number of PDO's) */
	wkc = ec_SDOread(slave, PDOassign, 0x00, FALSE, &rdl, &rdat, EC_TIMEOUTRXM);
	rdat = etohs(rdat);

	/* positive result from slave ? */
	if((wkc > 0) && (rdat > 0))
	{
		/* number of available sub indexes */
		nidx = rdat;
		*pdo_set_list = (pdo_set_list_t*)malloc(sizeof(pdo_set_list_t) * (nidx + 1));
		*list_num = nidx;

		bsize = 0;

		/* read all PDO's */
		for(idxloop = 1; idxloop <= nidx; idxloop++)
		{
			rdl = sizeof(rdat); rdat = 0;

			/* read PDO assign */
			wkc = ec_SDOread(slave, PDOassign, (uint8)idxloop, FALSE, &rdl, &rdat, EC_TIMEOUTRXM);

			/* result is index of PDO */
			idx = etohl(rdat);
			if(idx > 0)
			{
				rdl = sizeof(subcnt); subcnt = 0;

				/* read number of subindexes of PDO */
				wkc = ec_SDOread(slave,idx, 0x00, FALSE, &rdl, &subcnt, EC_TIMEOUTRXM);
				subidx = subcnt;

				(*pdo_set_list)[idxloop].pdo_mapping_list = (pdo_mapping_t*)malloc(sizeof(pdo_mapping_t) * (subidx + 1));
				(*pdo_set_list)[idxloop].list_num = subidx;

				/* for each subindex */
				for(subidxloop = 1; subidxloop <= subidx; subidxloop++)
				{
					rdl = sizeof(rdat2); rdat2 = 0;

					/* read SDO that is mapped in PDO */
					wkc = ec_SDOread(slave, idx, (uint8)subidxloop, FALSE, &rdl, &rdat2, EC_TIMEOUTRXM);
					rdat2 = etohl(rdat2);

					/* extract bitlength of SDO */
					bitlen = LO_BYTE(rdat2);
					bsize += bitlen;
					obj_idx = (uint16)(rdat2 >> 16);
					obj_subidx = (uint8)((rdat2 >> 8) & 0x000000ff);
					abs_offset = bitoffset / 8;
					abs_bit = bitoffset % 8;

					(*pdo_set_list)[idxloop].pdo_mapping_list[subidxloop].abs_offset = abs_offset;
					(*pdo_set_list)[idxloop].pdo_mapping_list[subidxloop].abs_bit = abs_bit;
					(*pdo_set_list)[idxloop].pdo_mapping_list[subidxloop].obj_idx = obj_idx;
					(*pdo_set_list)[idxloop].pdo_mapping_list[subidxloop].obj_subidx = obj_subidx;
					(*pdo_set_list)[idxloop].pdo_mapping_list[subidxloop].bitlen = bitlen;
					(*pdo_set_list)[idxloop].pdo_mapping_list[subidxloop].bitmask = 0x000000ff >> (8 - bitlen);

					bitoffset += bitlen;
				}
			}
		}
	}
	/* return total found bitlength (PDO) */
	return bsize;
}

static pdo_mapping_t* find_pdo_mapping(int slave, int index, int subindex, int direction)
{
	int i, j;
	int list_num;
	pdo_set_list_t* pdo_set_list;

	if(slave > slave_num)
		return NULL;

	if(direction)
	{
		list_num = slave_list[slave].input_list_num;
		pdo_set_list = slave_list[slave].input_pdo_set_list;
	}
	else
	{
		list_num = slave_list[slave].output_list_num;
		pdo_set_list = slave_list[slave].output_pdo_set_list;
	}

	for(i = 1; i <= list_num; i++)
	{
		for(j = 1; j <= pdo_set_list[i].list_num; j++)
		{
			if((pdo_set_list[i].pdo_mapping_list[j].obj_idx == index) &&
				(pdo_set_list[i].pdo_mapping_list[j].obj_subidx == subindex))
				return &(pdo_set_list[i].pdo_mapping_list[j]);
		}
	}

	return NULL;
}
