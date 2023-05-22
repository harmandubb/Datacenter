#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
// #include <stdint.h>
#include <linux/spi/spidev.h>
#include <stdbool.h>
 
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
 
static void pabort(const char *s)
{
        perror(s);
        abort();
}

// Set up parameters
static const char *device = "/dev/spidev0.0";
static uint8_t mode = 0;
static uint8_t bits = 8;
static uint32_t speed = 20000000;
static uint16_t delay = 0;
 
uint8_t createByte(uint8_t opCode, uint8_t arg){
    return (opCode << 5 ) | arg;
}

uint8_t readControlRegister(int fd, uint8_t regAddress, bool debug)
{
        int ret = 0;
        uint8_t opCode = 0;
        uint8_t tx[] = {
            createByte(opCode, regAddress)
        };

        uint8_t rx[ARRAY_SIZE(tx)] = {0, };// Equal array to get the response
        struct spi_ioc_transfer tr; //This struct is a standard struct in the spidev.h file 

        memset(&tr, 0, sizeof(tr)); //Holds the parameters for the SPI structure
        tr.tx_buf = (unsigned long)tx;
        tr.rx_buf = (unsigned long)rx;
        tr.len = ARRAY_SIZE(tx);
        tr.delay_usecs = delay;
        tr.speed_hz = speed;
        tr.bits_per_word = bits;
        tr.cs_change = 1; //we only need to see how the regester looks like 

        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1)
                pabort("can't send spi message");

        //use this for debugging for now
        for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
                if (debug){
                        printf("Command input to read: %d\n", tx[0]);
                        printf("Read function byte reading: %d \n", rx[ret]);
                }
        }
        return rx[0];
}

void writeControlRegister(int fd, uint8_t regAddress, uint8_t data, bool debug)
{
        int ret = 0;
        uint8_t opCode = 2;
        uint8_t tx[] = {
            createByte(opCode, regAddress),
            data
        };

        if (debug){
                printf("Writing to reg address: %.2X\n", regAddress);
                printf("Data being written to reg: %d\n", data);
                printf("Contents in the tx array:\n");
                for (int i = 0; i < ARRAY_SIZE(tx); i++){
                        printf("%d\n", tx[i]); 
                }
        }

        uint8_t rx[ARRAY_SIZE(tx)] = {0, };// Equal array to get the response
        struct spi_ioc_transfer tr; //This struct is a standard struct in the spidev.h file 

        memset(&tr, 0, sizeof(tr)); //Holds the parameters for the SPI structure
        tr.tx_buf = (unsigned long)tx;
        tr.rx_buf = (unsigned long)rx;
        tr.len = ARRAY_SIZE(tx);
        tr.delay_usecs = delay;
        tr.speed_hz = speed;
        tr.bits_per_word = bits;
        tr.cs_change = 1; //we only need to see how the regester looks like 

        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1)
                pabort("can't send spi message");
        if (debug){
                printf("Checking the transmit array:\n");
                for (int i = 0; i < ARRAY_SIZE(tx); i++){
                        printf("%d\n", tx[i]);
                }
                printf("Checking the receive array:\n");
                for (int i = 0; i < ARRAY_SIZE(rx); i++){
                        printf("%d\n", rx[i]);
                }
                printf("Reading the control register right after I have written to it: %d\n", readControlRegister(fd, regAddress, debug));
        }
}

uint8_t* readBufferMemory(int fd, bool debug)
{
        int ret = 0;
        int bufferSize = 100;
        uint8_t opCode = 1;
        uint8_t arg = 26;
        uint8_t tx[] = {
            createByte(opCode, arg)
        };
        if (debug){
                puts("Right before the calloc is called?");
        }

        uint8_t* rx = (uint8_t*)calloc(bufferSize, sizeof(uint8_t));
        if(rx == NULL){
                puts("Memory allocation failed");
                exit(1);
        }

        if (debug){
                puts("Is is a probelm with making the calloc?");
        }

       

        //write data into the allocated memory 
        struct spi_ioc_transfer tr; //This struct is a standard struct in the spidev.h file 

        memset(&tr, 0, sizeof(tr)); //Holds the parameters for the SPI structure
        tr.tx_buf = (unsigned long)tx;
        tr.rx_buf = (unsigned long)rx;
        tr.len = bufferSize + 1;
        tr.delay_usecs = delay;
        tr.speed_hz = speed;
        tr.bits_per_word = bits;
        tr.cs_change = 0; //we only need to see how the regester looks like 

        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        
        if (debug){
                puts("Is it a problem with receving the data into the buffer");
        }

        if (ret < 1)
            pabort("can't send spi message");
                
        //use this for debugging for now

        int counter = 0;
        for (ret = 0; ret < bufferSize; ret++) {
            if (rx[ret] == 0){
                // puts("Found a blank");
                counter++;
                if (counter > 100){
                    break;
                }
            }else{
                // puts("Something in the buffer");
                // printf("%.2X ", rx[ret]);
                counter = 0;
            }
        }
        return rx;
}

