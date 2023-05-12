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
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 20000000;
static uint16_t delay;
 
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

static void writeControlRegister(int fd, uint8_t regAddress, uint8_t data)
{
        int ret = 0;
        uint8_t opCode = 2;
        uint8_t tx[] = {
            createByte(opCode, regAddress),
            data
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
        }
        puts("");
}

int* readBufferMemory(int fd)
{
        int ret = 0;
        int bufferSize = 100;
        uint8_t opCode = 1;
        uint8_t arg = 26;
        uint8_t tx[] = {
            createByte(opCode, arg)
        };
        int* rx = (int*)calloc(bufferSize, sizeof(uint8_t));

        if(rx == NULL){
            puts("Memory allocation failed");
            exit(1);
        }

        //write data into the allocated memory 
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

        int counter = 0;
        for (ret = 0; ret < bufferSize; ret++) {
            if (rx[ret] == 0){
                puts("Found a blank");
                counter++;
                if (counter > 2){
                    break;
                }
            }else{
                puts("Something in the buffer");
                printf("%.2X ", rx[ret]);
                counter = 0;
            }
        }
        return rx;
}

int* readBufferMemory(int fd)
{
        int ret = 0;
        int bufferSize = 100;
        uint8_t opCode = 1;
        uint8_t arg = 26;
        uint8_t tx[] = {
            createByte(opCode, arg)
        };
        int* rx = (int*)calloc(bufferSize, sizeof(uint8_t));

        if(rx == NULL){
            puts("Memory allocation failed");
            exit(1);
        }

        //write data into the allocated memory 
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

        int counter = 0;
        for (ret = 0; ret < bufferSize; ret++) {
            if (rx[ret] == 0){
                puts("Found a blank");
                counter++;
                if (counter > 2){
                    break;
                }
            }else{
                puts("Something in the buffer");
                printf("%.2X ", rx[ret]);
                counter = 0;
            }
        }
        return rx;
}

void writeBufferMemory(int fd, int cmd[])
{
        int ret = 0;
        uint8_t opCode = 3;
        uint8_t arg = 26;

        int size = sizeof(cmd) / sizeof(cmd[0]);
        int* tx = (int*)calloc(size + 1, sizeof(uint8_t));
        
        if(tx == NULL){
            puts("Memory allocation failed");
            exit(1);
        }

        //inserting the information into 
        tx[0] = createByte(apCode,arg);

        for (int i = 0; i < size; i++){
            tx[1+i] = cmd[i];
        }
        
        uint8_t rx = [0];

        

        //write data into the allocated memory 
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

        // int counter = 0;
        // for (ret = 0; ret < bufferSize; ret++) {
        //     if (rx[ret] == 0){
        //         puts("Found a blank");
        //         counter++;
        //         if (counter > 2){
        //             break;
        //         }
        //     }else{
        //         puts("Something in the buffer");
        //         printf("%.2X ", rx[ret]);
        //         counter = 0;
        //     }
        // }
        // return rx;
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
        // uint8_t econ1Val = readControlRegister(fd, 31); //read what the ECON1 register holds 

        // //ensure only the last two bits are cleared 
        // econ1Val &= ~(3);
        
        // writeControlRegister(fd, 31, econ1Val);

        // for(int i = 0; i < 32; i++){
        //     printf("%d:\n", i);
        //     readControlRegister(fd, i);
        // }
        uint8_t econ2Val = readControlRegister(fd, 30);

        econ2Val |= 128;

        writeControlRegister(fd,30,econ2Val);

        writeBufferMemory(fd,["\r"]);

        readBufferMemory(fd);
    
        close(fd);
 
        return ret;
}

// scp /Users/harmandeepdubb/Desktop/Datacenter/lnk_java/pi/spi.c root@192.168.1.109:/home
