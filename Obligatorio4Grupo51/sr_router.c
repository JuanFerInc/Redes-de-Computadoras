/**********************************************************************
 * file:  sr_router.c
 * date:  Mon Feb 18 12:50:42 PST 2002
 * Contact: casado@stanford.edu
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing.
 *
 **********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    /* Initialize cache and cache cleanup thread */
    sr_arpcache_init(&(sr->cache));

    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t thread;

    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);
    
    /* Add initialization code here! */

} /* -- sr_init -- */


struct sr_rt* longest_prefix_match(struct sr_instance* sr, uint32_t ip){
	struct sr_rt* tabla = sr->routing_table;
	struct sr_rt* res = NULL;

	while (tabla != NULL){
		if (res == NULL || res->mask.s_addr < tabla->mask.s_addr){
			if (!((tabla->dest.s_addr ^ ip) & tabla->mask.s_addr)){
				res = tabla;
			}
		}
		tabla = tabla->next;
	}
	return res;
}



/* Send an ARP request. */
void sr_arp_request_send(struct sr_instance *sr, uint32_t ip) {

	printf("\nsr_arp_request_send\n\n");

	/* Buscar por que interfaz de salida mandar el mensaje */
	struct sr_rt* entrada_tabla = longest_prefix_match(sr, ip);

	struct sr_if* interfaz_salida = sr->if_list;
	while(interfaz_salida != NULL && strcmp(interfaz_salida->name, entrada_tabla->interface)){
		interfaz_salida = interfaz_salida->next;
	}

	if(interfaz_salida == NULL){
		printf("la interfaz de salida esta dando nulo\n");
	}

	uint8_t *macdestino = generate_ethernet_addr(255);

	unsigned int arpPacketLen = sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t);
	uint8_t *arpPacket = malloc(arpPacketLen);
	sr_ethernet_hdr_t *sendethHdr = (sr_ethernet_hdr_t *) arpPacket;
	memcpy(sendethHdr->ether_dhost, macdestino, ETHER_ADDR_LEN);
	memcpy(sendethHdr->ether_shost, interfaz_salida->addr, sizeof(uint8_t) * ETHER_ADDR_LEN);
	sendethHdr->ether_type = htons(ethertype_arp);
	sr_arp_hdr_t *sendarpHdr = (sr_arp_hdr_t *) (arpPacket + sizeof(sr_ethernet_hdr_t));
	sendarpHdr->ar_hrd = htons(1);
	sendarpHdr->ar_pro = htons(2048);
	sendarpHdr->ar_hln = 6;
	sendarpHdr->ar_pln = 4;
	sendarpHdr->ar_op = htons(arp_op_request);
	memcpy(sendarpHdr->ar_sha, interfaz_salida->addr, ETHER_ADDR_LEN);
	memcpy(sendarpHdr->ar_tha, macdestino, ETHER_ADDR_LEN);
	sendarpHdr->ar_sip = interfaz_salida->ip;
	sendarpHdr->ar_tip = ip;

	sr_send_packet(sr, arpPacket, arpPacketLen, &(interfaz_salida->name));

	free(arpPacket);
	free(macdestino);
}

