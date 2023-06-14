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

void checkReg(int fd, uint8_t reg, char name[]);
void splitByte(uint8_t byte, uint8_t* split);


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

        checkReg(fd, EIRREG, "EIR");

        //Enable Global interupt bit 
        bitSet(fd,data,EIEREG,0x80);

        //Enable interupts for packet transmit and transmit error
        bitSet(fd, data, EIEREG, 0x02+0x08);

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

        MACON3_Val = 0xF2;

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

uint16_t transmitPacket(int fd, uint16_t* trasmitStartPtr, uint8_t srcMACAddress[], char message[]){
        uint8_t data[2] = {0};
        uint8_t split[2];

        int controlByteLen = 1;
        int destMACAddressLen = 6;
        int srcMACAddressLen = 6;
        int typeLen = 2;
        int messageLen = strlen(message);

        checkReg(fd, EIRREG, "EIR");

        //set the trasmit start pointer 
        splitWord(*trasmitStartPtr, split);
        switchRegBank(fd, 0);
        writeControlReg(fd, data, ETXSTLREG, split[1]);
        writeControlReg(fd, data, ETXSTHREG, split[0]);

        checkReg(fd, EIRREG, "EIR");

        // Write a control byte
        data[0] = 0x0E;
        writeBufferMemory(fd, data, 1);

        checkReg(fd, EIRREG, "EIR");

        // destination address 
        uint8_t destMACAddress[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        writeBufferMemory(fd, destMACAddress, 6);

        checkReg(fd, EIRREG, "EIR");

        //source address        
        writeBufferMemory(fd, srcMACAddress, 6);

        checkReg(fd, EIRREG, "EIR");

        //type/length
        splitWord(messageLen, split);
        writeBufferMemory(fd, split, 2);

        checkReg(fd, EIRREG, "EIR");

        //message
        writeBufferMemory(fd, message, messageLen);

        checkReg(fd, EIRREG, "EIR");

        //set the trasmit end pointer
        uint16_t transmitEndPtr = *trasmitStartPtr + controlByteLen + destMACAddressLen + srcMACAddressLen + typeLen + messageLen - 1;
        splitWord(transmitEndPtr, split);
        switchRegBank(fd, 0);
        writeControlReg(fd, data, ETXNDLREG, split[1]);
        writeControlReg(fd, data, ETXNDHREG, split[0]);

        checkReg(fd, EIRREG, "EIR");

        // Clear EIR.TXIF
        bitClear(fd, data, EIRREG, 0x08);
        checkReg(fd, EIRREG, "EIR");

        //set EIE.TXIE and EIE.INTIE
        bitSet(fd, data, EIEREG, 0x88);
        checkReg(fd, EIEREG, "EIE");

        checkReg(fd, ECON1REG, "ECON1");
        
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

void readTransmissionPacket(int fd, uint8_t transmitStartPtr, char message[]){
        uint8_t split[2];
        int transmissionPacketBaseSize = 15;
        int statusVectorSize = 7;
        int messageLen = strlen(message);
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

        checkReg(fd, EIRREG, "EIR");

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
        uint8_t srcMacAddress[6] = {0x88,0x63,0xDF,0xB0,0xAF,0xCE};

        spiInitilization(fd,srcMacAddress);

        enableInterrupts(fd);

        checkReg(fd, EIRREG, "EIR");

        

        char transmissionMessage[] = "Hello World";

        uint16_t transmitStartPtr = 0x0000;

        uint16_t transmitEndPtr = transmitPacket(fd, &transmitStartPtr, srcMacAddress, transmissionMessage);

        readTransmissionPacket(fd,transmitStartPtr,transmissionMessage);

        // readTransmissionMessageBuffer(fd,transmitStartPtr,transmissionMessage);

        verifyTransmission(fd,transmitEndPtr);



        close(fd);
        return 0; 
}
