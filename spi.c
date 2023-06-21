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

#define BUFFERSIZE 0x1FFF
#define RECEIVEBUFFERSTART 0x0800


#define ECON1REG 0x1F
#define ECON2REG 0x1E
#define ESTATREG 0x1D
#define EIRREG 0x1C
#define EIEREG 0x1B
#define EPKTCNTREG 0x19
#define ERDPTLREG 0x00
#define ERDPTHREG 0x01
#define ETXSTLREG 0x04
#define ETXSTHREG 0x05
#define ETXNDLREG 0x06
#define ETXNDHREG 0x07
#define EWRPTLREG 0x02
#define EWRPTHREG 0x03
#define ERXSTLREG 0x08
#define ERXSTHREG 0x09
#define ERXNDLREG 0x0A
#define ERXNDHREG 0x0B
#define MACON1REG 0x00
#define MACON3REG 0x02
#define MACON4REG 0x03
#define MAMXFLLREG 0x0A
#define MAMXFLHREG 0x0B
#define MABBIPGREG 0x04
#define MAIPGLREG 0x06 
#define EPKTCNTREG 0x19
#define IPV4TYPE 0x0800
#define ADDRESSRESOLUTIONPROTOCOL 0x0806
#define IPV6TYPE 0x86DD

void checkReg(int fd, uint8_t reg, char name[]);
void splitByte(uint8_t byte, uint8_t* split);
void splitWord(uint16_t word, uint8_t* split);
uint8_t* IPV4PacketCreator(uint8_t payload[], uint8_t srcIPAddress[], uint8_t destIPAddress[], int* packetLen);
uint16_t IPV4CheckSumCalculate(uint8_t version, uint8_t headerLength, uint8_t typeOfService, uint16_t totalLength, uint16_t identification, uint8_t flags, uint16_t fragmentOffset, uint8_t timeToLive, uint8_t Protocol, uint8_t srcIPAddress[], uint8_t destIPAddress[]);
uint8_t* TCPHeaderCreator(uint16_t srcPort, uint16_t destPort, uint32_t sequenceNum, uint32_t ackNum, uint8_t* data, int dataLen, int* TCPHeaderLen);
uint16_t TCPCheckSumCalculator(uint8_t* TCPHeader, int* TCPHeaderLen);

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

void bitSet(int fd, uint8_t *data, uint8_t reg, uint8_t input){
        data[0] = createInstruction(4,reg);
        data[1] = input;

        spi_transfer(fd,data,2);
}

void bitClear(int fd, uint8_t *data, uint8_t reg, uint8_t input){
        data[0] = createInstruction(5,reg);
        data[1] = input;

        spi_transfer(fd,data,2);
}

/**
 * @brief       Soft resets all registers in the the ethernet module    
 * 
 * @param fd    File descriptor to the SPI buss
 * 
*/
void systemReset(int fd){
        uint8_t data[2] = {0}; 
        data[0] = 0xFF;
        spi_transfer(fd, data, 2);
}

void transmitReset(int fd){
        uint8_t data[2] = {0};
        data[0] = 0x80;
        writeControlReg(fd,data,ECON1REG,0x80);
}

void receiveReset(int fd){
        uint8_t data[2] = {0};
        data[0] = 0x40;
        writeControlReg(fd,data,ECON1REG,0x40);
}

void resetCommand(int fd){
        uint8_t data[2] = {0};
        systemReset(fd);
        transmitReset(fd);
        receiveReset(fd);

        //Reset all interupt enable bits
        bitClear(fd,data,EIEREG,0xFF);

        //check interupt enable register
        checkReg(fd,EIEREG, "EIE");

        //Reset all interupt flags 
        bitClear(fd,data,EIRREG,0xFF);

        //check interupt flag register
        checkReg(fd,EIRREG, "EIR");

}

