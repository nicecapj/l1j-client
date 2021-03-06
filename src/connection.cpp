#ifdef HAVE_CONFIG_H
#include <ac_config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "globals.h"

/**
Is the connection ok?
*/
int connection::connection_ok()
{
	return conn_ok;
}

/**
Close the connection to the update server and connect to the game server
*/
int connection::change()
{
	printf("\nConnecting to game server\n");
	if (sock != -1)
	{
		close(sock);
		sock = -1;
	}
	conn_ok = 0;
	try_names(the_config->get_game_port(srv_num));
	return conn_ok;
}

int connection::snd(packet_data &sendme)
{
	char *buf = new char[sendme.size()];
	for (int i = 0; i < sendme.size(); i++)
	{
		buf[i] = sendme[i];
	}
	snd(buf, sendme.size());
	delete [] buf;
	buf = 0;
}

/**
Send some data to the server. 
TODO: implement a "waiting buffer" when sending fails ?
TODO: limit how often this function will send data
*/
int connection::snd(const void* msg, int len)
{
	char *buf;
	int sent = 0;
	int trans;

	while (sent < len)
	{
		buf = (char*)msg;
		buf = &buf[sent];
		trans = send(sock, buf, len-sent, 0);
		if (trans > 0)
		{
			sent += trans;
		}
	}
	return len;
}

/**
Convenience function for sending a variable to the server. 
This function includes byte reordering that snd() does not do.
*/
int connection::snd_var(const void* msg, int len)
{
	if (len == 4)
	{
		unsigned int *temp = (unsigned int *)msg;
		*temp = htonl(*temp);
	}
	else if (len == 2)
	{
		unsigned short *temp = (unsigned short *)msg;
		*temp = htons(*temp);
	}
	char *buf;
	int sent = 0;
	int trans;
	while (sent < len)
	{
		buf = (char*)msg;
		buf = &buf[sent];
		trans = send(sock, buf, len-sent, 0);
		if (trans > 0)
		{
			sent += trans;
		}
	}
	return len;
}

/**
Just like snd_var, except it uses SWAP32 and SWAP16 macros
*/
int connection::snd_varg(const void* msg, int len)
{
	if (len == 4)
	{
		unsigned int *temp = (unsigned int *)msg;
		*temp = SWAP32(*temp);
	}
	else if (len == 2)
	{
		unsigned short *temp = (unsigned short *)msg;
		*temp = SWAP16(*temp);
	}
	char *buf;
	int sent = 0;
	int trans;

	while (sent < len)
	{
		buf = (char*)msg;
		buf = &buf[sent];
		trans = send(sock, buf, len-sent, 0);
		if (trans > 0)
		{
			sent += trans;
		}
	}
	return len;
}

/**
Recieve some data from the server
*/
int connection::rcv(void *buf, int len)
{
	int recvd = 0;
	int trans;
	char *temp;
	while (recvd < len)
	{
		temp = (char*)buf;
		temp = &temp[recvd];
		trans = recv(sock, temp, len-recvd, 0);
		if (trans > 0)
		{
			recvd += trans;
		}
	}

	return len;
}

/**
Recieve data from the server and byte order it (ntohl/ntohs)
*/
int connection::rcv_var(void *buf, int len)
{
	int recvd = 0;
	int trans;
	char *temp;
	while (recvd < len)
	{
		temp = (char*)buf;
		temp = &temp[recvd];
		trans = recv(sock, temp, len-recvd, 0);
		if (trans > 0)
		{
			recvd += trans;
		}
	}
	
	if (len == 4)
	{
		unsigned int *temp = (unsigned int *)buf;
		*temp = ntohl(*temp);
	}
	else if (len == 2)
	{
		unsigned short *temp = (unsigned short *)buf;
		*temp = ntohs(*temp);
	}

	return len;
}

/**
Recieve data from the server and byte order it (SWAP32/16)
*/
int connection::rcv_varg(void *buf, int len)
{
	int recvd = 0;
	int trans;
	char *temp;
	while (recvd < len)
	{
		temp = (char*)buf;
		temp = &temp[recvd];
		trans = recv(sock, temp, len-recvd, 0);
		if (trans >= 0)
		{
			recvd += trans;
		}
		else
		{
			if (recvd == 0)
			{
				return 0;
			}
		}
	}
	
	if (len == 4)
	{
		unsigned int *temp = (unsigned int *)buf;
		*temp = SWAP32(*temp);
	}
	else if (len == 2)
	{
		unsigned short *temp = (unsigned short *)buf;
		*temp = SWAP16(*temp);
	}

	return len;
}

int connection::make_connection()
{
	struct addrinfo *p;
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) 
	{
		if ((sock = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) 
		{
			perror("ERROR: cannot create socket\n");
			continue;
		}

		if (connect(sock, p->ai_addr, p->ai_addrlen) == -1) 
		{	
			#ifdef WINDOWS
			closesocket(sock);
			#else
			close(sock);
			#endif
//			perror("ERROR: connect");
			continue;
		}

		break;
	}


	#ifdef WINDOWS
	// If iMode!=0, non-blocking mode is enabled.
	u_long iMode=1;
	ioctlsocket(sock,FIONBIO,&iMode);
	#else
	int x = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, x | O_NONBLOCK);
	#endif

	if (p == NULL) 
	{
//		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	else
	{
		conn_ok = 1;
	}
	return 0;
}

int connection::get_addr(const char* port, const char* conto)
{
	int status;
			
	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	
	if ((status = getaddrinfo(conto, port, &hints, &servinfo)) != 0)
	{
	    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	    return -1;
	}
	return 0;
}

connection::connection(config* lcfg, int srv_n)
{
	the_config = lcfg;
	srv_num = srv_n;
	conn_ok = 0;
	sock = -1;
	servinfo = 0;
	try_names(the_config->get_port(srv_n));
}

void connection::try_names(const char *port)
{
	for (int i = 0; i < the_config->get_num_names(srv_num); i++)
	{
		printf("\tAttempting %s [%d of %d] port %s...\n", 
			the_config->get_addr(srv_num, i), i+1, 
			the_config->get_num_names(srv_num), port);
		printf("\tResolving server ip address...\n");		
		get_addr(port, the_config->get_addr(srv_num, i));
		printf("\tConnecting to ip address\n");
		make_connection();
		if (servinfo != 0)
		{
			freeaddrinfo(servinfo); // free the linked-list
			servinfo = 0;
		}
		if (connection_ok() == 1)
		{
			printf("\tsuccess\n");
			break;
		}
		else
		{
			printf("\tfailed\n");
		}
	}

}

connection::~connection()
{
	printf("Deleting connection\n");
	if (sock != -1)
	{
		#ifdef WINDOWS
		closesocket(sock);
		#else
		close(sock);
		#endif
	}
	if (servinfo != 0)
	{
		freeaddrinfo(servinfo); // free the linked-list
		servinfo = 0;
	}
	printf("\tDone\n");
}
