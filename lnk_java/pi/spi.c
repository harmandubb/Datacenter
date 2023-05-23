#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>


uint8_t mode = 0;
uint32_t speed = 20000000; //20 MHz
uint8_t bits = 8;

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

        // LEts do the transfer 
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

uint8_t* writeBufferMemory(int fd, uint8_t *message, int length){
        uint8_t* data = (uint8_t*) malloc((length + 1)*sizeof(uint8_t));
        if(data == NULL){
                perror("Memory Allocation Failed while writing to buffer memory");
                return NULL;
        } else {
                data[0] = 0x7A;
                for (int i = 0; i < length; i++){
                        data[i+1] = message[i];
                }
                spi_transfer(fd, data,length+1); //Check if the cs line needs to be change option needs to be changed because of the writing buffer needs to continuously write to the buffer
                return data;
        }
}

uint8_t* readBufferMemory(int fd){
        int length = 10000;
        uint8_t* data = (uint8_t*) calloc(length,sizeof(uint8_t));
        if (data == NULL){
                perror("Memory Allocation Failed while reading buffer memory");
                return NULL;
        } else {
                data[0] = 0x3A;
                spi_transfer(fd, data, length); //Check if there needs to be a condition in the spi_transfer function that breaks from the loop somehow if the read is not giving anything for a significant amount of time. 
        }
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

        // ready for SPU accessess

        // // Setup GPI's direction
        // data[0] = 0x1F;
        // data[1] = 0x01;

        // readControlReg(fd,data);

        // // register values after read
        // uint8_t regData = data[0];

        // printf("The old reg data is: %d\n", regData);

        // regData &= ~(3);
        // regData |= 1;

        // printf("The new reg data to be written: %d\n", regData);

        // writeControlReg(fd,data,0x1F,regData);

        // data[0] = 0x1F;
        // data[1] = 0x01;

        // readControlReg(fd,data);

        // printf("The new reg data 0 is: %d\n", data[0]);
        // printf("The new reg data 1 is: %d\n", data[1]);

        resetCommand(fd);

        readControlReg(fd, data, 0x1F);

        printf("Output of Ecom1 Reg: %d\n", data[0]);
        printf("Output of Ecom1 Reg: %d\n", data[1]);

        uint8_t ecom1Data = data[1];

        ecom1Data &= ~(3);
        ecom1Data |= (1);

        writeControlReg(fd,data,0x1F,ecom1Data);

        readControlReg(fd, data, 0x1F);

        printf("Output of Ecom1 Reg: %d\n", data[0]);
        printf("Output of Ecom1 Reg: %d\n", data[1]);


        // for(int i = 0; i < 32; i++){
        //         printf("Reading Reg: %d\n", i);
        //         readControlReg(fd, data, i);
        //         // printf("Output of Reg 0: %d\n", data[0]);
        //         printf("Output of Reg 1: %d\n", data[1]);
        // }

        close(fd);
        return 0; 
}