void enableInterrupts(int fd){
        uint8_t data[2] = {0};


        //Enable Global interupt bit 
        bitSet(fd,data,EIEREG,0x80);

        //Enable interupts for packet transmit and transmit error
        bitSet(fd, data, EIEREG, 0x02+0x08);

        //Enable interupts for packet receive and receive error 
        bitSet(fd, data, EIEREG, 0x01+0x40);

        //Check interupt enable register
        checkReg(fd,EIEREG, "EIE");

        checkReg(fd, EIRREG, "EIR");
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
        
        uint8_t ECON1_Val = data[0];

        ECON1_Val &= ~(3);
        ECON1_Val |= bankNum;

        data[0] = ECON1_Val;

        writeControlReg(fd,data,0x1F,ECON1_Val);
}

/**
 * @brief       Initialized the Ethernet module 
 * 
 * @param fd    File descriptor for SPI module
*/
void spiInitilization(int fd, uint8_t macAddressLocal[6]){
        uint8_t data[2] = {0};
        uint8_t split[2] = {0};

        checkReg(fd, EIRREG, "EIR");


        // Initializing the receive buffer bounds
        switchRegBank(fd,0);

        writeControlReg(fd,data,ERXSTLREG,0x00); //ERXSTL
        writeControlReg(fd,data,ERXSTHREG,0x08); //ERXSTH

        writeControlReg(fd,data,ERXNDLREG,0xFF); //ERXNDL
        writeControlReg(fd,data,ERXNDHREG,0x1F); //ERXNDH

        uint8_t ESTAT_Val = 0;

        // Wait for OST to do any changing of mac and phy registers
        do {
                readControlReg(fd,data,0x1D);

                ESTAT_Val = data[1];
        }
        while(!(ESTAT_Val & 1));

        // Mac address
        switchRegBank(fd, 2);

        //set MARXEN bit
        bitSet(fd,data,MACON1REG,0x01);

        checkReg(fd, MACON1REG, "MACON1");

        //ethernet packet config   
        readControlReg(fd, data, MACON3REG);
        uint8_t MACON3_Val = data[1];

        MACON3_Val = 0xF2; //includes padding
        // MACON3_Val = 0x12; //excludes padding

        writeControlReg(fd,data,MACON3REG,MACON3_Val);

        //Configuring Macon4
        switchRegBank(fd,2);
        writeControlReg(fd,data,MACON4REG,0x40);// only enables Deffer but if back pressure error occurs please revisit this

        //Program max frame -- default recomendded is 1518 bytes
        splitWord(1518,split);
        writeControlReg(fd,data,MAMXFLLREG,split[1]);
        // checkReg(fd,0x0A, "MAMXFLL");
        writeControlReg(fd,data,MAMXFLHREG,split[0]);
        // checkReg(fd,0x0B, "MAMXFLH");

        //configure Back-to-back inter-packet gap register
        writeControlReg(fd,data,MABBIPGREG,0x12); //used recommended settings of 12h as in the document
        // checkReg(fd,0x04, "MABBIPG");

        //configure non-back-to-back register 
        writeControlReg(fd,data,MAIPGLREG,0x0C); //page 34 point 7
        // checkReg(fd,0x06,"MAIPGH");

        checkReg(fd, EIRREG, "EIR");

        //configure local mac address
        switchRegBank(fd,3);
        writeControlReg(fd,data,0x04,macAddressLocal[5]); //Least Significant
        writeControlReg(fd,data,0x05,macAddressLocal[4]);
        writeControlReg(fd,data,0x02,macAddressLocal[3]);
        writeControlReg(fd,data,0x03,macAddressLocal[2]);
        writeControlReg(fd,data,0x00,macAddressLocal[1]);
        writeControlReg(fd,data,0x01,macAddressLocal[0]); //Most Significant
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
        printf("%s = %.2X\n",name, data[1]);
}

void clearInterupt(int fd, uint8_t interuptBit){
        uint8_t data[2] = {0};
        //clear the global enable 
        bitClear(fd,data,EIEREG,0x80);
        //clear the interupt flag
        bitClear(fd,data,EIRREG,interuptBit);
        //set the global enable
        bitSet(fd,data,EIEREG,0x80);
}