void writeBufferMemory(int fd, char cmd[], int size, bool debug)
{
        int ret = 0;
        uint8_t opCode = 3;
        uint8_t arg = 26;

        uint8_t* tx = (uint8_t*)calloc(size + 1, sizeof(uint8_t));
        
        if(tx == NULL){
                if (debug){
                        puts("Memory allocation failed");
                }
                exit(1);
        }

        //inserting the information into 
        tx[0] = createByte(opCode,arg);

        for (int i = 0; i < size; i++){
            tx[1+i] = cmd[i];
        }
        
        uint8_t* rx = (uint8_t*)calloc(size + 1, sizeof(uint8_t));
        if(rx == NULL){
                if (debug){
                        puts("Memory allocation failed");
                }
                exit(1);
        }
        

        //write data into the allocated memory 
        struct spi_ioc_transfer tr; //This struct is a standard struct in the spidev.h file 

        memset(&tr, 0, sizeof(tr)); //Holds the parameters for the SPI structure
        tr.tx_buf = (unsigned long)tx;
        tr.rx_buf = (unsigned long)rx;
        tr.len = size + 1;
        tr.delay_usecs = delay;
        tr.speed_hz = speed;
        tr.bits_per_word = bits;
        tr.cs_change = 1; 

        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1)
                pabort("can't send spi message");
        if (debug){
                puts("The message has been sent successfully");
        }
        free(rx);
        free(tx);
}
 
static void print_usage(const char *prog)
{
        printf("Usage: %s [-DsbdlHOLC3]\n", prog);
        puts("  -D --device   device to use (default /dev/spidev0.0)\n"
             "  -s --speed    max speed (Hz)\n"
             "  -d --delay    delay (usec)\n"
             "  -b --bpw      bits per word \n"
             "  -l --loop     loopback\n"
             "  -H --cpha     clock phase\n"
             "  -O --cpol     clock polarity\n"
             "  -L --lsb      least significant bit first\n"
             "  -C --cs-high  chip select active high\n"
             "  -3 --3wire    SI/SO signals shared\n");
        exit(1);
}

static void parse_opts(int argc, char *argv[])
{
        while (1) {
                static const struct option lopts[] = {
                        { "device",  1, 0, 'D' },
                        { "speed",   1, 0, 's' },
                        { "delay",   1, 0, 'd' },
                        { "bpw",     1, 0, 'b' },
                        { "loop",    0, 0, 'l' },
                        { "cpha",    0, 0, 'H' },
                        { "cpol",    0, 0, 'O' },
                        { "lsb",     0, 0, 'L' },
                        { "cs-high", 0, 0, 'C' },
                        { "3wire",   0, 0, '3' },
                        { "no-cs",   0, 0, 'N' },
                        { "ready",   0, 0, 'R' },
                        { NULL, 0, 0, 0 },
                };
                int c;
 
                c = getopt_long(argc, argv, "D:s:d:b:lHOLC3NR", lopts, NULL);
 
                if (c == -1)
                        break;

                switch (c) {
                case 'D':
                        device = optarg;
                        break;
                case 's':
                        speed = atoi(optarg);
                        break;
                case 'd':
                        delay = atoi(optarg);
                        break;
                case 'b':
                        bits = atoi(optarg);
                        break;
                case 'l':
                        mode |= SPI_LOOP;
                        break;
                case 'H':
                        mode |= SPI_CPHA;
                        break;
                case 'O':
                        mode |= SPI_CPOL;
                        break;
                case 'L':
                        mode |= SPI_LSB_FIRST;
                        break;
                case 'C':
                        mode |= SPI_CS_HIGH;
                        break;
                case '3':
                        mode |= SPI_3WIRE;
                        break;
                case 'N':
                        mode |= SPI_NO_CS;
                        break;
                case 'R':
                        mode |= SPI_READY;
                        break;
                default:
                        print_usage(argv[0]);
                        break;
                }
        }
}

uint8_t switchRegBank(int fd, uint8_t bankNum, int debug){
        if (debug){   
                printf("\nIn a switch Reg Bank function\n");
        }
        uint8_t econ1RegAddress = 0x1F;
        uint8_t econ1RegData = readControlRegister(fd, econ1RegAddress, false);
        
        if (debug){
                printf("ECON 1 being Read: %d\n", econ1RegData);
        }
        // printf("Econ1 before modification: %d\n", econ1RegData);
        econ1RegData &= ~(3);
        if (debug){
                printf("Econ1 after clearning selec bytes: %d\n", econ1RegData);
        }
        econ1RegData |= bankNum;
        if (debug){
                printf("Econ1 after bankNum: %d\n", econ1RegData);
                printf("Written to (Control Reg): %.2X\n", econ1RegAddress);
                printf("Data: %d\n", econ1RegData);
        }

        writeControlRegister(fd, econ1RegAddress, econ1RegData, false);

        uint8_t checkReg = readControlRegister(fd, econ1RegAddress, false);
        
        if (debug){
                printf("Read Value from Reg after uploaded: %d\n", checkReg);
        }

        return checkReg;
}

