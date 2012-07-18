#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>

#define I2CPORT "/dev/i2c-0"
#define pwmSlaveAddr  0b0011010
#define startRegister 0
#define FALSE 1
#define TRUE 0

int main(int argc, char *argv[])
{
      
      int pwmSlave;
      
      pwmSlave = open(I2CPORT, O_RDWR | O_NOCTTY | O_NDELAY);
      if (pwmSlave == -1)
      {
            printf("open_port: Unable to open i2c-bus\n");
            return FALSE; //0 == error
      }
      
      if (ioctl(pwmSlave, I2C_SLAVE, pwmSlaveAddr) < 0)
      {
            printf("Failed to acquire bus access and/or talk to slave.\n");
            return FALSE; // 0 == error
      }
        
      
      while(1)
      {
	    int i;
	    uint8_t dutyCycles[5];
	    printf("Please insert a new set of duty cycle (4 numbers from 0 - 255):\n");
	    scanf("%hhu %hhu %hhu %hhu", &dutyCycles[1], &dutyCycles[2], &dutyCycles[3], &dutyCycles[4]);
	    dutyCycles[0] = startRegister;
	    if (write(pwmSlave, dutyCycles, 5) != 5)
	    {
		  printf("Failed write duty cycles\n");
		  return FALSE;
	    }
	    else
		  printf("Sending new duty cycles succeeded\n");
	    
      }
      return TRUE;
}
      
      
