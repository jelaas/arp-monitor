/*
 * File: arp-monitor.c
 * Implements: ARP monitoring software
 *
 * Copyright: Jens Låås 2008, jelaas@gmail.com
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#ifdef USE_SYSLOG
#include <syslog.h>
#else
#define LOG_PID 0
#define LOG_DAEMON 0
#define LOG_INFO 0
void openlog(const char *ident, int option, int facility)
{return;}
void syslog(int priority, const char *format, ...) {return;}
void closelog(void) {return;}
#endif
#ifndef VERSION
#define VERSION "Unknown"
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <sys/types.h>

#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */

#include <stdio.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <time.h>

#include "jelist.h"
#include <jelio.h>

#include <net/if.h>

#include "jelopt.h"

/* man 7 packet */

struct ipreg {
  char ip[16];
  time_t seen;
};

struct hw {
  unsigned char a[6];
  int ifindex;
  int pending;
  char hw[20];
  struct jlhead iplist;
  time_t seen;
};

struct {
  int timeout;
  int checkcount;
  int syslog;
  int quiet;
  int nocheck;
} conf;

char *indextoname(unsigned int ifindex)
{
  static char ifname[IF_NAMESIZE+1];
  
  if(if_indextoname(ifindex, ifname))
    return ifname;
  return "ENXIO";
}

char *ip_list(struct jlhead *iplist)
{
  char *s=NULL;
  size_t ulen=0, mlen=0;
  struct ipreg *ipreg;
  
  jl_foreach(iplist, ipreg)
    {
      sbuprintv(&s, &ulen, &mlen, 1024, ":%%", s(ipreg->ip));
    }
  if(iplist->len)
    sbuprintv(&s, &ulen, &mlen, 1024, ":");
  return s;
}

int ip_free(struct ipreg *ipreg)
{
  free(ipreg);
  return 0;
}


int ip_list_free(struct jlhead *l)
{
  struct ipreg *ipreg;
  
  jl_foreach(l, ipreg)
    ip_free(ipreg);
  jl_free_static(l);
  
  return 0;
}

int ip_timeout(struct hw *h, time_t now, time_t timeout)
{
  struct ipreg *ipreg;
  jl_foreach(&h->iplist, ipreg)
    {
      if( (ipreg->seen + timeout) < now )
	{
	  jl_del(ipreg);
	  ip_free(ipreg);
	  return 1;
	}
    }
  return 0;
}

int hw_timeout(struct jlhead *l, time_t timeout)
{
  struct hw *h;
  time_t now;
  static char logstr[512];

  now = time(0);

  jl_foreach(l, h)
    {
      if( (h->seen + (timeout*h->pending)) < now)
	{
	  h->pending++;
	  if(h->pending == (conf.checkcount+1))
	    {
	      sprintv(logstr, sizeof(logstr), "REL %% %% %% %% %%",
		      d(h->ifindex), s(indextoname(h->ifindex)),
		      s(h->hw), at(h->seen), fs(ip_list(&h->iplist)));
	      if(conf.syslog)
		syslog(LOG_INFO, "%s", logstr);
	      if(!conf.quiet)
		if(buprintv(1, "%%\n", s(logstr))==-1)
		  {
		    xx_error("Error writing to stdout\n");
		    exit(1);
		  }
	      
	      jl_del(h);
	      ip_list_free(&h->iplist);
	      free(h);
	      return 1;
	    }
	  if(!conf.nocheck)
	    {
	      sprintv(logstr, sizeof(logstr), "CHK %% %% %% %% %%",
		      d(h->ifindex), s(indextoname(h->ifindex)),
		      s(h->hw), at(h->seen), fs(ip_list(&h->iplist)));
	      if(conf.syslog)
		syslog(LOG_INFO, "%s", logstr);
	      if(!conf.quiet)
		if(buprintv(1, "%%\n", s(logstr))==-1)
		  {
		    xx_error("Error writing to stdout\n");
		    exit(1);
		  }
	    }
	}
      ip_timeout(h, now, timeout*conf.checkcount);
    }
  return 0;
}

struct hw *hw_exists(struct jlhead *l, unsigned char *addr, int ifindex)
{
  struct hw *h;

  jl_foreach(l, h)
    {
      if(!memcmp(h->a, addr, 6))
	{
	  if(h->ifindex == ifindex)
	    return h;
	}
    }
  return NULL;
}

struct ipreg *ip_new(const char *ip)
{
  struct ipreg *ipreg;

  if(!ip) return NULL;
  
  ipreg = malloc(sizeof(struct ipreg));
  if(ipreg)
    {
      ipreg->seen = time(0);
      sprintv(ipreg->ip, sizeof(ipreg->ip), "%%", s(ip));
    }
  return ipreg;
}

struct ipreg *ip_find(struct jlhead *iplist, const char *ip)
{
  struct ipreg *ipreg;
  
