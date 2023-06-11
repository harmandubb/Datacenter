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
#define EIEREG 0x1B



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

        // switchRegBank(fd,2);
        // checkReg(fd,0x03, "MACON4");

        //ethernet packet config   
        readControlReg(fd, data, 0x02);
        uint8_t macon3Val = data[1];

        macon3Val &= ~(1); // clearing FULDPX
        // macon3Val |= 0xF2; //setting PADCFG, TXCRCEN, FRMLNEN
        macon3Val |= 0xF0; //setting PADCFG, TXCRCEN

        writeControlReg(fd,data,0x02,macon3Val);

        //Configuring Macon4
        switchRegBank(fd,2);
        writeControlReg(fd,data,0x03,0x40);

        //Program max frame -- default recomendded is 1518 bytes
        writeControlReg(fd,data,0x0A,0xEE);
        // checkReg(fd,0x0A, "MAMXFLL");
        writeControlReg(fd,data,0x0B,0x05);
        // checkReg(fd,0x0B, "MAMXFLH");

        //configure Back-to-back inter-packet gap register
        writeControlReg(fd,data,0x04,0x12); //used recommended settings of 12h as in the document
        // checkReg(fd,0x04, "MABBIPG");

        //configure non-back-to-back register 
        writeControlReg(fd,data,0x06,0x0C); //page 34 point 7
        // checkReg(fd,0x06,"MAIPGH");

        //configure local mac address
        switchRegBank(fd,3);
        writeControlReg(fd,data,0x04,0xCE); //Least Significant
        writeControlReg(fd,data,0x05,0xAF);
        writeControlReg(fd,data,0x02,0xB0);
        writeControlReg(fd,data,0x03,0xDF);
        writeControlReg(fd,data,0x00,0x63);
        writeControlReg(fd,data,0x01,0x88); //Most Significant
        //Imac's mac address: 88:63:df:b0:af:cf
        // made up mac address for the module: 88:63:df:b0:af:ce
        //order of mac address registers has been confrimed 
}

void splitByte(uint8_t byte, uint8_t* split){
        split[0] = (byte & 0xF0) >> 4; 
        split[1] = byte & 0x0F;
}

void splitWord(uint16_t word, uint8_t* split){
        split[0] = (word & 0xFF00) >> 8;
        split[1] = word & 0x00FF;
}

void checkReg(int fd, uint8_t reg, char name[]){
        uint8_t data[2] = {0};
        readControlReg(fd,data,reg);
        puts(name);
        printf("The value of the register is %.2X\n",data[1]);
}

uint16_t transmitPacket(int fd, uint16_t transmitStartPtr, uint8_t* srcMacAddress, char message[]){
        uint8_t split[2];
        uint8_t data[2] = {0};
        //Set ETXST
        splitWord(transmitStartPtr, split);
        switchRegBank(fd,0);
        writeControlReg(fd, data,0x04,split[1]);
        checkReg(fd,0x04,"ETXSTL");
        writeControlReg(fd,data,0x05,split[0]);
        checkReg(fd,0x05,"ETXSTH");

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

        messageSize = strlen(message);

        //Data to be sent
        writeBufferMemory(fd,message,messageSize);


        //transmit end ptr set 
        uint16_t transmitEndPtr = transmitStartPtr + 1 + 6 + 6 + 2 + messageSize-1; //transmitStartPtr + control byte + Dest Mac + Src Mac + Length/Type (2 byte) + message length 

        splitWord(transmitEndPtr, split);

        //Set the ETXND ptr
        switchRegBank(fd,0);
        writeControlReg(fd,data,0x06,split[1]);
        checkReg(fd,0x06,"ETXNDL");
        writeControlReg(fd,data,0x07,split[0]);
        checkReg(fd,0x07,"ETXNDH");
        //The logic for the end of the pointer checks out

        //Clear EIR.TXIF
        readControlReg(fd, data, EIRREG);
        uint8_t TXIF_Val = data[1];
        TXIF_Val &= ~(0x08) + ~(0x20);
        writeControlReg(fd,data,EIRREG,TXIF_Val);
        checkReg(fd,EIRREG,"EIR"); //---- 0---

        //Set EIE.TXIE and EIE.INTIE
        readControlReg(fd, data,EIEREG);
        uint8_t EIE_Val = data[1];
        EIE_Val |= 0x8A;
        writeControlReg(fd,data,EIEREG,EIE_Val);
        checkReg(fd,EIEREG, "EIE"); //1000 1010 or 8A

        //Set ECON1.TXRTS 
        readControlReg(fd,data,ECON1REG);
        uint8_t ECON1_Val = data[1];
        ECON1_Val &= ~(0xC0);
        ECON1_Val |= 0x08;
        writeControlReg(fd,data,ECON1REG,ECON1_Val);

        checkReg(fd,ECON1REG, "ECON1"); //The TRTS bit clears before another read can be done

        return transmitEndPtr;
}

