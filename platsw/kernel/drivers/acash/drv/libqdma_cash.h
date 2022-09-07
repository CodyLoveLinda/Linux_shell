/*
 * This file is part of the Autox addition to Xilinx DMA IP Core driver for Linux
 *
 */

#ifndef __LIBQDMA_CASH_H__
#define __LIBQDMA_CASH_H__
#include <linux/types.h>

/**
 * @file
 * @brief This file contains the declarations for CaSH specific qdma APIs
 * errors are all logged in dmesg
 */

struct pci_dev;
struct cash_host_cmd_t;
struct cash_fpga_event_t;
struct device;

/******************************************************************************/
/**
 * cash_qdma_get_handles() -  get devices and queue handles for existing qdma
 * configuration
 * @idx: qdma device index - pci address, for example 0x3000 for "03:00.0"
 * @dev_hndl: pointer to the qdma device handle result
 * @video_qhndls: video queue handle array holding handle results
 * - returns 0 for unconfigured queue
 * @video_qhndls_count: the size / dimension of the above array
 * @return	0 on success
 * @return	<0 on failure with negated Linux error code
 *****************************************************************************/
int cash_qdma_get_handles(unsigned int idx
	, unsigned long * dev_hndl
	, unsigned long * video_qhndls
	, unsigned int    video_qhndls_count
	, unsigned long * host_cmd_qhndl
	, unsigned long * fpga_event_qhndl);

/******************************************************************************/
/**
 * initialize_cash_video() -  call this one time at the beginning to prepare
 * FPGA into the right state before video transfers
 * @dev_hndl: qdma device handle.
 * @return	0 on success
 * @return	on failure with Linux error code
 *****************************************************************************/
int initialize_cash_video(unsigned long dev_hndl);

/******************************************************************************/
/**
 * clear_cash_video_status() -  resync host with FPGA
 * - set video request and response counters to 0, so video transfer 
 * is ready to start fresh - it is NOT a FPGA HW reset
 * @dev_hndl: qdma device handle.
 * @video_stream_id: video queue id
 * @req_init_count: the initial value for request counter - normally set to 0,
 * do not set it to more than 3
 * @return	0 on success
 * @return	on failure with Linux error code
 *****************************************************************************/
int clear_cash_video_status(unsigned long dev_hndl, uint8_t video_stream_id
	, uint32_t req_init_count);

/******************************************************************************/
/**
 * read_io_area_conf() - get the IO area configuration
 * @dev_hndl: qdma device handle
 * @bram_addr: pointer to board address result
 * @size: pointer to byte size result
 *****************************************************************************/
void read_io_area_conf(unsigned long dev_hndl
	, uint64_t* bram_addr, uint32_t* size);

/******************************************************************************/
/**
 * @struct - cash_qdma_video_request
 * @brief	use for both blocking and non-blocking video read requests.
 * the life span of this struct
 * need to outlast the call and its callback (if applicable)
 *****************************************************************************/
struct cash_qdma_video_request {
	unsigned long dev_hndl; 					//qdma dev handle 
	unsigned long qhndl;						//qdma queue handle
	struct pci_dev *pdev;
	size_t buffer_chunk_count;              	//how many buffer chunks, cannot be 0
	size_t buffer_chunk_size;               	//each chunk's byte size, cannot be 0
	void** buffer_chunks;                   	//a void* array of dimension == 
												//buffer_chunk_count, to hold the 
												//resulting allocated chunks,
												//user needs to initialized it to NULL
												//and never change them
	dma_addr_t* buffer_chunk_dma_hndls;			//a dma_addr_t array of dimension ==
												//buffer_chunk_count, to hold the 
												//resulting chunk dma handles,
												//user needs to initialized it to NULL
												//and never change them
	unsigned long user_data;					//filled by the caller and used by the
												//caller in callback

	/*if the following is NULL, it is a blocking call, otherwise it holds the
	  callback function when the read is done*/
	void (*aio_done)(unsigned long user_data 	//see above
		, unsigned int bytes_done				//how many byte read
		, int err);								//error code if not zero
	struct qdma_request* reserved;          	//initialize to 0, then do not touch
	uint16_t qid_reserved;						//initialize to 0, then do not touch
} __attribute__((aligned(8)));

/******************************************************************************/
/**
 * prepare_cash_qdma_video_request() - prepare a request for video read use based 
 * on its data members, only need to call this once if no data member changes.
 * The IO buffer chunks are allocated in this function, the user needs to specify
 * the size and number of chunks
 * Only prepared cash_qdma_video_request can be used in 
 * send_cash_qdma_video_request. call it multiple times on the same req
 * only the last one is effective and no esource leak happens
 *
 * @vreq: the request that needs to get prepared
 * @return	0 on success
 * @return	<0 on failure with negated Linux error code
 *****************************************************************************/
int prepare_cash_qdma_video_request(struct cash_qdma_video_request* vreq);

/******************************************************************************/
/**
 * unprepare_cash_qdma_video_request() - release the resources obtained by 
 * prepare_video_request including buffers
 *
 * @cash_qdma_request: the request that needs to get teared down
 *****************************************************************************/
