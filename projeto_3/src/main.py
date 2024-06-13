from scapy.all import rdpcap, IP, ls, ICMP, ARP
from decimal import Decimal
from typing import Dict, List
import argparse

class ICMPMetrics:
    def __init__(self, raw_pcap):
        self.raw_pcap = raw_pcap
        self.prc_pcap = list(filter(lambda x: ICMP in x, raw_pcap))
    
    def get_throughput(self) -> Dict[str, Decimal]:
        hosts, throughput = {}, {}

        # Gets first and last timestamp of pkts
        # for each host in pcap
        for pkt in self.prc_pcap:
            src = pkt[IP].src
            data = hosts.get(src)

            if data:
                hosts[src] = (data[0], pkt, data[2] + len(pkt[ICMP]))
            else:
                hosts[src] = (pkt, None, len(pkt[ICMP]))

        # Calculates avg throughput for each host    
        for host, data in hosts.items():
            throughput[host] = (data[2] * 8) / (data[1].time - data[0].time)

        return throughput

    def get_delay(self) -> Dict[str, Decimal]:
        hosts, delay = {}, {}

        # Gets all timestamps
        # for each host in pcap
        for pkt in self.prc_pcap:
            src = pkt[IP].src
            data = hosts.get(src)

            if data:
                hosts[src] = hosts[src] + [pkt]
            else:
                hosts[src] = [pkt]

        # Calculates avg delay for each host    
        for host, data in hosts.items():
            pairs = [(data[i - 1], data[i]) for i in range(1, len(data))]
            delay[host] = sum(map(lambda x: x[1].time - x[0].time, pairs)) / len(pairs)

        return delay

    def get_count(self) -> Dict[str, Dict[str, int]]:
        count = {}

        for pkt in self.raw_pcap:
            if ICMP in pkt:
                src = pkt[IP].src
                pkt_t = "ICMP"
            elif ARP in pkt:
                src = pkt[ARP].psrc
                pkt_t = "ARP"
            else:
                print("Found unrecognized pkt type.\nTerminating...")
                exit(1)
            
            data = count.get(src)
            if not data: count[src] = {}

            t_count = count[src].get(pkt_t)
            count[src][pkt_t] = 1 if t_count is None else count[src][pkt_t] + 1
        
        return count

    def get_hosts(self) -> Dict[str, List]:
        srcs, dsts = [], []

        # gets ping src and dst using echo (type == 8) and echo reply (type == 0)
        # ICMP type field value
        for pkt in self.prc_pcap:
            if pkt[ICMP].type == 8 and pkt[IP].src not in srcs:
                srcs.append(pkt[IP].src)
            elif pkt[ICMP].type == 0 and pkt[IP].src not in dsts:
                dsts.append(pkt[IP].src)

        return {"sources": srcs, "destinies": dsts}

def main():
    parser = argparse.ArgumentParser(description='Process pcap file')
    parser.add_argument("filename", type=str, nargs='+')
    args = parser.parse_args()
    files = args.filename

    for file in files:
        raw_pcap = rdpcap(file)
        metrics = ICMPMetrics(raw_pcap)

        print(f"Analysing \"{file}\"")
        print()

        print("Listing Hosts:")
        print("\n".join([f"\t{dk}: {''.join(dv)}" for dk, dv in metrics.get_hosts().items()]))

        print("Average Throughput:")
        print("\n".join([f"\t{dk}: {dv:.4f} b/s" for dk, dv in metrics.get_throughput().items()]))

        print("Average Time Between Packets:")
        print("\n".join([f"\t{dk}: {dv:.4f} s" for dk, dv in metrics.get_delay().items()]))

        count = metrics.get_count()
        print("Listing Number of Packets Sent:")
        print("\n".join([f"\t{dk}: {dv}" for dk, dv in count.items()]))

        print("-" * 48)

    
if __name__ == "__main__":
    main()