  if(!ip) return NULL;
  jl_foreach(iplist, ipreg)
    {
      if(!strcmp(ipreg->ip, ip))
	return ipreg;
    }
  return NULL;
}

struct hw *hw_new(const unsigned char *addr, const char *ip, int ifindex)
{
  struct hw *h;
  int i;
  struct ipreg *ipreg;
  
  h = malloc(sizeof(struct hw));
  if(h)
    {
      h->ifindex = ifindex;
      h->pending = 1;
      memcpy(h->a, addr, 6);
      h->seen = time(0);
      memset(h->hw, '0', sizeof(h->hw));
      for(i=0;i<6;i++)
	{
	  int j;
	  if(h->a[i]>15) j=0; else j=1;
	  sprintv(h->hw+i*3, sizeof(h->hw), "%%%%%%",
		  s(j?"0":""),
		  x(h->a[i]), s(i<5?":":""));
	}
      jl_new_static(&h->iplist);
      if(ip)
	{
	  ipreg = ip_new(ip);
	  if(ipreg) 
	    jl_append(&h->iplist, ipreg);
	  else
	    xx_error("Failed to register ip %% correctly\n", s(ip));
	}
    }
  return h;
}

int hw_ip_reg(struct hw *h, const char *ip)
{
  struct ipreg *ipreg;

  if(!ip) return 0;
  
  if((ipreg = ip_find(&h->iplist, ip)))
    {
      ipreg->seen = time(0);
      return 0;
    }

  jl_append(&h->iplist, ip_new(ip));
  
  return 1;
}

char *arp_getip(const unsigned char *buf, ssize_t n)
{
  char *ip=NULL;
  
  //for(i=0;i<n;i++) printf("%x:", buf[i]); printf("\n");

  // for(i=14;i<18;i++) printf("%x:", buf[i]); printf("\n");

  if(n > 17)
    sprintv_dup(&ip, 256, "%%.%%.%%.%%",
		u(buf[14]),u(buf[15]),u(buf[16]),u(buf[17]));
  
  xx_debug("ip=%%\n", s(ip));
  
  return ip;
}