uint16_t transmitPacket(int fd, uint16_t* trasmitStartPtr, uint8_t srcMACAddress[], uint8_t* message, int messageLen){
        uint8_t data[2] = {0};
        uint8_t split[2];

        int controlByteLen = 1;
        int destMACAddressLen = 6;
        int srcMACAddressLen = 6;
        int typeLen = 2;

        // checkReg(fd, EIRREG, "EIR");

        //set the trasmit start pointer 
        splitWord(*trasmitStartPtr, split);
        switchRegBank(fd, 0);
        writeControlReg(fd, data, ETXSTLREG, split[1]);
        writeControlReg(fd, data, ETXSTHREG, split[0]);

        // checkReg(fd, EIRREG, "EIR");

        // Write a control byte
        data[0] = 0x0E;
        writeBufferMemory(fd, data, 1);

        // checkReg(fd, EIRREG, "EIR");

        // destination address 
        uint8_t destMACAddress[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        writeBufferMemory(fd, destMACAddress, 6);

        // checkReg(fd, EIRREG, "EIR");

        //source address        
        writeBufferMemory(fd, srcMACAddress, 6);

        // checkReg(fd, EIRREG, "EIR");

        //type/length
        splitWord(IPV4TYPE, split);

        writeBufferMemory(fd, split, 2);

        // checkReg(fd, EIRREG, "EIR");

        //message
        writeBufferMemory(fd, message, messageLen);

        // checkReg(fd, EIRREG, "EIR");

        //set the trasmit end pointer
        uint16_t transmitEndPtr = *trasmitStartPtr + controlByteLen + destMACAddressLen + srcMACAddressLen + typeLen + messageLen - 1;
        splitWord(transmitEndPtr, split);
        switchRegBank(fd, 0);
        writeControlReg(fd, data, ETXNDLREG, split[1]);
        writeControlReg(fd, data, ETXNDHREG, split[0]);

        // checkReg(fd, EIRREG, "EIR");

        // Clear EIR.TXIF
        bitClear(fd, data, EIRREG, 0x08);
        // checkReg(fd, EIRREG, "EIR");

        //set EIE.TXIE and EIE.INTIE
        bitSet(fd, data, EIEREG, 0x88);
        // checkReg(fd, EIEREG, "EIE");

        // checkReg(fd, ECON1REG, "ECON1");
        
        //Clear the transmit reset and receive reset bits
        bitClear(fd, data, ECON1REG, 0x80 + 0x40);

        //start the transmission --> set ECON1.TXRTS
        
        bitSet(fd, data, ECON1REG, 0x08);
        checkReg(fd, ECON1REG, "ECON1");

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

void readTransmissionPacket(int fd, uint8_t transmitStartPtr, int messageLen){
        uint8_t split[2];
        int transmissionPacketBaseSize = 15;
        int statusVectorSize = 7;
        int packetSize = messageLen+transmissionPacketBaseSize+statusVectorSize;
        uint8_t* data = (uint8_t*) calloc(packetSize, sizeof(uint8_t));


        readBuffer(fd, transmitStartPtr, data, packetSize);

        printf("Transmission Packet: \n");
        for (int i = 0; i < packetSize; i++){
                printf("%02X ", data[i]);
        }
        printf("\n");

        free(data);
}
 
bool verifyTransmission(int fd, uint8_t transmitEndPtr){
        int statusVectorSize = 7;
        uint8_t data[7] = {0};
        uint8_t split[2];

        bool success = true; 

        //Check if ECON1.TXRTS is cleared 
        readControlReg(fd, data, ECON1REG);
        uint8_t ECON1_Val = data[1];
        if (ECON1_Val & 0x08){
                printf("ECON1.TXRTS is not cleared\n");
                success = false;
        }

        //set the read pointer to read the status vector 
        splitWord(transmitEndPtr+1, split);
        switchRegBank(fd,0);
        writeControlReg(fd,data,ERDPTLREG,split[1]);
        writeControlReg(fd,data,ERDPTHREG,split[0]);

        //read the status vector
        readBufferMemory(fd,data,statusVectorSize);

        //check if EIR.TXIF interrupt flag is set
        readControlReg(fd,data,EIRREG);
        uint8_t EIR_Val = data[1];

        if (!(EIR_Val & 0x08)){
                printf("EIR.TXIF interrupt flag is not set\n");
                success = false;
        }

        //clear the EIR.TXIF interrupt flag
        clearInterupt(fd, 0x08);

        // checkReg(fd, EIRREG, "EIR");

        //check if ESTAT.TXABRT is clear
        readControlReg(fd,data,ESTATREG);
        uint8_t ESTAT_Val = data[1];

        if (ESTAT_Val & 0x02){
                printf("ESTAT.TXABRT is set\n");
                success = false;

                //interrogate ESTAT.LATECOL
                if(ESTAT_Val & 0x10){
                        printf("ESTAT.LATECOL is set\n");
                }

        }

        //print the contents of the status vector 
        printf("Status Vector: \n");
        for (int i = 0; i < statusVectorSize; i++){
                printf("%02X ", data[i]);
        }
        printf("\n");

        return success;
}

void enableReceive(int fd){
        uint8_t data[2] = {0};
        bitSet(fd,data,ECON1REG,0x04);

        //at this point the receive ptrs should not be modified until the receive is complete
}

bool receivePacketAvailable(int fd){
        bool available = false; 
        uint8_t data[2] = {0};

        switchRegBank(fd,1);
        readControlReg(fd,data,EPKTCNTREG);

        uint8_t EPKTCNT_Val = data[1];

        if (EPKTCNT_Val > 0){
                available = true;
        }

        return available;
}

uint16_t mergeWord(uint8_t highByte, uint8_t lowByte){
        uint16_t merged = 0;
        merged = (highByte << 8) | lowByte;
        return merged;
}

uint8_t* serviceReceivePacket(int fd, uint16_t* packetPtr){
        uint8_t data[14] = {0};
        uint8_t split[2];
        uint8_t crcData[4] = {0};

        //set the read pointer to the start of the receive packet 
        splitWord(*packetPtr, split);
        switchRegBank(fd,0);
        writeControlReg(fd,data,ERDPTLREG,split[1]);
        writeControlReg(fd,data,ERDPTHREG,split[0]);

        //read the first two byte to get the next packet pointer 
        readBufferMemory(fd,data,2);
        *packetPtr = mergeWord(data[1],data[0]);

        //read the receive status Vector 
        readBufferMemory(fd,data,7);

        //print the contents of the status vector
        printf("Receive Status Vector: \n");
        for (int i = 0; i < 7; i++){
                printf("%02X ", data[i]);
        }
        printf("\n");

        //read the receive headers at the next 14 bytes
        readBufferMemory(fd,data,14);

        //extract the length of the message 
        uint16_t messageLen = mergeWord(data[12],data[13]);

        //malloc space for the message data
        uint8_t* message = (uint8_t*) calloc(messageLen, sizeof(uint8_t));
        //read the message
        readBufferMemory(fd,message,messageLen);

        //read in the crc data 
        readBufferMemory(fd,crcData,4);

        return message;
}

uint8_t* IPV4PacketCreator(uint8_t payload[], uint8_t srcIPAddress[], uint8_t destIPAddress[], int* packetLen){
        uint16_t srcPort = 80;
        uint16_t destPort = 22;
        uint32_t seqNum = 0;
        uint32_t ackNum = 0;
        int TCPPacketLen = 0;
        uint8_t TCPPacket = TCPHeaderCreator(srcPort, destPort, seqNum,ackNum, payload, strlen(payload), &TCPPacketLen);

        *packetLen = 20 + TCPPacketLen;

        uint8_t* packet = (uint8_t*) calloc(*packetLen, sizeof(uint8_t));
        int dataSegmentDivide = 1400;

        uint8_t version = 4;
        uint8_t headerLength = 5; 
        uint8_t typeOfService = 0;
        uint16_t totalLength = *packetLen;

        if (totalLength > dataSegmentDivide){
                printf("Payload is too large\n");
        }       

        uint16_t identification = 0;
        uint8_t flags = 0;
        uint16_t fragmentOffset = 0; //num bytes divided by 8 

        uint8_t timeToLive = 64;
        uint8_t protocol = 6; //TCP

        uint16_t checksum = IPV4CheckSumCalculate(version, headerLength, typeOfService, totalLength, identification, flags, fragmentOffset, timeToLive, protocol, srcIPAddress, destIPAddress);

        packet[0] = (version << 4) | headerLength;
        packet[1] = typeOfService;
        packet[2] = totalLength >> 8;
        packet[3] = totalLength & 0xFF;
        packet[4] = identification >> 8;
        packet[5] = identification & 0xFF;
        packet[6] = (flags << 5) | fragmentOffset >> 8;
        packet[7] = fragmentOffset & 0xFF;
        packet[8] = timeToLive;
        packet[9] = protocol;
        packet[10] = checksum >> 8;
        packet[11] = checksum & 0xFF;
        packet[12] = srcIPAddress[0];
        packet[13] = srcIPAddress[1];
        packet[14] = srcIPAddress[2];
        packet[15] = srcIPAddress[3];
        packet[16] = destIPAddress[0];
        packet[17] = destIPAddress[1];
        packet[18] = destIPAddress[2];
        packet[19] = destIPAddress[3];

        

        

        for(int i = 0; i < TCPPacketLen; i++){
                packet[20+i] = payload[i];
        }

        printf("IPV4 Packet: \n");
        for (int i = 0; i< *packetLen; i++){
                printf("%02X ", packet[i]);
        }
        printf("\n");

        return packet;
}

uint16_t IPV4CheckSumCalculate(uint8_t version, uint8_t headerLength, uint8_t typeOfService, uint16_t totalLength, uint16_t identification, uint8_t flags, uint16_t fragmentOffset, uint8_t timeToLive, uint8_t Protocol, uint8_t srcIPAddress[], uint8_t destIPAddress[]){
        uint16_t words[9] = {0};
        words[0] = (version << 12) | (headerLength << 8) | typeOfService;
        words[1] = totalLength;
        words[2] = identification;
        words[3] = (flags << 13) | fragmentOffset;
        words[4] = (timeToLive << 8) | Protocol;
        words[5] = srcIPAddress[0] << 8 | srcIPAddress[1];
        words[6] = srcIPAddress[2] << 8 | srcIPAddress[3];
        words[7] = destIPAddress[0] << 8 | destIPAddress[1];
        words[8] = destIPAddress[2] << 8 | destIPAddress[3];

        uint32_t sum = 0;
        for (int i = 0; i < 9; i++){
                sum += words[i];
        }

        //folding the 32 bit number into a 16 bit number 
        uint16_t checksum = (sum >> 16) + (sum & 0xFFFF);
        checksum = ~checksum;

        printf("Checksum: %04X\n", checksum);

        return checksum;
}

uint8_t* TCPHeaderCreator(uint16_t srcPort, uint16_t destPort, uint32_t sequenceNum, uint32_t ackNum, uint8_t* data, int dataLen, int* TCPHeaderLen){
        *TCPHeaderLen = 20+dataLen+dataLen%2;
        uint8_t* TCPHeader = (uint8_t*) calloc(*TCPHeaderLen, sizeof(uint8_t));
        
        bool CWR = false; //Congestion window reduced
        bool ECE = false; //Echo 
        bool URG = false; //Urgent Point active 
        bool ACK = false; //Acknowledgement field is to be read
        bool PSH = false; //Push function
        bool RST = false; //Reset the connection
        bool SYN = true; //Synchronize sequence numbers: active on the first packet
        bool FIN = true; //Last packet from sender 

        TCPHeader[0] = srcPort >> 8;
        TCPHeader[1] = srcPort & 0xFF;
        TCPHeader[2] = destPort >> 8;
        TCPHeader[3] = destPort & 0xFF;
        TCPHeader[4] = sequenceNum >> 24;
        TCPHeader[5] = (sequenceNum >> 16) & 0xFF;
        TCPHeader[6] = (sequenceNum >> 8) & 0xFF;
        TCPHeader[7] = sequenceNum & 0xFF;
        TCPHeader[8] = ackNum >> 24;
        TCPHeader[9] = (ackNum >> 16) & 0xFF;
        TCPHeader[10] = (ackNum >> 8) & 0xFF;
        TCPHeader[11] = ackNum & 0xFF;
        TCPHeader[12] = 5 << 4;
        TCPHeader[13] = (uint8_t) CWR << 7 | (uint8_t) ECE << 6 | (uint8_t) URG << 5 | (uint8_t) ACK << 4 | (uint8_t) PSH << 3 | (uint8_t) RST << 2 | (uint8_t) SYN << 1 | (uint8_t) FIN;
        TCPHeader[14] = 0xFF;
        TCPHeader[15] = 0xFF;
        TCPHeader[18] = 0;
        TCPHeader[19] = 0;

        for(int i = 0; i < dataLen; i++){
                TCPHeader[20+i] = data[i];
        }

        if (dataLen%2){
                TCPHeader[20+dataLen] = 0;
        }

        //calculate the checksum to be inserted in the tcp header
        uint16_t checksum = TCPCheckSumCalculator(TCPHeader, TCPHeaderLen);
        TCPHeader[16] = (uint8_t) (checksum >> 8);
        TCPHeader[17] = (uint8_t) (checksum & 0xFF);

        // print out the TCP header
        printf("TCP Header: \n");
        for (int i = 0; i< *TCPHeaderLen; i++){
                printf("%02X ", TCPHeader[i]);
        }
        printf("\n");

        return TCPHeader;
}

uint16_t TCPCheckSumCalculator(uint8_t* TCPHeader, int* TCPHeaderLen){
        uint32_t sum = 0;
        for(int i = 0; i < *TCPHeaderLen/2; i++){
                sum += TCPHeader[2*i] << 8 | TCPHeader[2*i+1];
        }

        //folding the 32 bit number into a 16 bit number 
        uint16_t checksum = (sum >> 16) + (sum & 0xFFFF);
        checksum = ~checksum;

        printf("Checksum: %04X\n", checksum);

        return checksum;
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

        //88:63:df:b0:af:ce
        uint8_t srcMacAddress[] = {0x88,0x63,0xDF,0xB0,0xAF,0xCE};
        uint8_t srcIPAddress[] = {169,254,250,95};
        uint8_t destIPAddress[] = {192,168,1,255};

        spiInitilization(fd,srcMacAddress);

        enableInterrupts(fd);

        enableReceive(fd);

        int counter = 0; 

        while(counter < 1){

        uint16_t receiveBufferPtr = RECEIVEBUFFERSTART;

        char transmissionMessage[] = "Check if this tranmission of TCP is working?";

        uint16_t transmitStartPtr = 0x0000;

        int packetLen;
        uint8_t* packet = IPV4PacketCreator(transmissionMessage,srcIPAddress,destIPAddress, &packetLen);
        
        uint16_t transmitEndPtr = transmitPacket(fd, &transmitStartPtr, srcMacAddress, packet, packetLen);

        readTransmissionPacket(fd,transmitStartPtr,packetLen);

        success = verifyTransmission(fd,transmitEndPtr);

        if (!success){
                printf("Transmission Failed\n");
                return -1;
        } else {
                printf("Transmission Successful\n");

                // while(1){
                //         if (receivePacketAvailable(fd)){
                //                 uint8_t* message = serviceReceivePacket(fd,&receiveBufferPtr);
                //                 printf("Message: %s\n", message);
                //                 free(message);
                //         }
                // }
        }

        counter++;

        }



        close(fd);
        return 0; 
}
