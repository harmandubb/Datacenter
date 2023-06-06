#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <linux/spi/spidev.h>


uint8_t mode = 0;
uint32_t speed = 20000000; //20 MHz
uint8_t bits = 8;

#define ECON1REG 0x1F
#define ECON2REG 0x1E
#define ESTATREG 0x1D
#define EIRREG 0x1C



/**
 * @brief Funciton makes a SPI transfer 
 * 
 * @param fd            File descriptor to SPU bus
 * @param data          Data array with output (write) data
 * @param length        Length of the data array 
 */

void spi_transfer(int fd, uint8_t *data, int length){
        struct spi_ioc_transfer spi[length];
        int i; 

        // Settup transfer struct 

        for (i=0; i<length; i++){
                memset(&spi[i], 0, sizeof(struct spi_ioc_transfer));
                spi[i].tx_buf = (unsigned long) (data+i);
                spi[i].rx_buf = (unsigned long) (data+i);
                spi[i].len = 1;
                spi[i].speed_hz = speed;
                spi[i].bits_per_word = bits;
        }

        // Lets do the transfer 
        if (ioctl(fd, SPI_IOC_MESSAGE(length), spi) < 0) {
                perror("Error trasnfering data over SPU bus");
                close(fd);
                exit(-1);
        }       
}


/**
 * @brief               Combines the op code and related arugement to be 1 byte to be trasnfered to spi bus
 * 
 * @param opCode        2 bit code used to start command for the module 
 * @param arguement     5 bits arguement code be combined with op code
*/
uint8_t createInstruction(uint8_t opCode, uint8_t argument){
        uint8_t output = ((opCode << 5) | argument);

        return output; 
}

/**
 * @brief               Read the contents of the desired reg
 * 
 * @param fd            File descriptor to SPU bus
 * @param regAddress    Desired address to read
 */
void readControlReg(int fd, uint8_t *data, uint8_t regAddress){
        *data = createInstruction(0, regAddress);
        spi_transfer(fd, data, 2);
}

/**
 * @brief               Writes some given data to the specified register 
 * 
 * @param fd            File descriptor to SPI bus
 * @param data          Memory to which the SPI transfer uses to store communication 
 * @param reg           Register address that is to be updated
 * @param input         Value that is to be written to the register
 * 
*/

void writeControlReg(int fd, uint8_t *data, uint8_t reg, uint8_t input){
        data[0] = createInstruction(2,reg);
        data[1] = input;

        spi_transfer(fd,data,2);
}

/**
 * @brief       Soft resets all registers in the the ethernet module    
 * 
 * @param fd    File descriptor to the SPI buss
 * 
*/
void resetCommand(int fd){
        uint8_t data[2] = {0}; 
        data[0] = 0xFF;
        spi_transfer(fd, data, 2);
}
/**
 * @brief               Send a message over ehternet module 
 * 
 * @param fd            File descriptor to the SPI Bus
 * @param message       The message that you want to write to be transfered
 * @param length        The number of byes of the message
*/
void writeBufferMemory(int fd, uint8_t *message, int length){
        uint8_t* data = (uint8_t*) malloc((length + 1)*sizeof(uint8_t));
        if(data == NULL){
                perror("Memory Allocation Failed while writing to buffer memory");
        } else {
                data[0] = 0x7A;
                for (int i = 0; i < length; i++){
                        data[i+1] = message[i];
                }
                spi_transfer(fd, data,length+1); //Check if the cs line needs to be change option needs to be changed because of the writing buffer needs to continuously write to the buffer
        }
}

/**
 * @brief               Read any incoming messages from the ethernet module 
 * 
 * @param fd            File descriptor to the SPI Bus
*/
void readBufferMemory(int fd, uint8_t *storage, int length){
        uint8_t* data = (uint8_t*) calloc(length+1,sizeof(uint8_t));
        if (data == NULL){
                perror("Memory Allocation Failed while reading buffer memory");
        } else {
                data[0] = 0x3A;
                spi_transfer(fd, data, length + 1); //Check if there needs to be a condition in the spi_transfer function that breaks from the loop somehow if the read is not giving anything for a significant amount of time. 
        }

        for (int i = 0; i < length; i++){
                storage[i] = data[i+1];
        }
        free(data);

}

