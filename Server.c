#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pty.h>
#include <termios.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <signal.h>
#include <assert.h>


//Declared constants:
#define PORTNUM 5910
#define SECRET "cs591secret\n"
#define MAXEVENTS 64


//Function prototypes:
void handle_client(int connect_fd);
char *input_matches_protocol(int infd, char *protocol_str);
void print_id_info(char *message);


int main()
{
  int listen_fd, connect_fd, fork_pid;
  struct sockaddr_in servaddr;
  struct epoll_event event;
  struct epoll_event events[MAXEVENTS];
  int aaaa[64];
  char buf[5120];

  //Set SIGCHLD signals to be ignored, which causes child process
  //results to be automatically discarded when they terminate:
  signal(SIGCHLD,SIG_IGN);

  //Create socket for server to listen on:
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
    	perror("Server: socket call failed");
    	exit(EXIT_FAILURE); 
   }

  //Set up socket so port can be immediately reused:
  int i=1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));

  //Set up server address struct (simplified vs. text):
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORTNUM);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  //Means accept all local interfaces.
  //As an alternative for setting up address, could have had for declaration:
  //struct sockaddr_in servaddr = {AF_INET, htons(PORTNUM), htonl(INADDR_ANY)};

  //Give socket a name/address by binding to a port:
  if (bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
   {
    	perror("Server: bind call failed");
    	exit(EXIT_FAILURE);
   }

  //Start socket listening:
  if (listen(listen_fd, 10) == -1) 
   {
    	perror("Server: listen call failed");
    	exit(EXIT_FAILURE); 
   }

  #ifdef DEBUG
  print_id_info("Server starting: ");
  #endif

  //creating epoll
  int efd = epoll_create1 (0);
  if (efd == -1)
   {
      perror ("epoll_create");
      abort ();
   }


  event.data.fd = listen_fd;
  event.events = EPOLLIN | EPOLLET;
  int s = epoll_ctl (efd, EPOLL_CTL_ADD, listen_fd, &event);
  if (s == -1)
   {
      perror ("epoll_ctl");
      abort ();
   }

  /* Buffer where events are returned */
  //events = calloc (MAXEVENTS, sizeof event);

  /* The event loop */
  while (1)
    { 
      int n, i;

      n = epoll_wait (efd, events, MAXEVENTS, -1);
      for (i = 0; i < n; i++)
      {
        
		if ((events[i].events & EPOLLERR) ||
             	   (events[i].events & EPOLLHUP) ||
              	   (!(events[i].events & EPOLLIN)))
	       	{
			/* An error has occured on this fd, or the socket is not
                 	ready for reading (why were we notified then?) */
	      		close (aaaa[events[i].data.fd]);
	      		close (events[i].data.fd);
	      		continue;
	 	}
	 	else if (listen_fd == events[i].data.fd)
	 	{
      
			//Accept a new connection and get socket to use for client:
   		 	 connect_fd = accept(listen_fd, (struct sockaddr *) NULL, NULL); 
	
         
      			
  				int stdin_fd, stdout_fd, stderr_fd;
				char *sock_input;
  				char *server1 = "<rembash2>\n";
  				char *server2 = "<ok>\n";
		
				//Write initial protocol ID to client:
  				if (write(connect_fd,server1,strlen(server1)) == -1) 
				{
    					perror("Server: Error writing to socket");
    					exit(EXIT_FAILURE);
				}

				//Check if correct shared secret was received: 
  				if ((sock_input = input_matches_protocol(connect_fd,SECRET)) != NULL) 
				{
    				fprintf(stderr,"Client: Invalid shared secret received from client: %s\n",sock_input);
    				exit(EXIT_FAILURE); 
				}

				//Send OK response to client:
  				if (write(connect_fd, server2, strlen(server2)) == -1) 
				{
    					perror("Server: Error writing OK to connection socket");
    					exit(EXIT_FAILURE); //Make child to run bash in:
  				}
					
				#ifdef DEBUG
    				print_id_info("Subprocess to exec bash, after setsid(): ");
    				#endif
				event.data.fd = connect_fd;
				event.events = EPOLLIN | EPOLLET;
                  		int s = epoll_ctl (efd, EPOLL_CTL_ADD, connect_fd, &event);
                  		if (s == -1)
                    		{
                      			perror ("epoll_ctl");
                      			abort ();
                    		}
    					
				//Setup stdin, stdout, and stderr redirection:
     				int masterfd, pid;
				if ((pid = forkpty(&masterfd, NULL, NULL, NULL)) < 0)
    					perror("FORK");
				else if (pid == 0)
				{			
		
    					if (execlp("bash","bash","--noediting","-i",NULL)<0)
					{
       		 				perror("execvp");
        	 				exit(2);
    					}

				}
				else
				{
					event.data.fd = masterfd;
					event.events = EPOLLIN | EPOLLET;
                  			int m = epoll_ctl (efd, EPOLL_CTL_ADD, masterfd, &event);
					aaaa[connect_fd] = masterfd;
					aaaa[masterfd] = connect_fd;

				}
      					
			
		}
	
			else 	
			{
        
				int done = 0;
                  		int count;

                  		count = read (events[i].data.fd, buf, 5120);
                      
                  		if (count == -1)
                    		{
                      			/* If errno == EAGAIN, that means we have read all
                         		data. So go back to the main loop. */
					if (errno != EAGAIN)
                        		{
                          			perror ("read");
                          			done = 1;
                        		}
                    		}
                  		else if (count == 0)
                    		{
                      			/* End of file. The remote has closed the
                         		connection. */
                      			done = 1;
                    		}
                        else
                        {

                  			/* Write the buffer to standard output */
                  			s = write (aaaa[events[i].data.fd], buf, count);
                  			if (s == -1)
                    			{
                      				perror ("write");
          
                    			}
                	}
			
			if (done)
                	{
                  		printf ("Closed connection on descriptor %d\n",
                          	events[i].data.fd);

                  		/* Closing the descriptor will make epoll remove it
                     		from the set of descriptors which are monitored. */
                  		close (events[i].data.fd);
                      close(aaaa[events[i].data.fd]);
                	}

		}
	}
  }}

	
char *input_matches_protocol(int infd, char *protocol_str)
{
  static char buff[513];  //513 to handle extra '\0'
  int nread;

  if ((nread = read(infd,buff,512)) == -1) {
      perror("Server: Error reading from socket");
      exit(EXIT_FAILURE); }

  buff[nread] = '\0';

  if (strcmp(buff,protocol_str) == 0)
    return NULL;
  else
    return buff;
}


		