/* Send an ICMP error. */
void sr_send_icmp_error_packet(uint8_t type,
                              uint8_t code,
                              struct sr_instance *sr,
                              uint32_t ipDst,
                              uint8_t *ipPacket)
{

	printf("sr_send_icmp_error_packet\n");

	struct sr_rt* entrada_tabla = longest_prefix_match(sr, ipDst);

	struct sr_if* interfaz_salida = sr->if_list;
	while(interfaz_salida != NULL && strcmp(interfaz_salida->name, entrada_tabla->interface)){
		interfaz_salida = interfaz_salida->next;
	}

	uint32_t ip_nexthop = entrada_tabla->gw.s_addr;


	unsigned int icmpHdr_len;
	if(type == 0){
		icmpHdr_len = ntohs(((sr_ip_hdr_t *)ipPacket)->ip_len) - sizeof(sr_ip_hdr_t);
	}else{
		icmpHdr_len = sizeof(sr_icmp_t3_hdr_t);
	}


	uint8_t *sendPacket = malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + icmpHdr_len);


	sr_ethernet_hdr_t *sendeHdr = (sr_ethernet_hdr_t *) sendPacket;
	memcpy(sendeHdr->ether_shost, interfaz_salida->addr, ETHER_ADDR_LEN);
	sendeHdr->ether_type = htons(ethertype_ip);


	sr_ip_hdr_t *sendipHdr = (sr_ip_hdr_t *) (sendPacket + sizeof(sr_ethernet_hdr_t));
	sendipHdr->ip_p = 1;
	sendipHdr->ip_ttl = 255;
	sendipHdr->ip_len = htons(sizeof(sr_ip_hdr_t) + icmpHdr_len);
	sendipHdr->ip_id = htons(0);
	sendipHdr->ip_off = htons(0);
	sendipHdr->ip_tos = 0;
	sendipHdr->ip_v = 4;
	sendipHdr->ip_hl = 5;
	sendipHdr->ip_dst = ipDst;
	sendipHdr->ip_src = interfaz_salida->ip;


	sr_icmp_t3_hdr_t *sendicmp = (sr_icmp_t3_hdr_t *) (sendPacket + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));

	sendicmp->icmp_code = code;
	sendicmp->icmp_type = type;


	if(type == 0){ /* echo reply */
		uint8_t *icmp_recivido_data = (uint8_t *) (ipPacket + sizeof(sr_ip_hdr_t) + 2*sizeof(uint8_t) + sizeof(uint16_t));
		uint8_t *data_icmp = (uint8_t *) &sendicmp->unused;

		memcpy(data_icmp, icmp_recivido_data, icmpHdr_len - (2*sizeof(uint8_t) + sizeof(uint16_t)));

	}else{
		memcpy(sendicmp->data, ipPacket, sizeof(uint8_t) * ICMP_DATA_SIZE);


	}

	sendicmp->icmp_sum = icmp3_cksum(sendicmp, icmpHdr_len);

	sendipHdr->ip_sum = ip_cksum(sendipHdr, sizeof(sr_ip_hdr_t));


	struct sr_arpentry *arpentry = sr_arpcache_lookup(&(sr->cache), ip_nexthop);
	char* nombre_interfaz = malloc(sr_IFACE_NAMELEN * sizeof(char));
	memcpy(nombre_interfaz, interfaz_salida->name, sr_IFACE_NAMELEN);

	if (arpentry == NULL) {
		struct in_addr ip_nexthop_struct;
		ip_nexthop_struct.s_addr = ip_nexthop;
		printf("IP next hop:\n");
		print_addr_ip(ip_nexthop_struct);
		sr_arpcache_queuereq(&(sr->cache), ip_nexthop, sendPacket, sizeof(sr_ip_hdr_t) + icmpHdr_len + sizeof(sr_ethernet_hdr_t), nombre_interfaz); /* verificar si la ip tiene que ir en este formato o no*/
	} else {
		if (arpentry->valid != 0) {
			memcpy(sendeHdr->ether_dhost, arpentry->mac, ETHER_ADDR_LEN); /* verificar si tiene que estar en este formato*/

			print_hdr_eth(sendeHdr);
			print_hdr_ip(sendipHdr);
			print_hdr_icmp(sendicmp);

			printf("Inicio validación del mensaje que mando\n");
			is_packet_valid(sendPacket, sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + icmpHdr_len);
			printf("Fin de validación\n");

			sr_send_packet(sr, sendPacket, sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + icmpHdr_len, nombre_interfaz);
		}

		free(arpentry);
	}

	free(sendPacket);

}