int main(int argc, char **argv)
{
  int s, err=0;
  struct sockaddr_ll from;
  socklen_t fromlen = sizeof(from);
  char *device = NULL;
  int ifindex=-1;
  unsigned char buf[1024];
  char logstr[512];
  ssize_t n;
  struct jlhead *hwlist;
  struct hw *h;
  char *ip;
  
  conf.timeout = 60;
  conf.checkcount = 10;
  
  hwlist = jl_new();
  
  if(jelopt(argv, 'h', "help", NULL, &err))
    {
      xx_app("arp-monitor [-hiItcq]\n"
	     "Version: " VERSION "\n"
	     " -i --if INTERFACE\n"
	     "      Optionally only listen on INTERFACE.\n"
	     " -I --ifindex IFINDEX\n"
	     "      Optionally only listen on IFINDEX.\n"
	     " -t --timeout SECS [60]\n"
	     "      Every SECS seconds a CHK event is reported if the\n"
	     "      HW has not been seen.\n"
	     " -c --checkcount COUNT [10]\n"
	     "      Number of CHK events before release.\n"
	     " -n --nocheck\n"
	     "      Do not report CHK events.\n"
	     " -q --quiet\n"
	     "      Suppress output to stdout.\n"
#ifdef USE_SYSLOG
	     " --syslog\n"
	     "      Log to syslog using LOG_DAEMON.LOG_INFO\n"
#endif
	     " -h --help\n"
	     "\n"
	     "Default operation without any arguments is to listen on all\n"
	     "interfaces.\n"
	     "\nOutput:\n"
	     "EVENT IFINDEX IFNAME HW DATE TIME IPLIST\n"
	     "HW is the MAC address.\n"
	     "IPLIST is a colon separated list of IP addresses claimed by HW.\n"
	     "\nEVENT is one of the following:\n"
	     "REG = New MAC hardware seen.\n"
	     "IP  = New IP for MAC seen.\n"
	     "CHK = MAC not seen in timeout seconds.\n"
	     "REL = not seen in timeout*checkcount seconds. Removed from list.\n"
	     );
      exit(0);
    }

  while(jelopt(argv, 'q', "quiet", NULL, &err))
    conf.quiet = 1;
  while(jelopt(argv, 'n', "nocheck", NULL, &err))
    conf.nocheck = 1;
  while(jelopt(argv, 0, "syslog", NULL, &err))
    {
      openlog("dhcp-monitor", LOG_PID, LOG_DAEMON);
      conf.syslog = 1;
    }
  while(jelopt(argv, 'i', "if", &device, &err))
    ;
  while(jelopt_int(argv, 'I', "ifindex", &ifindex, &err))
    ;

  while(jelopt_int(argv, 't', "timeout", &conf.timeout, &err))
    if(conf.timeout < 1) conf.timeout=1;
  while(jelopt_int(argv, 'c', "checkcount", &conf.checkcount, &err))
    if(conf.checkcount < 1) conf.checkcount=1;
  
  argc = jelopt_final(argv, &err);
  
  if((argc>1) || err)
    {
      xx_error("Syntax error in options: %%\n", d(err));
      exit(1);
    }

  /* ETH_P_ALL ETH_P_ARP ETH_P_RARP ETH_P_IP ETH_P_IPV6 ETH_P_MPLS_UC */
  s = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_ARP));
  if(s == -1)
    {
      xx_error("socket(): %%\n", s(strerror(errno)));
      exit(1);
    }

  /*
    By default all packets of the specified protocol type are passed  to  a
       packet  socket.   To  only  get  packets  from a specific interface use
       bind(2) specifying an address in  a  struct  sockaddr_ll  to  bind  the
       packet   socket  to  an  interface.   Only  the  sll_protocol  and  the
       sll_ifindex address fields are used for purposes of binding.


       bind() till SO_BROADCAST som i broadcast-ip-udp kanske låter oss
       läsa enbart broadcast ip paket.
   */

  if(device)
    {
      /*

char *if_indextoname(unsigned ifindex, char *ifname);
if_nametoindex

       */

      ifindex = if_nametoindex(device);
      if(!ifindex)
	{
	  xx_error("%%: Not an interface\n", s(device));
	  exit(1);
	}
    }


  if(ifindex >= 0)
    {
      struct sockaddr_ll addr;
      memset(&addr, 0, sizeof(addr));

      addr.sll_family = AF_PACKET;
      addr.sll_protocol = htons(ETH_P_ARP);
      addr.sll_ifindex = ifindex;

      if(bind(s, &addr, sizeof(addr)))
	{
	  xx_error("bind() to interface failed\n");
	  exit(1);
	}
      xx_debug("Bound to ifindex %%\n", d(ifindex));
    }
  
  while(1)
    {
      struct pollfd fds;
      int rc;
      
      fds.fd = s;
      fds.events = POLLIN;
      rc = poll(&fds, 1, 1000);
      if(rc == 0)
	{
	  while(hw_timeout(hwlist, conf.timeout));
	  continue;
	}
      
      n = recvfrom(s, buf, sizeof(buf), 0, &from, &fromlen);
      
      /* look for sll_addr in list */
      if((h=hw_exists(hwlist, from.sll_addr, from.sll_ifindex)))
	{
	  h->seen = time(0);
	  h->pending = 1;
	  ip = arp_getip(buf, n);
	  if(hw_ip_reg(h, ip)>0)
	    {
	      sprintv(logstr, sizeof(logstr), "IP %% %% %% %% %%",
		      d(h->ifindex), s(indextoname(h->ifindex)),
		      s(h->hw), at(h->seen), fs(ip_list(&h->iplist)));
	      if(conf.syslog)
		syslog(LOG_INFO, "%s", logstr);
	      if(!conf.quiet)
		if(buprintv(1, "%%\n", s(logstr))==-1)
		  {
		    xx_error("Error writing to stdout\n");
		    exit(1);
		  }
	    }
	  if(ip) free(ip);

	}
      else
	{
#if 0
      printf("recvfrom() = %d\n", n);
	  printf("from.sll_protocol = %u\n", ntohs(from.sll_protocol));
	  printf("from.sll_pkttype = %u\n", from.sll_pkttype);
	  printf("from.sll_hatype = hardware type %u\n", from.sll_hatype);
	  printf("from.sll_ifindex = %d\n", from.sll_ifindex);
	  printf("from.sll_halen = %u\n", from.sll_halen);
	  // sll_addr[8]
	  for(i=0;i<6;i++) printf("%x:", from.sll_addr[i]); printf("\n");
#endif
	  if(ntohs(from.sll_protocol)==ETH_P_ARP)
	    xx_debug("ARP packet!\n");
	  

	  /* create new entry */
	  ip = arp_getip(buf, n);
	  h = hw_new(from.sll_addr, ip, from.sll_ifindex);
	  if(ip) free(ip);
	  if(!h)
	    {
	      xx_error("OOM\n");
	      exit(1);
	    }
	  sprintv(logstr, sizeof(logstr), "REG %% %% %% %% %%",
		  d(h->ifindex), s(indextoname(h->ifindex)),
		  s(h->hw), at(h->seen), fs(ip_list(&h->iplist)));
	  if(conf.syslog)
	    syslog(LOG_INFO, "%s", logstr);
	  if(!conf.quiet)
	    if(buprintv(1, "%%\n", s(logstr))==-1)
	      {
		xx_error("Error writing to stdout\n");
		exit(1);
	      }
	  jl_append(hwlist, h);
	}
      while(hw_timeout(hwlist, conf.timeout));
    }
  exit(0);
}
