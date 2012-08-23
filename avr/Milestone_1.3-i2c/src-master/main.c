/**
    @file src-master/main.c
    @brief program for the I2C master running a Linux OS
    @author Jan Sommer
    @date  1/8/2012

    Initializes an I2C-device already available in the linux /dev-tree.
    Provides a primitive ncurses-UI to send new duty cycle value via I2C to the slave.
    Each channel is represented with a labeled horizontal bar which length corresponds to
    the value set to the duty cycle.
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <ncurses.h>

#define I2CPORT "/dev/i2c-0"        ///< name of the I2C-device (i2c-0 for raspberryPi)
#define PWM_SLAVE_ADDR  0b0011010   ///< Address of the I2C-slave @sa SLAVE_ADDR_ATTINY
/**
    The first "register" of the I2C-slave.
    As it is only a buffer array the address space starts with 0.
*/
#define STARTREGISTER 0
// #define FALSE 1
// #define TRUE 0
#define LENGTH 62   ///< Maximum length of the bar presented in the UI

int channel[4] = {0,0,0,0}; /*!< Array which holds the current value of each channel */
char row[LENGTH+1]; /*!< String which contains length '#'s which represent a full bar*/
int pwmSlave; /*!< device descriptor of the PWM-slave */

/*!
 \brief initialize the I2C slave device

 Creates and sets up a file descriptor for the I2C-device.

 \return int TRUE if successful otherwise FALSE
*/
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

/*!
 \brief Set a new duty cycle for a certain channel @a ch of the slave

 \param ch the channel which is to be updated
 \return int TRUE if successful otherwise FALSE
*/
int setSingleChannel(int ch)
{
    // Protect upper and lower limits
    uint8_t data[2];
    if (channel[ch] > 255)
        channel[ch] = 255;
    if (channel[ch] < 0)
        channel[ch] = 0;
    

    data[0] = STARTREGISTER + ch;       // Register to write to
    data[1] = channel[ch];              // New Value of the register
    if (write(pwmSlave, data, 2) != 2)
     {
       endwin();
       printf("Failed write duty cycle\n");
       exit (1);
     }
     else
//        printf("Sending new duty cycle succeeded\n");
     
      return TRUE;
}

/*!
 \brief Writes new duty cycles to all channels of the slave at once (only on write command)

 \return int TRUE if successful otherwise FALSE
*/
int setAllChannels()
{
    uint8_t data[5];  //buffer for startregister + 4 values
    int i;
    data[0] = STARTREGISTER;
    for (i=0; i<4;i++)
    {
        // Protece upper and lower limits
        if (channel[i] > 255)
            channel[i] = 255;
        if (channel[i] < 0)
            channel[i] = 0;
        data[i+1] = channel[i];
    }
    
    //Write all values for the channels in one go
    if (write(pwmSlave, data, 5) != 5)
     {
       endwin();
       printf("Failed write duty cycles\n");
       exit (1);
     }
//      else
//        printf("Sending new duty cycles succeeded\n");
      return TRUE;
}

/*!
 \brief  Update the ncurses ui-screen

*/
void printScreen()
{
   erase();     //clear the ui

   //create the labels for the bar charts
   mvprintw(2, 2, "Channel 1:");
   mvprintw(6, 2, "Channel 2:");
   mvprintw(10, 2, "Channel 3:");
   mvprintw(14, 2, "Channel 4:");
   
   //calculate the length of each bar an draw it next to the labels
   attron(COLOR_PAIR(1) | A_INVIS);
   mvprintw(4, 5, "%s", &row[LENGTH - LENGTH*channel[0]/255]);
   mvprintw(8, 5, "%s", &row[LENGTH - LENGTH*channel[1]/255]);
   mvprintw(12, 5, "%s", &row[LENGTH - LENGTH*channel[2]/255]);
   mvprintw(16, 5, "%s", &row[LENGTH - LENGTH*channel[3]/255]);
   attroff(COLOR_PAIR(1) | A_INVIS);
   
   //print the numeric values after each bar
   mvprintw(4, 63, ":%3d", channel[0]);
   mvprintw(8, 63, ":%3d", channel[1]);
   mvprintw(12, 63, ":%3d", channel[2]);
   mvprintw(16, 63, ":%3d", channel[3]);
   
   //Print the information how to use the program
   mvprintw(18, 2, "+/-: Switch to increase or decrease mode");
   mvprintw(19, 2, "1-4:Change value of channel");
   mvprintw(20, 2, "a:Change all channels");
   mvprintw(21, 2, "q: Quit");
   
   refresh();   //paint everything on screen
}
      

/*!
 \brief  main program of the master

 Sets up the I2C device and ncurses ui.
 Reacts on the user input.

 \param argc
 \param argv[]
 \return int
*/
int main(int argc, char *argv[])
{
   int ch = 0;
   int i;

   //initialize an array of '#' which determines the maximum length of a bar
   for (i = 0; i<LENGTH+1; i++)
	 row[i] = '#';
   row[0] = 'a';
   row[LENGTH] = '\0';
   
   //Initialize I2C-device
   if (I2Cinit() != TRUE)
   {
       printf("Initializing I2C interface failed\n");
       exit (1);
   }

   //Initialize ncurses
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
    
   //Initialize the duty cycles of the slave
   setAllChannels();
   printScreen();

   int inc = 1;  /*!< Increment/Decrement per key press event */

   //Wait for user input
   while(ch != 'q')
   {
	 ch = getch();
	 switch(ch)
	 {
		 case '+':inc = 2; break;
		 case '-':inc = -2; break;
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
   endwin();  //Stop ncurses
   
   return 0;
}
      
      
