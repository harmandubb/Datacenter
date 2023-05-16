#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
 
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
 
static void transfer(int fd)
{
        int ret;
        uint8_t tx[] = {
                0x01, 0x02, 0x03, 0x04,
        }; //Out going command
        uint8_t rx[ARRAY_SIZE(tx)] = {0, }; // Equal array to get the response
        struct spi_ioc_transfer tr; //This struct is a standard struct in the spidev.h file 

        // struct spi_ioc_transfer {
        //     __u64	tx_buf;    // array for the tx buffer
        //     __u64	rx_buf;    // array for the rx biffer  

        //     __u32	len;       // length of the data trasnfer that will be expressed in bytes.
        //     __u32	speed_hz;  // speed of the spi interface 

        //     __u16	delay_usecs;
        //     __u8	    bits_per_word;
        //     __u8	    cs_change;
        //     __u8	    tx_nbits;
        //     __u8	    rx_nbits;
        //     __u16	pad;
        // };

        memset(&tr, 0, sizeof(tr)); //Holds the parameters for the SPI structure
        tr.tx_buf = (unsigned long)tx;
        tr.rx_buf = (unsigned long)rx;
        tr.len = ARRAY_SIZE(tx);
        tr.delay_usecs = delay;
        tr.speed_hz = speed;
        tr.bits_per_word = bits;
        tr.cs_change = 0;

        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1)
                pabort("can't send spi message");
 
        for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
                if (!(ret % 6))
                        puts("");
                printf("%.2X ", rx[ret]);
        }
        puts("");
}

uint8_t createByte(uint8_t opCode, uint8_t arg){
    return (opCode << 5 ) | arg;
}

uint8_t readControlRegister(int fd, uint8_t regAddress)
{
        int ret = 0;
        uint8_t opCode = 0;
        uint8_t tx[] = {
            createByte(opCode, regAddress)
        };

        uint8_t rx[ARRAY_SIZE(tx)] = {0, };// Equal array to get the response
        struct spi_ioc_transfer tr; //This struct is a standard struct in the spidev.h file 

        // struct spi_ioc_transfer {
        //     __u64	tx_buf;    // array for the tx buffer
        //     __u64	rx_buf;    // array for the rx biffer  

        //     __u32	len;       // length of the data trasnfer that will be expressed in bytes.
        //     __u32	speed_hz;  // speed of the spi interface 

        //     __u16	delay_usecs;
        //     __u8	    bits_per_word;
        //     __u8	    cs_change;
        //     __u8	    tx_nbits;
        //     __u8	    rx_nbits;
        //     __u16	pad;
        // };

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
                if (!(ret % 6))
                        puts("");
            printf("%.2X ", rx[ret]);
            return rx[ret];
        }
}

void writeControlRegister(int fd, uint8_t regAddress, uint8_t data)
{
        int ret = 0;
        uint8_t opCode = 2;
        uint8_t tx[] = {
            createByte(opCode, regAddress),
            data
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
                if (!(ret % 6))
                        puts("");
                printf("%.2X ", rx[ret]);
        }
        puts("");
}

uint8_t* readBufferMemory(int fd)
{
        int ret = 0;
        int bufferSize = 100;
        uint8_t opCode = 1;
        uint8_t arg = 26;
        uint8_t tx[] = {
            createByte(opCode, arg)
        };
        puts("Right before the calloc is called?");

        uint8_t* rx = (uint8_t*)calloc(bufferSize, sizeof(uint8_t));
        if(rx == NULL){
                puts("Memory allocation failed");
                exit(1);
        }

        puts("Is is a probelm with making the calloc?");

       

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
        
        puts("Is it a problem with receving the data into the buffer");

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
                printf("%.2X ", rx[ret]);
                counter = 0;
            }
        }
        return rx;
}

void writeBufferMemory(int fd, char cmd[], int size)
{
        int ret = 0;
        uint8_t opCode = 3;
        uint8_t arg = 26;

        uint8_t* tx = (uint8_t*)calloc(size + 1, sizeof(uint8_t));
        
        if(tx == NULL){
            puts("Memory allocation failed");
            exit(1);
        }

        //inserting the information into 
        tx[0] = createByte(opCode,arg);

        for (int i = 0; i < size; i++){
            tx[1+i] = cmd[i];
        }
        
        uint8_t* rx = (uint8_t*)calloc(size + 1, sizeof(uint8_t));
        if(rx == NULL){
                puts("Memory allocation failed");
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
        
        puts("The message has been sent successfully");
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

void switchRegBank(int fd, uint8_t bankNum){   
        uint8_t econ1RegAddress = 0x1F;

        uint8_t econ1RegData = readControlRegister(fd, econ1RegAddress);

        econ1RegData &= ~(3);
        econ1RegData |= bankNum;

        writeControlRegister(fd, econ1RegAddress, econ1RegData);
}

void initilization(int fd, uint16_t rBufSt, uint16_t rBufEd){
        // write to bank 1 to the received buffer pointers 
        switchRegBank(fd,0);

        uint8_t rBuffStL_Data = (uint8_t) (rBuffSt & 0xFF);
        uint8_t rBuffStH_Data = (uint8_t) ((rBuffSt >> 8) & 0xFF);

        uint8_t rBuffNdL_Data = (uint8_t) (rBuffEd & 0xFF);
        uint8_t rBuffNdH_Data = (uint8_t) ((rBuffEd >> 8) & 0xFF);

        uint8_t rBuffStL_Address = 0x08;
        uint8_t rBuffStH_Address = 0x09;

        writeControlRegister(fd, rBuffStL_Address, rBuffStL_Data);
        writeControlRegister(fd, rBuffStH_Address, rBuffStH_Data);

        uint8_t rBuffNdL_Address = 0x0A;
        uint8_t rBuffNdH_Address = 0x0B;

        writeControlRegister(fd, rBuffNdL_Address, rBuffNdL_Data);
        writeControlRegister(fd, rBuffNdH_Address, rBuffNdH_Data);

        // incorporate filters 

        uint8_t EstatRegAddress = 0x1D;

        // don't want to do anything else until the start up timer has been finished
        int startUpTimer = 1;

        while (startUpTime) {
                uint8_t EstatReg_Data = readControlRegister(fd, EstatRegAddress);
                if(EstatReg_Data & (1 << 0)){
                        startUpTimer = 0;
                }
        }
        


}



int main(int argc, char *argv[])
{
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

        //------------------END of SET UP---------------//
        uint8_t econ2Val = readControlRegister(fd, 30);

        econ2Val |= 128;

        writeControlRegister(fd,30,econ2Val);

        char cmd[] = "\r \r \r";

        int size = strlen(cmd);

        printf("Size length: %d\n", size);
        
        puts("About to write to the buffer memory");
        writeBufferMemory(fd,cmd, size);

        puts("About to read to the buffer memory");
        uint8_t *output = readBufferMemory(fd); //error with the malloc is present here
 
        puts("Done reading");

        free(output);
    
        close(fd);
 
        return ret;
}

// scp /Users/harmandeepdubb/Desktop/Datacenter/lnk_java/pi/spi.c root@192.168.1.109:/home