void switchRegBank(int fd, uint8_t bankNum){
        uint8_t data[1] = {0};
        readControlReg(fd,data,0x1F);
        
        uint8_t econ1Val = data[0];

        econ1Val &= ~(3);
        econ1Val |= bankNum;

        data[0] = econ1Val;

        writeControlReg(fd,data,0x1F,econ1Val);
}

/**
 * @brief       Initialized the Ethernet module 
 * 
 * @param fd    File descriptor for SPI module
*/
void spiInitilization(int fd){
        uint8_t data[2] = {0};

        // Initializing the receive buffer bounds
        switchRegBank(fd,0);

        writeControlReg(fd,data,0x08,0x00); //ERXSTL
        writeControlReg(fd,data,0x09,0x08); //ERXSTH

        writeControlReg(fd,data,0x0A,0xFF); //ERXNDL
        writeControlReg(fd,data,0x09,0x1F); //ERXSTH

        uint8_t estatVal = 0;

        // Wait for OST to do any changing of mac and phy registers
        do {
                readControlReg(fd,data,0x1D);

                estatVal = data[0];
        }
        while(!(estatVal & 1));

        // Mac address

        switchRegBank(fd, 2);
        readControlReg(fd,data,0x00); //check if the mac codes are being read properly
        uint8_t macon1Val = data[1];

        macon1Val |= 1;
        data[0] = macon1Val;
        data[1] = 0; //Check how this impacts the performance of the system

        writeControlReg(fd,data,0x00,macon1Val);

        //ethernet packet config 
        readControlReg(fd, data, 0x02);
        uint8_t macon3Val = data[1];

        macon3Val &= ~(1); // clearing FULDPX
        macon3Val |= 0xF2; //setting PADCFG, TXCRCEN, FRMLNEN

        writeControlReg(fd,data,0x02,macon3Val);

        //Configuring Macon4
        readControlReg(fd, data, 0x03);
        uint8_t macon4Val = data[1];

        macon4Val |= 0x40;

        writeControlReg(fd,data,0x03,macon4Val);

        //Program max frame
        writeControlReg(fd,data,0x0A,0xEE);
        writeControlReg(fd,data,0x0B,0x05);

        //configure Back-to-back inter-packet gap register
        writeControlReg(fd,data,0x04,0x12); //used recommended settings of 12h as in the document

        //configure non-back-to-back register 
        writeControlReg(fd,data,0x06,0x0C); //page 34 point 7

        //configure local mac address
        switchRegBank(fd,3);
        writeControlReg(fd,data,0x04,0xCF); //Least Significant
        writeControlReg(fd,data,0x05,0xAF);
        writeControlReg(fd,data,0x02,0xB0);
        writeControlReg(fd,data,0x03,0xDF);
        writeControlReg(fd,data,0x00,0x63);
        writeControlReg(fd,data,0x01,0x88); //Most Significant
}

void splitByte(uint8_t byte, uint8_t* split){
        split[0] = (byte & 0xF0) >> 4; 
        split[1] = byte & 0x0F;
}