void sr_handle_arp_packet(struct sr_instance *sr,
        uint8_t *packet /* lent */,
        unsigned int len,
        uint8_t *srcAddr,
        uint8_t *destAddr,
        char *interface /* lent */,
        sr_ethernet_hdr_t *eHdr) {

	printf("sr_handle_arp_packet\n");

	print_hdr_eth(eHdr);

  /* Get ARP header and addresses */
	sr_arp_hdr_t *arpHdr = (sr_arp_hdr_t *) (packet + sizeof(sr_ethernet_hdr_t));
	print_hdr_arp(arpHdr);
	unsigned short opcode = ntohs(arpHdr->ar_op);
	unsigned char *srcmac = malloc(sizeof(unsigned char) * ETHER_ADDR_LEN);
	memcpy(srcmac, arpHdr->ar_sha, sizeof(unsigned char) * ETHER_ADDR_LEN);
	uint32_t srcip = arpHdr->ar_sip;
	uint32_t dstip = arpHdr->ar_tip;

  /* add or update sender to ARP cache*/
	struct sr_arpreq *request = sr_arpcache_insert(&(sr->cache), srcmac, srcip);

  /* check if the ARP packet is for one of my interfaces. */
	struct sr_if* interfaces = sr->if_list;
	while(interfaces != NULL && interfaces->ip != dstip){
		interfaces = interfaces->next;
	}
	if(interfaces != NULL){

	  /* check if it is a request or reply*/
		if (opcode == 1){ /*request*/
		/* if it is a request, construct and send an ARP reply*/
			unsigned int arpPacketLen = sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t);
			uint8_t *arpPacket = malloc(arpPacketLen);
			sr_ethernet_hdr_t *sendethHdr = (sr_ethernet_hdr_t *) arpPacket;
			memcpy(sendethHdr->ether_dhost, srcAddr, ETHER_ADDR_LEN);
			memcpy(sendethHdr->ether_shost, interfaces->addr, sizeof(uint8_t) * ETHER_ADDR_LEN);
			sendethHdr->ether_type = htons(ethertype_arp);
			sr_arp_hdr_t *sendarpHdr = (sr_arp_hdr_t *) (arpPacket + sizeof(sr_ethernet_hdr_t));
			sendarpHdr->ar_hrd = htons(1);
			sendarpHdr->ar_pro = htons(2048);
			sendarpHdr->ar_hln = 6;
			sendarpHdr->ar_pln = 4;
			sendarpHdr->ar_op = htons(arp_op_request);
			memcpy(sendarpHdr->ar_sha, interfaces->addr, ETHER_ADDR_LEN);
			memcpy(sendarpHdr->ar_tha, srcAddr, ETHER_ADDR_LEN);
			sendarpHdr->ar_sip = interfaces->ip;
			sendarpHdr->ar_tip = arpHdr->ar_sip;

			sr_send_packet(sr, arpPacket, arpPacketLen, &(interfaces->name));

			free(arpPacket);


		}else if (opcode == 2){ /*reply*/
		/* else if it is a reply, add to ARP cache if necessary and send packets waiting for that reply*/
			if(request != NULL){
				printf("Tengo paquetes esperando a ser enviados, los mando\n");
				struct sr_packet *packets = request->packets;
				while(packets != NULL){
					printf("%s\n",packets->iface);

					sr_ethernet_hdr_t *sendPacket = (sr_ethernet_hdr_t *)packets->buf;
					memcpy(sendPacket->ether_dhost, srcmac, ETHER_ADDR_LEN);

					sr_send_packet(sr, sendPacket, packets->len, packets->iface);
					packets = packets->next;
				}
				sr_arpreq_destroy(&(sr->cache), request);
			}

		}

	}else{}

	free(srcAddr);
	free(destAddr);

}