void initilization(int fd, uint16_t rBuffSt, uint16_t rBuffEd, bool debug){
        // write to bank 1 to the received buffer pointers 
        switchRegBank(fd,0, debug); //Write to buffer pointers 

        //splitting data into bytes
        uint8_t rBuffStL_Data = (uint8_t) (rBuffSt & 0xFF);
        uint8_t rBuffStH_Data = (uint8_t) ((rBuffSt >> 8) & 0xFF);

        uint8_t rBuffNdL_Data = (uint8_t) (rBuffEd & 0xFF);
        uint8_t rBuffNdH_Data = (uint8_t) ((rBuffEd >> 8) & 0xFF);

        //storing the reg addresses in the bank 
        uint8_t rBuffStL_Address = 0x08;
        uint8_t rBuffStH_Address = 0x09;

        writeControlRegister(fd, rBuffStL_Address, rBuffStL_Data, debug);
        writeControlRegister(fd, rBuffStH_Address, rBuffStH_Data, debug);

        uint8_t rBuffNdL_Address = 0x0A;
        uint8_t rBuffNdH_Address = 0x0B;

        writeControlRegister(fd, rBuffNdL_Address, rBuffNdL_Data, debug);
        writeControlRegister(fd, rBuffNdH_Address, rBuffNdH_Data, debug);

        // incorporate filters 

        uint8_t EstatRegAddress = 0x1D;

        // don't want to do anything else until the start up timer has been finished
        int startUpTimer = 1;

        while (startUpTimer) {
                if (debug){
                        printf("In the start up Timer loop\n");
                }
                uint8_t EstatReg_Data = readControlRegister(fd, EstatRegAddress, false);
                if(EstatReg_Data & (1 << 0)){
                        startUpTimer = 0;
                }
        }
        
        // Initializing MAC
        switchRegBank(fd,2, debug);

        // uint8_t macon1 = readControlRegister(fd, 0x00);
        // // printf("macon1 before modification: %d \n", macon1);
        // macon1 &= (1 << 0);
        // // printf("macon1 after modification: %d \n", macon1);

        // writeControlRegister(fd, 0x00, macon1);



}

void serverlnkInitilization(int fd, bool debug){
        initilization(fd, 0x099A, 0x1FFF, debug);
}

int spiSetup(int argc, char *argv[]){
        int ret = 0;
        int fd;

        //------------START of Device set up-----------//
 
        parse_opts(argc, argv);
 
        fd = open(device, O_RDWR); //opening the SPI device
        if (fd < 0)
                pabort("can't open device");
 
        /*
         * spi mode
         */
        ret = ioctl(fd, SPI_IOC_WR_MODE, &mode); //input/output control
        if (ret == -1)
                pabort("can't set spi mode");
 
        ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
        if (ret == -1)
                pabort("can't get spi mode");
 
        /*
         * bits per word
         */
        ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
        if (ret == -1)
                pabort("can't set bits per word");
 
        ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
        if (ret == -1)
                pabort("can't get bits per word");
 
        /*
         * max speed hz
         */
        ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
        if (ret == -1)
                pabort("can't set max speed hz");
 
        ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
        if (ret == -1)
                pabort("can't get max speed hz");
 
        printf("spi mode: %d\n", mode);
        printf("bits per word: %d\n", bits);
        printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

        return fd;
}

int main(int argc, char *argv[])
{
        int fd = spiSetup(argc, argv);

        //------------------END of SET UP---------------//

        serverlnkInitilization(fd, false);

        //-----------Testing to read all MAC registers----//

        // Mac registers are in bank 2
        // printf("\nIn the main after set up done:\n");
        // uint8_t econ1Data = switchRegBank(fd, 2);

        // printf("Econ1Data is: %d\n", econ1Data);

        // do a basic read write test 
        for (int i = 0; i < 32; i++){
                printf("Reading Register: %d\n", i);

                uint8_t initialRead = readControlRegister(fd,i,true);

                printf("The initial Read Value is: %.2x\n", initialRead);

                writeControlRegister(fd, i, 0x01, false);

                uint8_t secondRead = readControlRegister(fd,0x1F,true);

                printf("The second Read Value is: %.2x\n", secondRead);        
        }
        return 1;
}

// scp /Users/harmandeepdubb/Desktop/Datacenter/lnk_java/pi/spi.c root@192.168.1.109:/home