uint16_t transmitPacket(int fd, uint16_t transmitStartPtr, uint8_t* srcMacAddress, char message[]){
        uint8_t split[2];
        uint8_t data[2] = {0};
        //Set ETXST
        splitByte(transmitStartPtr, split);
        switchRegBank(fd,0);
        writeControlReg(fd, data,0x04,split[1]);
        writeControlReg(fd,data,0x05,split[0]);

        //Writing the control byte
        data[0] = 0x0E;
        writeBufferMemory(fd,data,1);

        //Writing the Destination address
        //Send FF 6 times for a broadcast send out
        data[0] = 0xFF;
        for (int i = 0; i < 6; i++){
                writeBufferMemory(fd,data,1);
        }

        //Writing the Source Mac Address 
        writeBufferMemory(fd,srcMacAddress,6);


        //Type/length input here : Typically length is sent due to being less than 1500 in size
        int messageSize = strlen(message);

        uint8_t lengthL = messageSize & 0x00FF;
        uint8_t lengthH = (messageSize & 0xFF00) >> 8;

        data[0] = lengthH;
        writeBufferMemory(fd,data,1);
        data[0] = lengthL;
        writeBufferMemory(fd,data,1);

        //Data to be sent
        writeBufferMemory(fd,message,messageSize);


        //transmit end ptr set 
        uint16_t transmitEndPtr = transmitStartPtr + 1 + 6 + 6 + 6 + 2 + messageSize; //transmitStartPtr + control byte + Dest Mac + Src Mac + Length/Type (2 byte) + message length 

        splitByte(transmitEndPtr, split);

        //Set the ETXND ptr
        switchRegBank(fd,0);
        writeControlReg(fd,data,0x06,split[1]);
        writeControlReg(fd,data,0x07,split[0]);

        //Clear EIR.TXIF
        readControlReg(fd, data, 0x1C);
        uint8_t TXIF_Val = data[0];
        TXIF_Val &= ~(0x08);
        writeControlReg(fd,data,0x1C,TXIF_Val);

        //Set EIE.TXIW and EIE.INTIE
        readControlReg(fd, data,0x1B);
        uint8_t EIE_Val = data[0];
        EIE_Val |= 0x88;
        writeControlReg(fd,data,0x1B,EIE_Val);

        //Set ECON1.TXRTS 
        readControlReg(fd,data,0x1F);
        uint8_t ECON1_Val = data[0];
        ECON1_Val |= 0x08;
        writeControlReg(fd,data,0x1F,ECON1_Val);

        return transmitEndPtr;
}

bool checkTransmission(int fd, uint8_t transmitEndPtr){
        uint8_t data[7] = {0};
        uint8_t split[2];
        uint8_t TXRTS_Cleared = 0;

        
        readControlReg(fd, data, ECON1REG);
        TXRTS_Cleared = (data[0] & 0x08) == 0;

        if (!TXRTS_Cleared){
                return false;
        }

        //Check if EIR.TXIF is set
        //All good if set
        readControlReg(fd, data, EIRREG);
        uint8_t EIR_Val = data[0];
        uint8_t TXIF_Set = (EIR_Val & 0x08) == 1;

        if (!TXIF_Set){
                return false;
        }

        //Check if ESTAT.TXABRT is cleared 
        //All good if not set 
        readControlReg(fd, data, ESTATREG);
        uint8_t ESTAT_Val = data[0];
        uint8_t TXABRT_Cleared = (ESTAT_Val & 0x02) == 0;

        if (!TXABRT_Cleared){
                return false;
        }

        
        //Reading the tranmissiotn status vector 
        switchRegBank(fd,0);
        splitByte(transmitEndPtr+1, split);
        //put the ERDPT at the start of the trasmission status vector
        writeControlReg(fd,data,0x00,split[1]);
        writeControlReg(fd,data,0x01,split[0]);

        //Enable autoincrementing in the ECON2 register
        readControlReg(fd,data,ECON2REG);
        uint8_t ECON3_Val = data[0];
        ECON3_Val |= 0x80;
        writeControlReg(fd,data,ECON2REG,ECON3_Val);

        // Read 8 bytes of data which is the transmission status vector
        readBufferMemory(fd,data,7);

        printf("Transmission Status Vector: (lowest byte to highest) \n");
        for (int i = 0; i < 7; i++){
                printf("%02X\n", data[i]);
        }

        //good to do a actual test at this point. 
}



int main(int argc, char * argv[]){
        int fd; 
        uint8_t data[2] = {0};

        // Opent he SPI bus file
        fd = open ("/dev/spidev0.0", O_RDWR);

        if (fd < 0){
                perror("Error opening SPI Buss");
                return -1;
        }

        // Set up of the SPI Buss
        if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0){
                perror("Error setting the SPI mode");
                close(fd);
                return -1;
        }

        if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0){
                perror("Error setting the SPI speed");
                close(fd);
                return -1;
        }

        if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0){
                perror("Error setting the word length");
                close(fd);
                return -1;
        }

        resetCommand(fd);

        spiInitilization(fd);

        uint8_t srcMacAddress[6] = {0x00,0x00,0x00,0x00,0x00,0x00};

        uint16_t endTransmitPtr = transmitPacket(fd,0x0000,srcMacAddress,"Hello World");

        bool success = checkTransmission(fd,endTransmitPtr);

        if (success){
                printf("Transmission Successful\n");
        }else {
                printf("Transmission Failed\n");
        }
        
        close(fd);
        return 0; 
}
