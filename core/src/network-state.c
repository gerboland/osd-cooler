/*
 *  Copyright(C) 2005 Neuros Technology International LLC.
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *
 *
 *  This program is distributed in the hope that, in addition to its
 *  original purpose to support Neuros hardware, it will be useful
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * Network routines
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2007-09-21 SZ
 *
 *
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/types.h>
#include "network-state.h"

#define IFR_NAME "eth0"
typedef enum
{
	ERROR = -1,
    DISCONNECT,
	CONNECT
}PHYSIC_NETWORK_STATE;

struct mii_data {  
       __u16 phy_id;   
      __u16 reg_num;
      __u16 val_in;  
       __u16 val_out; 
};  

/* check the newwork physical connetion */
static PHYSIC_NETWORK_STATE
network_physic_connection_check ( void )
{
	int skfd = 0;
	PHYSIC_NETWORK_STATE state;

	if ( ( skfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
	{
		perror( "socket" );             
		return ERROR;        
	}

	struct ifreq ifr;        
	bzero( &ifr, sizeof( ifr ) );       
	strncpy( ifr.ifr_name, IFR_NAME, IFNAMSIZ - 1 );      
	ifr.ifr_name[IFNAMSIZ - 1] = 0; 
	if ( ioctl( skfd, SIOCGMIIPHY, &ifr ) < 0 )
	{
		perror( "ioctl" );        
		return ERROR;       
	}

	struct mii_data* mii = NULL;        
	mii = (struct mii_data*)&ifr.ifr_data;      
	mii->reg_num = 0x01;        
	if ( ioctl( skfd, SIOCGMIIREG, &ifr ) < 0 )
	{
		perror( "ioctl2" );       
		return ERROR;  
	}

	if ( mii->val_out & 0x0004 )
		state = CONNECT;
	else
		state	= DISCONNECT;

	close( skfd );     
	return state; 
}

/* check the network state 1 success 0 failed */
int 
CoolNetworkStateCheck( void )
{
	PHYSIC_NETWORK_STATE phystate = network_physic_connection_check ( );

	switch ( phystate )
	{
	case DISCONNECT:
		return 0;
	case ERROR:
		return -1;
	case CONNECT:
		/* logic connection check */

		/* -------------end-----------*/
		return 1;
	default:
		break;
	}
	return 0;
}




