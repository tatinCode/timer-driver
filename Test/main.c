/**************************************************************
* File:: Tenorio_Justine_HW6_main.c
*
* Description::
*   This program is meant to run the driver from Module/TimerDriver.c.
*   It's a timer device driver which lets the user sets a timer via ioctl().
*   The read() call blocks until the timer expires.
*   There's also a prompt to input a custom message which could be skipped
*   by pressing [ENTER] and you'd get the default message.
*
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define DEVICE_PATH "/dev/TimerDriver"  //path to the driver
#define CMD_SET_TIMER 1                 //ioctl command to set timer

int parse_duration(const char *arg);    //function to parse user input like "1m"

int main(int argc, char *argv[]) {
    //variables
    int fd;
    int duration_ms;
    char buffer[256];
    char duration_input[256];
    char custom_msg[256];

    // if(argc != 2){
    //     printf("Error, please put in an argument (e.g., \"1m 30s\". 45s, 1h\n");
    //     return 1;
    // }

    //to get the duration string.
    //if there wasn't any arguments passed, then it asks the user for an input,
    //but if it detects that there were arguments, the program proceeds without
    //asking the user for input.
    if(argc == 2){
        strncpy(duration_input, argv[1], sizeof(duration_input));
        duration_input[sizeof(duration_input) - 1] = '\0';
    }
    else{
        //prompt user to input a duration string
        printf("\n(e.g., \"1m 30s\", 45s, 1h)\n");
        printf("No need for double quotes on single input\n");
        printf("Enter a timer duration: ");
        if(fgets(duration_input, sizeof(duration_input), stdin) == NULL){
            printf("Failed to read input, rerun program again.\n");
            printf("Exiting...");
            return 1;
        }

        //removes trailing newline
        duration_input[strcspn(duration_input, "\n")] = '\0';
    }

    //prompts the user for the custom message for a custom message, or just skip it 
    //and get the default message
    printf("Enter the message to display after the timer expires or just press enter:\n");
    if(fgets(custom_msg, sizeof(custom_msg), stdin) == NULL){
        printf("Failed to read message input.\n");
        printf("Exiting...");
        return 1;
    }

    //removes the newline before calling
    duration_input[strcspn(duration_input, "\n")] = '\0';
    
    //gets the parsed duration in ms.
    //when will they natively read in s -_-
    duration_ms = parse_duration(duration_input);
    if(duration_ms <= 0){
        printf("Invalid duration provided.\n");
        printf("Exiting...");
        return 1;
    }
    
    //opens the device
    fd = open(DEVICE_PATH, O_RDWR);
    if(fd < 0){
        printf("Failed to open device");
        printf("Exiting...");
        return 1;
    }    

    //strip newline from custom_msg
    custom_msg[strcspn(custom_msg, "\n")] = '\0';
    
    //send the message to the driver if one was entered
    if (strlen(custom_msg) > 0) {
    write(fd, custom_msg, strlen(custom_msg));
    } else {
        //send the default message explicitly to reset it
        // const char *default_msg = "Timer expired!\n";
        // write(fd, default_msg, strlen(default_msg));

        /**
         * I think setting the default message in myOpen() is better
         */
    }

    
    printf("\nSetting timer for %d ms...\n", duration_ms);

    //send timer duration to kernel module via ioctl
    if(ioctl(fd, CMD_SET_TIMER, duration_ms) < 0){
        printf("ioctl failed!");
        printf("Exiting...");
        close(fd);
        return 1;
    }

    //blocks until timer expires, then read the message from the driver
    printf("Waiting for the timer to expire...\n");
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if(bytesRead > 0){
        buffer[bytesRead] = '\0';
        printf("Read from driver: %s", buffer);
        printf("\n");
    }   
    else{
        printf("Read failed");
        printf("Exiting...");
    }

    close(fd);
    
    return 0;
}

/**
 * parses the argument(s) passed by the user. Would return the appropriate 
 * time in ms, but can parse a bunch of time measurements that I currently know
 * of, not adding anything below ms. And if a new time measurement comes out,
 * this driver would just be obselete, absolute legacy of a driver.
 * 
 * If you try to put anything other than these 4, they get converted to
 * seconds, no need to thank me for typos.
 * 
 *  - ms = millisecond  .001 of second per millisecond
 *  - s = seconds       1000ms per second
 *  - m = minutes       (1000ms * 60s) = 60000ms per minute
 *  - h = hour          (1000ms * 60s * 60m) = 360000ms per hour
 */
int parse_duration(const char *arg) {
    int total_ms = 0;
    char input[64];

    //makes a copy of the input so strtok doesn't modify the original
    //not even by accident
    strncpy(input, arg, sizeof(input));
    input[sizeof(input) - 1] = '\0';

    //takes the first space
    char *token = strtok(input, " ");

    while(token != NULL){
        int len = strlen(token);
        int value = 0;

        /**
         * checks for the appropriate arguments.sorry, no support 
         * for years, nanoseceonds, microseconds, picoseconds etc. 
         * this is just the one with most practical use.
         */
        if(len > 2 && strcmp(&token[len - 2], "ms") == 0){
            token[len - 2] = '\0';
            value = atoi(token);
            total_ms += value;              //for milliseconds
        }
        else if(token[len - 1] == 's'){
            token[len - 1] = '\0';
            value = atoi(token);
            total_ms += value * 1000;       //for seconds
        }
        else if(token[len - 1] == 'm'){
            token[len - 1] = '\0';
            value = atoi(token);
            total_ms += value * 60000;      //for minutes
        }
        else if(token[len - 1] == 'h'){
            token[len - 1] = '\0';
            value = atoi(token);
            total_ms += value * 3600000;    //for minutes
        }
        else{
            // token[len - 1] = '\0';
            value = atoi(token);
            total_ms += value * 1000;       //for seconds
        }

        token = strtok(NULL, " ");
    }
    return total_ms;
}
