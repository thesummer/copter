#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <ncurses.h>

#define I2CPORT "/dev/i2c-0"
#define PWM_SLAVE_ADDR  0b0011010
#define STARTREGISTER 0
// #define FALSE 1
// #define TRUE 06
#define LENGTH 62

#define uniq(LOW,HEIGHT)	((HEIGHT << 8)|LOW)			  // Create 16 bit number from two bytes
#define LOW_BYTE(x)        	(x & 0xff)					    // Get low byte from 16 bit number
#define HIGH_BYTE(x)       	((x >> 8) & 0xff)			  // Get high byte from 16 bit number

int channel[4] = {0,0,0,0};
char row[LENGTH+1];
int pwmSlave;

int I2Cinit()
{
      pwmSlave = open(I2CPORT, O_RDWR | O_NOCTTY | O_NDELAY);
      if (pwmSlave == -1)
      {
            printf("open_port: Unable to open i2c-bus. Maybe root permissions necessary?\n");
            return FALSE; //0 == error
      }
      
      if (ioctl(pwmSlave, I2C_SLAVE, PWM_SLAVE_ADDR) < 0)
      {
            printf("Failed to acquire bus access and/or talk to slave.\n");
            return FALSE; // 0 == error
      }
        
      return TRUE;     
}

int setSingleChannel(int ch)
{
    uint8_t data[3];
    if (channel[ch] > 8191)
        channel[ch] = 8191;
    if (channel[ch] < 0)
        channel[ch] = 0;
    
    data[0] = STARTREGISTER + 2*ch;
    data[1] = HIGH_BYTE(channel[ch]);
    data[2] = LOW_BYTE(channel[ch]);
    if (write(pwmSlave, data, 3) != 3)
     {
       endwin();
       printf("Failed write duty cycle\n");
       exit (1);
     }
//     else
//        printf("Sending new duty cycle succeeded\n");
     
      return TRUE;
}

int setAllChannels()
{
    uint8_t data[9];
    int i;
    data[0] = STARTREGISTER;
    for (i=0; i<4;i++)
    {
        if (channel[i] > 8191)
            channel[i] = 8191;
        if (channel[i] < 0)
            channel[i] = 0;
        data[2*i+1]   = HIGH_BYTE(channel[i]);
        data[2*i+2] = LOW_BYTE(channel[i]);
    }

    if (write(pwmSlave, data, 9) != 9)
     {
       endwin();
       printf("Failed write duty cycles\n");
       exit (1);
     }
//      else
//        printf("Sending new duty cycles succeeded\n");
      return TRUE;
}

void printScreen()
{
   erase();
   mvprintw(2, 2, "Channel 1:");
   mvprintw(6, 2, "Channel 2:");
   mvprintw(10, 2, "Channel 3:");
   mvprintw(14, 2, "Channel 4:");
   
   attron(COLOR_PAIR(1) | A_INVIS);
   mvprintw(4, 5, "%s", &row[LENGTH - LENGTH*channel[0]/8191]);
   mvprintw(8, 5, "%s", &row[LENGTH - LENGTH*channel[1]/8191]);
   mvprintw(12, 5, "%s", &row[LENGTH - LENGTH*channel[2]/8191]);
   mvprintw(16, 5, "%s", &row[LENGTH - LENGTH*channel[3]/8191]);
   attroff(COLOR_PAIR(1) | A_INVIS);
   
   mvprintw(4, 63, ":%4d", channel[0]);
   mvprintw(8, 63, ":%4d", channel[1]);
   mvprintw(12, 63, ":%4d", channel[2]);
   mvprintw(16, 63, ":%4d", channel[3]);
   
   mvprintw(18, 2, "+/-: Switch to increase or decrease mode");
   mvprintw(19, 2, "1-4:Change value of channel");
   mvprintw(20, 2, "a:Change all channels");
   mvprintw(21, 2, "q: Quit");
   
   refresh();
}
      

int main(int argc, char *argv[])
{
   int ch = 0;
   int i;
   for (i = 0; i<LENGTH+1; i++)
	 row[i] = '#';
   row[0] = 'a';
   row[LENGTH] = '\0';
   
   if (I2Cinit() != TRUE)
   {
       printf("Initializing I2C interface failed\n");
       exit (1);
   }
   initscr();
   if(has_colors() == FALSE)
	{	endwin();
		printf("Your terminal does not support color\n");
		exit(1);
	}
   start_color();			/* Start color 			*/
   init_pair(1, COLOR_BLUE, COLOR_BLUE);
   cbreak();
   noecho();
    
   setAllChannels();
   printScreen();
   int inc = 1;
   while(ch != 'q')
   {
	 ch = getch();
	 switch(ch)
	 {
         case '+':inc = 32; break;
         case '-':inc = -32; break;
		 case '1': channel[0]+=inc; 
                   setSingleChannel(0); break;
		 case '2': channel[1]+=inc;
                   setSingleChannel(1); break;
		 case '3': channel[2]+=inc;
                   setSingleChannel(2); break;
		 case '4': channel[3]+=inc;
                   setSingleChannel(3); break;
		 case 'a': channel[0]+=inc; 
				   channel[1]+=inc; 
				   channel[2]+=inc; 
				   channel[3]+=inc; 
                   setAllChannels();    break;
     }
     printScreen();
   }
   endwin();
   
   return 0;
}
      
      