void sr_handle_ip_packet(struct sr_instance *sr,
        uint8_t *packet /* lent */,
        unsigned int len,
        uint8_t *srcAddr,
        uint8_t *destAddr,
        char *interface /* lent */,
        sr_ethernet_hdr_t *eHdr) {

	printf("sr_handle_ip_packet\n");

	/* Get IP header and addresses */
	sr_ip_hdr_t *ipHdr = (sr_ip_hdr_t *) (packet + sizeof(sr_ethernet_hdr_t));
	uint32_t ip_src = ipHdr->ip_src;
	uint32_t ip_dst = ipHdr->ip_dst;
	uint8_t ip_p = ipHdr->ip_p;
	uint8_t ip_ttl = ipHdr->ip_ttl;
	uint16_t ip_len = ntohs(ipHdr->ip_len);

	print_hdr_eth(eHdr);
	print_hdr_ip(ipHdr);

	/* Check if packet is for me or the destination is in my routing table*/
	struct sr_if* interfaces = sr->if_list;
	while(interfaces != NULL && interfaces->ip != ip_dst){
		interfaces = interfaces->next;
	}

	struct sr_rt* entrada_tabla = longest_prefix_match(sr, ip_dst);


	/* If non of the above is true, send ICMP net unreachable */
	if (interfaces == NULL && entrada_tabla == NULL) {

		printf("No es para mis interfaces ni está en mi tabla de routeo\n");
		sr_send_icmp_error_packet(3, 0, sr, ip_src, ipHdr);
	}

	/* Else if for me, check if ICMP and act accordingly*/
	else if (interfaces != NULL && interfaces->ip == ip_dst) {
		printf("Es para una de mis interfaces\n");
		if (ip_p == ip_protocol_icmp) { /* es ICMP*/
			printf("Es ICMP\n");
			sr_icmp_hdr_t *icmpHdr = (sr_icmp_hdr_t *) (packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));

			print_hdr_icmp(icmpHdr);

			uint8_t icmp_type = icmpHdr->icmp_type;
			uint8_t icmp_code = icmpHdr->icmp_code;
			if (icmp_type == 8 && icmp_code == 0) { /* echo request*/
				sr_send_icmp_error_packet(0, 0, sr, ip_src, ipHdr); /* echo reply*/
			}
		} else { /* es TCP o UDP*/
			printf("Es TCP o UDP\n");
			sr_send_icmp_error_packet(3, 3, sr, ip_src, ipHdr); /* port unreachable*/
		}

	}

	/* Else, check TTL, ARP and forward if corresponds (may need an ARP request and wait for the reply) */
	else {
		printf("No es para ninguna de mis interfaces y tengo el destino en mi tabla de fordwarding\n");
		/* Buscar por que interfaz de salida mandar el mensaje */
		struct sr_if* interfaz_salida = sr->if_list;
		while(interfaz_salida != NULL && strcmp(interfaz_salida->name, entrada_tabla->interface)){
			interfaz_salida = interfaz_salida->next;
		}

		uint32_t ip_nexthop = entrada_tabla->gw.s_addr;

		struct in_addr ip_nexthop_struct;
		ip_nexthop_struct.s_addr = ip_nexthop;
		printf("IP next hop:\n");
		print_addr_ip(ip_nexthop_struct);

		if (ip_ttl == 1) {
			sr_send_icmp_error_packet(11, 0, sr, ip_src, ipHdr); /* Time Exceeded, ttl expired*/
		} else {
			uint8_t *sendPacket = malloc(len);

			memcpy(sendPacket, packet, ip_len + sizeof(sr_ethernet_hdr_t));
			sr_ip_hdr_t *sendipHdr = (sr_ip_hdr_t *) (sendPacket + sizeof(sr_ethernet_hdr_t));
			sendipHdr->ip_ttl = ip_ttl - 1;
			sendipHdr->ip_sum = ip_cksum(sendipHdr, sizeof(sr_ip_hdr_t));

			sr_ethernet_hdr_t *sendeHdr = (sr_ethernet_hdr_t *) sendPacket;
			 memcpy(sendeHdr->ether_shost, interfaz_salida->addr, ETHER_ADDR_LEN);


			struct sr_arpentry *arpentry = sr_arpcache_lookup(&(sr->cache), ip_nexthop);
			char* nombre_interfaz = malloc(sr_IFACE_NAMELEN);
			memcpy(nombre_interfaz, interfaz_salida->name, sr_IFACE_NAMELEN);
			if (arpentry == NULL) {
				printf("No lo tengo en la ARP cache\n");

				sr_arpcache_queuereq(&(sr->cache), ip_nexthop, sendPacket, len, nombre_interfaz); /* verificar si la ip tiene que ir en este formato o no*/
			} else {
				if (arpentry->valid != 0) {
					printf("Si lo tengo el la ARP cache, lo mando\n");

					memcpy(sendeHdr->ether_dhost, arpentry->mac, ETHER_ADDR_LEN); /* verificar si tiene que estar en este formato*/

					print_hdr_eth(sendeHdr);

					sr_send_packet(sr, sendPacket, len, nombre_interfaz);
				}

				free(arpentry);
			}

			free(sendPacket);

		}

	}

}

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */)
{
  /* REQUIRES */
  assert(sr);
  assert(packet);
  assert(interface);

  printf("*** -> Received packet of length %d \n",len);

  /* Obtain dest and src MAC address */
  sr_ethernet_hdr_t *eHdr = (sr_ethernet_hdr_t *) packet;
  uint8_t *destAddr = malloc(sizeof(uint8_t) * ETHER_ADDR_LEN);
  uint8_t *srcAddr = malloc(sizeof(uint8_t) * ETHER_ADDR_LEN);
  memcpy(destAddr, eHdr->ether_dhost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  memcpy(srcAddr, eHdr->ether_shost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  uint16_t pktType = ntohs(eHdr->ether_type);

  if (is_packet_valid(packet, len)) {
    if (pktType == ethertype_arp) {
      sr_handle_arp_packet(sr, packet, len, srcAddr, destAddr, interface, eHdr);
    } else if (pktType == ethertype_ip) {
      sr_handle_ip_packet(sr, packet, len, srcAddr, destAddr, interface, eHdr);
    }
  }

}/* end sr_ForwardPacket */