void unprepare_cash_qdma_video_request(struct cash_qdma_video_request* vreq);


/******************************************************************************/
/**
 * send_cash_qdma_video_request() - send out a cash qdma video read request
 *
 * @dev_hndl: qdma device handle
 * @qhndl: qdma queue handle
 * @vreq: pointer to a cash_qdma_video_request which need to be prepared using 
 * 			prepare_cash_qdma_video_request	
 * @return	0 on non-blocking submission success, IO byte count on blocking
 * @return	<0 on failure with negated Linux error code
 *****************************************************************************/
ssize_t send_cash_qdma_video_request(struct cash_qdma_video_request* vreq);

/**
 * @struct - cash_qdma_request
 * @brief	use for both blocking and non-blocking data read requests.
 * the life span of this struct
 * need to outlast the call and its callback (if applicable)
 */
struct cash_qdma_request {
	unsigned long dev_hndl; 					//qdma dev handle 
	unsigned long qhndl;						//qdma queue handle
	struct pci_dev *pdev;
	size_t buffer_chunk_count;              	//how many buffer chunks, cannot be 0
	size_t buffer_chunk_size;               	//each chunk's byte size, cannot be 0
	void** buffer_chunks;                   	//a void* array of dimension == 
												//buffer_chunk_count, to hold the 
												//resulting allocated chunks,
												//user needs to initialized it to NULL
												//and never change them
	dma_addr_t* buffer_chunk_dma_hndls;     	//a dma_addr_t array of dimension ==
												//buffer_chunk_count, to hold the 
												//resulting chunk dma handles,
												//user needs to initialized it to NULL
												//and never change them
	unsigned long user_data; 	            	//filled by the caller and used by the 
												//caller in callback

	/*if the following is NULL, it is a blocking call, otherwise it holds the
	  callback function when this IO request is done*/
	void (*aio_done)(unsigned long user_data 	//see above
		, unsigned int bytes_done				//how many byte read
		, int err);								//error code if not zero
	struct qdma_request* reserved;          	//initialize to 0, then do not touch
	bool is_write;								//is wrting to board?
	u64 bram_addr;          					//MM only boardram address
} __attribute__((aligned(8)));

/******************************************************************************/
/**
 * prepare_cash_qdma_request() - prepare a request for data write or read use based 
 * on its data members, only need to call this once if no data member changes.
 * The IO buffer chunks are allocated in this function, the user needs to specify
 * the size and number of chunks
 * Only prepared cash_qdma_request can be used in 
 * send_cash_qdma_request
 *
 * @qreq: the request that needs to get prepared
 * @return	0 on success
 * @return	<0 on failure with negated Linux error code
 *****************************************************************************/
int prepare_cash_qdma_request(struct cash_qdma_request* qreq);

/******************************************************************************/
/**
 * unprepare_cash_qdma_request() - release the resources obtained by 
 * prepare_video_request including buffers
 *
 * @qreq: the request that needs to get teared down
 *****************************************************************************/
void unprepare_cash_qdma_request(struct cash_qdma_request* qreq);


/******************************************************************************/
/**
 * send_cash_qdma_request() - send out a cash qdma write/read request
 *
 * @qreq: pointer to a cash_qdma_request which need to be prepared using 
 * 			prepare_cash_qdma_request	
 * @return	0 on non-blocking submission success, IO byte count on blocking
 * @return	<0 on failure with negated Linux error code
 *****************************************************************************/
ssize_t send_cash_qdma_request(struct cash_qdma_request* qreq);

uint32_t read_cash_user_reg(unsigned long dev_hndl, uint32_t reg_addr);
int write_cash_user_reg(unsigned long dev_hndl, uint32_t reg_addr, uint32_t value);

/******************************************************************************/
/**
 * send_host_cmd() - send host cmd to FPGA - this also triggers the interrupt
 * so the FPGA knows a host cmd has come. It is a sync call and not threadsafe
 * due to cmds ae stored at the same IO area. 
 * @dev_hndl: qdma device handle
 * @cmd: pointer to a cmd struct - see cash_host_cmds.h
 * @cmd_size: the size of the above cmd
 * @wait_until_done: block the call until they are finished by FPGA 
 * and the caller can check the CASH_SCRATCH_REG_CMD_STATUS_ADDR[n] for the
 * n'th sub cmd status
 * @return	>=0 sent byte count
 * @return	<0 on failure with negated Linux error code
 *****************************************************************************/
ssize_t send_host_cmd(unsigned long dev_hndl
	, struct cash_host_cmd_t* cmd
	, size_t cmd_size
	, bool wait_until_done);

/******************************************************************************/
/**
 * get_fpga_event() - retrieve a FPGA event from the FPGA event queue
 * @dev_hndl: qdma device handle. It is a sync call that could block if
 * there is no event reported yet.
 * @fpga_event_qhndl: queue handle for FPGA event queue
 * @event: pointer to a 4kB buffer to hold an event
 * @return	>=0 read byte count, should always be 4kB
 * @return	<0 on failure with negated Linux error code
 *****************************************************************************/
// ssize_t get_fpga_event(unsigned long dev_hndl, struct cash_fpga_event_t* event);

#endif