void readBuffer(int fd, uint16_t startReadPtr, uint8_t* dataStorage, int dataLen){
        uint8_t split[2];
        uint8_t data[2] = {0};

        switchRegBank(fd,0);
        splitWord(startReadPtr, split);
        //put the ERDPT at the start of the trasmission status vector
        writeControlReg(fd,data,0x00,split[1]);
        writeControlReg(fd,data,0x01,split[0]);

        //Enable autoincrementing in the ECON2 register
        readControlReg(fd,data,ECON2REG);
        uint8_t ECON3_Val = data[1];
        ECON3_Val |= 0x80;
        writeControlReg(fd,data,ECON2REG,ECON3_Val);

        // Read 8 bytes of data which is the transmission status vector
        readBufferMemory(fd,dataStorage,dataLen);
}

void readTransmissionMessageBuffer(int fd, uint8_t transmitStartPtr, char message[]){
        uint8_t split[2];
        int messageLen = strlen(message);
        uint8_t* data = (uint8_t*) calloc(messageLen, sizeof(uint8_t));
        int messageOffset = 15;

        readBuffer(fd, transmitStartPtr+messageOffset, data, messageLen);

        printf("Transmission Message: \n");
        for (int i = 0; i < messageLen; i++){
                printf("%02X ", data[i]);
        }
        printf("\n");

        free(data);
}

void readTransmissionPacket(int fd, uint8_t transmitStartPtr, char message[]){
        uint8_t split[2];
        int transmissionPacketBaseSize = 15;
        int statusVectorSize = 7;
        int messageLen = strlen(message);
        int packetSize = messageLen+transmissionPacketBaseSize+statusVectorSize;
        uint8_t* data = (uint8_t*) calloc(packetSize, sizeof(uint8_t));
        int messageOffset = 15;

        readBuffer(fd, transmitStartPtr, data, packetSize);

        printf("Transmission Packet: \n");
        for (int i = 0; i < packetSize; i++){
                printf("%02X ", data[i]);
        }
        printf("\n");

        free(data);
}

bool checkTransmission(int fd, uint8_t transmitEndPtr){
        uint8_t data[7] = {0};
        uint8_t split[2];
        uint8_t TXRTS_Cleared = 0;
        int statusVectorOffset = 0;
        bool success = true;

        
        readControlReg(fd, data, ECON1REG);
        TXRTS_Cleared = (data[1] & 0x08) == 0;

        if (!TXRTS_Cleared){
                success = false;
        }

        //Check if ESTAT.TXABRT is cleared 
        //All good if not set 
        readControlReg(fd, data, ESTATREG);
        uint8_t ESTAT_Val = data[1];
        uint8_t TXABRT_Cleared = (ESTAT_Val & 0x02) == 0;

        if (!TXABRT_Cleared){
                success = false;
        }

        //Check if EIR.TXIF is set
        //All good if set
        readControlReg(fd, data, EIRREG);
        uint8_t EIR_Val = data[1];
        uint8_t TXIF_Set = (EIR_Val & 0x08) == 1;

        if (!TXIF_Set){
                success = false;
        }

        //Check EIE register to see if transimision abort occured
        readControlReg(fd, data, EIEREG);
        uint8_t EIE_Val = data[1];

        

        //Reading the tranmissiotn status vector 
        switchRegBank(fd,0);
        splitWord(transmitEndPtr+statusVectorOffset, split);
        //put the ERDPT at the start of the trasmission status vector
        writeControlReg(fd,data,0x00,split[1]);
        writeControlReg(fd,data,0x01,split[0]);

        //Enable autoincrementing in the ECON2 register
        readControlReg(fd,data,ECON2REG);
        uint8_t ECON3_Val = data[1];
        ECON3_Val |= 0x80;
        writeControlReg(fd,data,ECON2REG,ECON3_Val);

        // Read 8 bytes of data which is the transmission status vector
        readBufferMemory(fd,data,7);

        // q: if the transmission is successful, the first byte of the transmission status vector should be 0x00?
        // a: yes, if the transmission is successful, the first byte of the transmission status vector should be 0x00

        // q: what does it mean for multi-byte fiels to be written in little-endian format?
        // a: little endian means that the least significant byte is stored first.

        // q: in the above data array and knowing the bytes are returned in little endian format what would the data[0] give me in terms of bit designation?
        // a: data[0] would give you the lowest byte of the transmission status vector

        printf("Transmission Status Vector: (lowest byte to highest) \n");
        for (int i = 0; i < 7; i++){
                printf("%02X ", data[i]);
        }
        printf("\n");

        return success;

        //good to do a actual test at this point. 
}



int main(int argc, char * argv[]){
        int fd; 
        uint8_t data[2] = {0};
        bool success = false; 

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

        //88:63:df:b0:af:ce
        uint8_t srcMacAddress[6] = {0x88,0x63,0xDF,0xB0,0xAF,0xCE};



        uint16_t endTransmitPtr = transmitPacket(fd,0x0000,srcMacAddress,"Hello World");

        readTransmissionPacket(fd,0x0000,"Hello World");
        // readTransmissionMessageBuffer(fd,0x0000,"Hello World");

        success = checkTransmission(fd,endTransmitPtr);

        if (success){
                printf("Transmission Successful\n");
        }else {
                printf("Transmission Failed\n");
        }
        
        close(fd);
        return 0; 
}
