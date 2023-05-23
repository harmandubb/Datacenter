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



uint8_t createInstruction(uint8_t opCode, uint8_t argument){
        uint8_t output = ((opCode << 5) | argument);

        // printf("The Created instruction is: %d\n", output);

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

        // printf("Read Control Reg input: %d\n", *data);

        spi_transfer(fd, data, 2);

        // printf("First output after reading: %d\n", data[0]);
        // printf("Secound output after reading: %d\n", data[1]);
}

void writeControlReg(int fd, uint8_t *data, uint8_t reg, uint8_t input){
        data[0] = createInstruction(2,reg);
        data[1] = input;

        printf("The instruction is: %d\n", data[1]);
        printf("The value to write is: %d\n", data[0]);

        spi_transfer(fd,data,2);
}

void resetCommand(int fd){
        uint8_t data[2] = {0}; 
        data[0] = 0xFF;
        spi_transfer(fd, data, 2);
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